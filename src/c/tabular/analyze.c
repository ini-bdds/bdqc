
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>

#include "format.h"
#include "scan.h"
#include "strbag.h"
#include "murmur3.h"
#include "rstrip.h"

#define MAX_LABELS_PER_COLUMN (32)

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

	struct file_analysis *fs
		= (struct file_analysis*)context;
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
	  * Try to interpret the field as an integer, floating-point number,
	  * and fall back to string if both fail.
	  * TODO: Because it is highly iterated this block is likely worthy
	  * of careful optimization.
	  */

	if( isdigit( *field ) /* preempt rel. expensive parsing by strtoX */ ) {
		if( ( u.ival = strtol( field, &endpt, 0 ), *endpt == '\0' ) ) {
			type = FTY_INTEGER;
		} else
		if( ( u.fval = strtod( field, &endpt    ), *endpt == '\0' ) ) {
			type = FTY_FLOAT;
		}
	} else
	if( strchr( "+-.", *field ) /* preceding may NOT be worthwhile! */
			&& ( u.fval = strtod( field, &endpt    ), *endpt == '\0' ) ) {
		type = FTY_FLOAT;
	}

	/**
	  * 2. Determine the type
	  */

	if( type == FTY_STRING ) {
		// Add it to the string table.
		if( c->label_set == NULL ) {
			c->label_set = sbag_create(
					MAX_LABELS_PER_COLUMN,
					true, /* duplicate strings */
					murmur3_32, // fnv_32,
					rand() );
			if( c->label_set == NULL )
				return; // catastrophic
		}
		if( ! c->label_set_exhausted ) {
			const int status
				= sbag_insert( c->label_set, field, NULL );
			if( status < 0 ) {
				if( SZS_TABLE_FULL == status )
					c->label_set_exhausted = true;
				else {
					fprintf( stderr, "error: unexpected empty field\n" );
					return;
				}
			}
		} else {
			// TODO: count skipped strings as a last resort
		}

	} else { // numeric, update our stats
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
		if( type == FTY_FLOAT ) X = u.fval; else X = u.ival;
		*mu = ( (N-0.0)*(*mu) + X )
			/   (N+1.0);
		delta = X - (*mu);
		if( N > 0 ) {
			*ss = ( (N-1.0)*(*ss) + (N+1.0) *delta*delta / N )
				/   (N-0.0);
		}
	}

	c->type_vote[ type ] += 1;
}


/**
  * Currently this only allocates accumulators for column statistics.
  */
int _init_analysis( struct file_analysis *a ) {
	const int COLUMNS
		= a->table.column_count;
	if( COLUMNS > 0 ) {
		a->column = calloc( COLUMNS, sizeof(struct column) );
		return a->column ? 0 : -1;
	} else
		return 0; // ...not failure, since nothing to allocate.
}


void fini_analysis( struct file_analysis *a ) {
	if( a->column ) {
		for(int i = 0; i < a->table.column_count; i++ ) {
			struct column *c
				= a->column + i; 
			if( c->label_set )
				sbag_destroy( c->label_set );
		}
		free( a->column );
		a->column = NULL;
	}
}


/**
  * Classify and, if appropriate, parse the line.
  * Given a line and a struct field_spec parse the line and 
  * update file statistics.
  * All we regard fixed for columns is their separation.
  */
int _analyze_row( struct file_analysis *a, char *line, int len ) {

	const char *PREFIX
		= a->table.metadata_line_prefix;
	const int PREFIX_LEN
		= a->table.metadata_line_prefix_len;

	assert( len > 0 );

	len = rstrip( line, len );

	if( len == 0 ) {
		a->empty_rows ++;
		return 0;
	}

	assert( strchr(line,'\r') == NULL && strchr(line,'\n') == NULL );

	if( PREFIX_LEN > 0 && memcmp( line, PREFIX, PREFIX_LEN ) == 0 ) {
		a->meta_rows ++;
		return 0;
	}

	/**
	  * Note: content never constitutes aberration in the context of this
	  * app; only an unexpected column count is aberration. Content, is
	  * merely tallied, whatever it is...
	  */

	if( a->table.parse_line( line, & a->table, _parse_field, a ) != a->table.column_count )
		a->aberrant_rows += 1;
	
	a->data_rows += 1; // ...AFTER the computations in parse_field!

	return 0;
}

