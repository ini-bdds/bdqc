
/**
  * FUTURE:
  * 1. An alternative for positive identification of ordinal variables:
  *    Maintain an interval tree in which adjacent intervals are merged
  *    when possible, so worst case tree size is N/2 nodes which would
  *    (transiently) consume as much memory as holding the entire column
  *    in memory. Average behavior should be much better and this does
  *    allow *positive* identification of ordinality.
  */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include "tabular.h"
#include "strset.h"
#include "murmur3.h"
#include "rstrip.h"
#include "column.h"
#include "environ.h"


/**
  * Currently this only allocates accumulators for column statistics.
  */
int _init_analysis( struct table_description *d ) {
	const int COLUMNS
		= d->table.column_count;
	if( COLUMNS > 0 ) {
		d->column = calloc( COLUMNS, sizeof(struct column) );
		if( d->column ) {
			int i = 0;
			for(; i < COLUMNS; i++ ) {
				if( set_init( & d->column[i].value_set,
					MAX_CATEGORY_CARDINALITY,
					true, /* duplicate strings */
					murmur3_32, // fnv_32,
					rand() ) ) break;
			}
			// Graceful, exhaustive clean-up and abort.
			if( i < COLUMNS || init_column_analysis() ) {
				// Note that i was NOT initialized above.
				while( i-- > 0 ) {
					set_fini( & d->column[i].value_set );
				}
				free( d->column );
				d->column = NULL;
			}
		}
		return d->column ? 0 : -1;
	} else
		return 0; // ...not failure, since nothing to allocate.
}


/**
  * Because, in general, a numeric column may contain other non-numeric
  * "magic" values (e.g. NaN, or NA), we reassess the type of each field on
  * every line and produce a tally of:
  * 1) ints
  * 2) floats
  * 3) strings
  * It is not considered an error if a column (field) contains something
  * other than current tallies might lead one to expect. We simply tally
  * everything up.
  * In general, there are no errors, only statistics to gather.
  * Returns number of chars consumed.
  */
static void _parse_field( const char *field, int off, void *context ) {

	struct table_description *d
		= (struct table_description*)context;
	struct column *c
		= d->column + off;

	union {
		long int ival;
		double fval;
	} u;
	char *endpt = NULL;
	int type = FTY_STRING; // unless demonstrated otherwise below.

	const int FIELD_LEN
		= strlen( field );

	if( FIELD_LEN == 0 ) {
		c->type_vote[ FTY_EMPTY ] += 1;
		return;
	}

	if( c->max_field_len < FIELD_LEN )
		c->max_field_len = FIELD_LEN;

	/**
	  * 1. Determine the type
	  * ...by speculatively trying to treat it as a number since, if it is
	  * numeric, we'll operate on its value. Specifically, try to interpret
	  * the field as an integer (the most restrictive type) or else a float,
	  * and fall back to string if both fail. Note that even if it's a 
	  * floating-point representation of an int (1.0) we *want* to treat
	  * that as a float; someone upstream decided that floats were needed!
	  *
	  * Note that strtod DOES handle the special values: /[+-]?(inf|nan)/i
	  * as demonstrated by:
	  * #include <stdio.h>
	  * #include <stdlib.h>
	  * void main(int argc,char*argv[]) {printf("%f\n",strtod(argv[1],NULL));}
	  */

	if( ( u.ival = strtol( field, &endpt, 0 ), *endpt == '\0' ) ) {
		type = FTY_INTEGER;
		// ...and update two other statistics associated with integers.
		c->has_negative_integers
			= c->has_negative_integers
			|| u.ival < 0;
		c->integer_magnitudes
			|= ( 1 << (int)floorf(log10f(1.0+abs(u.ival))) );

	} else // ...*entire* string wasn't consumed by strtol, so...
	if( ( u.fval = strtod( field, &endpt    ), *endpt == '\0' ) ) {
		type = FTY_FLOAT;
	}

	/**
	  * 2. Add string and integer values to the value set.
	  *    Never add floats since they *almost* always indicate
	  *    a quantitative variable and need not participate in
	  *    the decision tree in _analyze_column.
	  */

	if( FIELD_LEN <= MAXLEN_CATEGORY_LABEL ) {

		/**
		  * !!! Warning !!!
		  * If the set is being used as a bag, call set_insert even when
		  * excess_values so that item counts are updated.
		  * Otherwise, elide set_insert because it is rather expensive.
		  * !!! Warning !!!
		  */

		if( c->excess_values == 0 && type != FTY_FLOAT ) {

			const int status
				= set_insert( & c->value_set, field );
			if( SZS_TABLE_FULL == status ) {
				// Note the (0-based) line number at which the table filled.
				c->excess_values
					= d->rows.empty
					+ d->rows.meta
					+ d->rows.data;
				// Since the table can't possibly fill on the 0th line,
				// excess_values can be treated as a boolean, too.
			}

		} else {
			// TODO: count skipped values as a last resort?
		}
	} else
		c->long_field_count ++;

	if( type > FTY_STRING /* it's numeric */ ) {

		assert( type == FTY_FLOAT || type == FTY_INTEGER );

		/**
		  * Compute recursive means and variances for each of the sample
		  * points.
		  *             t-1             1
		  * mu_t     =  --- mu_{t-1} + --- x_t,             t=1,2,..N
		  *              t              t
		  *
		  *
		  *             t-1             1
		  * ss_t     =  --- ss_{t-1} + --- (x_t - mu_t)^2   t=1,2,..N
		  *              t             t-1
		  */

		const double N
			= c->type_vote[ FTY_INTEGER ]
			+ c->type_vote[ FTY_FLOAT ];
		double *mu = c->statistics + 0;
		double *ss = c->statistics + 1;
		double X, delta;

		// Coerce, if necessary.

		if( type == FTY_FLOAT )
			X = u.fval;
		else
			X = u.ival;

		// Update the stats

		*mu = ( (N-0.0)*(*mu) + X )
			/   (N+1.0);
		delta = X - (*mu);
		if( N > 0 ) {
			*ss = ( (N-1.0)*(*ss) + (N+1.0) *delta*delta / N )
				/   (N-0.0);
		}

		// Update the extrema

		if( N > 0 ) {
			if( c->extrema[0] > X )
				c->extrema[0] = X;
			else
			if( c->extrema[1] < X )
				c->extrema[1] = X;
		} else {
			c->extrema[0] = X;
			c->extrema[1] = X;
		}
	}

	c->type_vote[ type ] += 1;
}


/**
  * Classify and, if appropriate, parse the line.
  * Given a line and a struct field_spec parse the line and 
  * update file statistics.
  * All we regard fixed for columns is their separation.
  */
int _analyze_line( struct table_description *d, char *line, int len ) {

	const char *PREFIX
		= d->table.metadata_line_prefix;
	const int PREFIX_LEN
		= d->table.metadata_line_prefix_len;

	assert( len > 0 );

	len = rstrip( line, len );

	if( len == 0 ) {
		d->rows.empty ++;
		return 0;
	}

	assert( strchr(line,'\r') == NULL && strchr(line,'\n') == NULL );

	if( PREFIX_LEN > 0 && memcmp( line, PREFIX, PREFIX_LEN ) == 0 ) {
		d->rows.meta ++;
		return 0;
	}

	/**
	  * Note: content never constitutes aberration in the context of this
	  * app; only an unexpected column count is aberration. Content, is
	  * merely tallied, whatever it is...
	  */

	if( d->table.split_line( line, & d->table, _parse_field, d ) != d->table.column_count )
		d->rows.aberrant += 1;
	
	d->rows.data += 1; // ...AFTER the computations in parse_field!

	return 0;
}


/**
  * Infer statistical class of each column.
  */
void _fini_analysis( struct table_description *d ) {

	for(int i = 0; i < d->table.column_count; i++ )
		analyze_column( d->column + i );
	fini_column_analysis();
}


void tabular_free( struct table_description *d ) {
	if( d->column ) {
		for(int i = 0; i < d->table.column_count; i++ ) {
			set_fini( & d->column[i].value_set );
		}
		free( d->column );
		d->column = NULL;
	}
}
