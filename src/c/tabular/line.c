
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
#include "util.h"

#define MAX_CATEGORY_CARDINALITY (32)
#define TYPICAL_MAX_CARDINALITY  (5)

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

	struct table_description *fs
		= (struct table_description*)context;
	struct column *c
		= fs->column + off;

	union {
		long int ival;
		double fval;
	} u;
	char *endpt = NULL;
	int type = FTY_STRING; // unless demonstrated otherwise below.

	if( *field == 0 ) {
		c->type_vote[ FTY_EMPTY ] += 1;
		return;
	}

	/**
	  * 1. Determine the type
	  * ...by speculatively trying to treat it as a number since, if it is
	  * numeric, we'll operate on its value. Specifically, try to interpret
	  * the field as an integer (the most restrictive type) or else a float,
	  * and fall back to string if both fail. Note that even if it's a 
	  * floating-point representation of an int (1.0) we *want* to treat
	  * that as a float; someone upstream decided that floats were needed!
	  *
	  * Note that strtod DOES handle the distinguish values: [+-]?(inf|nan)
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
	  * 2. Regardless of type add its string value to the value set.
	  */

	if( ! c->excess_values ) {
		const int status
			= set_insert( & c->value_set, field );
		c->excess_values = (SZS_TABLE_FULL == status);
	} else {
		// TODO: count skipped values as a last resort?
	}

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
			if( i < COLUMNS ) {
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


static const char *DV_NA    // case-insensitive
	= "n/?a|missing|null|none|unavailable|empty";
/*
static const char *DV_FLOAT // case-insensitive
	= "[+-]?(nan|inf)";
static const char *DV_BOOL  // case-insensitive
	= "y(es)?|n(o)?|t(rue)?|f(alse)?";
*/

static bool _isalpha( const char *sz ) {
	while( *sz ) if( ! isalpha(*sz) ) return false;
	return true;
}


static int _fetch_string_values( struct strset *s, const char **out, int max ) {
	void *cookie;
	const char *v;
	int n = 0;
	if( set_iter( s, &cookie ) ) {
		while( n < max && set_next( s, &cookie, &v ) ) {
			if( _isalpha( v ) ) {
				out[ n++ ] = v;
			}
		}
	}
	return n;
}


/**
  * Having finished scanning a putatively tabular file, apply heuristics
  * to the column statistics to finally determine the statistical class
  * for each column (quantitative, categorical or (maybe) ordinal).
  * The determination considers:
  * 1) unanimity in one category
  * 2) cardinality of the labels
  * 3) magic string values that might be observed
  *
  * Propositions affecting the decision:
  * P1: the set of strings in the column has cardinality 1
  * P2: a string value matches a missing-data distinguished value
  * P3: excess (>32) values have been observed
  * P4  sample_size <= MAX_CATEGORY_CARDINALITY
  * P5: (P4 AND |{values}| < sample_size else) OR |{values}|*3 < sample_size else
  *
  * If there are TWO supported value types:
  *       I          F          S
  *     +------------------------
  *   I | x          QUA        Note 1
  *   F | x          x          Note 2
  *   S | x          x          x
  */
void _analyze_columns( struct table_description *d ) {

	for(int i = 0; i < d->table.column_count; i++ ) {

		struct column *c
			= d->column + i;
		const int N
			= set_count( & c->value_set );
		const int SUPPORTED_STAT_CLASSES
			= count_nonzero( c->type_vote+1 /* exclude FTY_EMPTY */, FTY_COUNT-1 );

		c->stat_class = STC_UNK; // ...unless overridden below!

		if( SUPPORTED_STAT_CLASSES == 0 /* all must be empty */ ) {

			assert( c->type_vote[ FTY_EMPTY ] > 0 );

		} else
		if( SUPPORTED_STAT_CLASSES == 1 /* Unanimity... */ ) {

			// ...doesn't necessarily determine the stat class because...

			switch( find_nonzero( c->type_vote, FTY_COUNT ) ) {

			case FTY_STRING:
				c->stat_class
					= c->excess_values
					? STC_UNK
					: STC_CAT;
				break;

			case FTY_INTEGER: // ...integers can be used in many ways!

				if( c->excess_values ) {

					if( (c->extrema[0] == 1.0 ) && 
						(c->extrema[1] == (double)(c->type_vote[FTY_INTEGER]) ) )
						c->stat_class = STC_ORD;
					else
						c->stat_class = STC_QUA;

				} else { // |{value_set}| <= MAX_CATEGORY_CARDINALITY

					// There are few enough values to treat it as a
					// categorical variable, but it has to pass extra
					// tests...

					if( c->has_negative_integers && N > TYPICAL_MAX_CARDINALITY )
						c->stat_class = STC_QUA;
					else
					if( __builtin_popcount( c->integer_magnitudes ) > 2 ) {
					}
				}

				if( d ) {
					c->stat_class
						= c->excess_values
						? STC_UNK
						: STC_CAT;
				}
				break;
			case FTY_FLOAT:
				c->stat_class = STC_QUA;
			}

		} else 
		if( SUPPORTED_STAT_CLASSES == 2 ) {

			const bool UNIQUE_STRING
				= c->type_vote[FTY_STRING] > 0;
			if( c->type_vote[FTY_INTEGER] > 0 && c->type_vote[FTY_STRING] > 0 ) {
			}
		}
	}
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

