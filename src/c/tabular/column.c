
/**
  * This module is concerned with analyzing vectors (assumed to come from
  * table columns) of data of unknown type and statistical class.
  * Types are inferred from syntax; statistical class is inferred from
  * many heuristics on the observed types, their values, and various other
  * metadata collected (in struct column) about them.
  *
  * There is a persistent problem with essentially nominative variables
  * (e.g. identifiers) or essentially non-statistical data such as DNA
  * sequence fragments being identified as categorical.
  */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <math.h>
#include <regex.h>
#include <alloca.h>
#include <assert.h>

#include "strset.h"
#include "column.h"
#include "util.h"

/**
  * Compile regular expressions to identify special string values:
  * 1. missing data placeholders
  * 2. boolean values
  */
static const char *_patt_NA   // case-insensitive
	= "n/?a|missing|null|none|unavailable|empty";
static regex_t _cre_NA;

#ifdef HAVE_BOOLEAN_DETECTION
static const char *_patt_BOOL // case-insensitive
	= "y(es)?|n(o)?|t(rue)?|f(alse)?";
static regex_t _cre_BOOL;
#endif

/**
  * A wrapper around regcomp to dump error information.
  * This is only important if/when user is allowed to override
  * our regular expression definitions with their own.
  */
static int _compile( const char *patt, regex_t *cre, int flags ) {

	const int regerr
		= regcomp( cre, patt, flags );
	if( regerr ) {
		const size_t required 
			= regerror( regerr, cre, NULL, 0 );
		char *buf 
			= alloca( required );
		regerror( regerr, cre, buf, required );
		fprintf( stderr, "%s\n", buf );
		return -1;
	}
	return 0;
}


void fini_column_analysis() {
#ifdef HAVE_BOOLEAN_DETECTION
	regfree( &_cre_BOOL );
#endif
	regfree( &_cre_NA );
}

int init_column_analysis() {

	const int FLAGS
	   = REG_EXTENDED | REG_NOSUB | REG_ICASE | REG_NEWLINE;

	if( _compile( _patt_NA,   &_cre_NA,   FLAGS ) )
		return -1;
#ifdef HAVE_BOOLEAN_DETECTION
	if( _compile( _patt_BOOL, &_cre_BOOL, FLAGS ) )
		return -1;
#endif
	return 0;
}


static bool _isalpha( const char *sz ) {
	while( *sz ) if( ! isalpha(*sz++) ) return false;
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
  * Handle the case of a vector consisting entirely of integers (possibly
  * after removal of a unique placeholder string for missing data). This is
  * the trickiest case since it could be any or none of the 3 statistical
  * classes.
  * TODO: For plausibly categorical cases might be better to consider number
  * of duplicate values (using value_bag instead of value_set).
  */
static int _integer_inference( const struct column *c ) {

	int stat_class = STC_UNK; // ...unless overridden below.

	const int N
		= c->type_vote[ FTY_INTEGER ];
	const double K
		= set_count( & c->value_set );
	const int N_MAG
		= __builtin_popcount( c->integer_magnitudes );

	if( c->excess_values ) {

		// It can only be ordinal or quantitative, and...

		if( c->has_negative_integers ) { // ...it's not ordinal.

			stat_class = STC_QUA;

		} else {

			const int MAX_MAG
				= (int)floorf( log10f( c->extrema[1] ) );

			// Following can't positively id ordinal variables,
			// but it can positively rule them out. I'm sampling
			// the interval [1,N] divided into ranges by base-10
			// magnitude. If data are missing in any range, it's
			// not strictly ordinal...or it's just missing some
			// data. TODO: See comments at top.

			if( ( N_MAG == MAX_MAG )
				&& ( (int)round(c->extrema[0]) == 1 )
				&& ( (int)round(c->extrema[1]) == N ) )
				stat_class = STC_ORD;
			else
				stat_class = STC_QUA;
		}

	} else { // |{value_set}| <= MAX_CATEGORY_CARDINALITY

		// There are few enough values to treat it as a
		// categorical variable, but...

		if( c->has_negative_integers ) {

			// ...require all values to be in (-K,+K).
			// e.g. { -2, -1, 0, 1, 2 } with -2 indicating
			// "strongly dislike" and +2 "strongly like"

			stat_class 
				= (-K <= c->extrema[0]) && (c->extrema[1] <= +K)
				? STC_CAT
				: STC_QUA;

		} else {

			// The column takes on a "small" set of non-negative
			// integer values. Very likely categorical. The
			// relation between the cardinality of the value set
			// and the sample size is primary determinant now...

			stat_class 
				 = K < N
				 ? STC_CAT
				 : STC_QUA;

			// Zip codes are a confounding case; they're neither
			// categorical nor quantitative. They're nominative.
			// If a small enough subset of zipcodes are present
			// in a given data set, treatable as categorical.
		}
	}
	return stat_class;
}


/**
  * Having finished scanning a putatively tabular file, apply heuristics
  * to the column statistics to finally determine the statistical class
  * for each column (quantitative, categorical or (maybe) ordinal).
  *
  * The determination depends primarily on the presence or lack of unanimity
  * in the observed types (integer, float, string) in the column, then
  * devolves to consideration of the many other statistics in the column
  * struct.
  */
void analyze_column( struct column *c ) {

	const int OBSERVED_TYPE_COUNT
		= count_nonzero( c->type_vote+1 /* exclude FTY_EMPTY */, FTY_COUNT-1 );

	c->stat_class = STC_UNK; // ...unless overridden below!

	if( OBSERVED_TYPE_COUNT == 0 /* all must be empty */ ) {

		assert( c->type_vote[ FTY_EMPTY ] > 0 );

	} else
	if( OBSERVED_TYPE_COUNT == 1 /* Unanimity... */ ) {

		// ...doesn't necessarily determine the stat class because...

		switch( find_nonzero( c->type_vote, FTY_COUNT ) ) {

		case FTY_INTEGER: // ...integers can be used in many ways!
			c->stat_class = _integer_inference( c );
			break;

		case FTY_STRING:

			if( ( ! c->excess_values )
				&& set_count( & c->value_set ) <  c->type_vote[ FTY_STRING ] 
				&& c->long_field_count == 0 )
				c->stat_class = STC_CAT;
			break;

		case FTY_FLOAT:
			c->stat_class = STC_QUA;
		}

	} else {

		/**
		  * If more than two types are observed and STRING is one of them,
		  * then everything hinges on the cardinality of observed strings
		  * (the contents of value_set EXCLUDING any integers it contains).
		  * 1. If exactly one string value is observed AND it's a potential
		  *    missing data indicator, then inference devolves to that for
		  *    numeric types.
		  * 2. If more than one string value is observed all bets are off;
		  *    the column remains STC_UNKnown.
		  */

		const char *sval[2];
		const bool UNIQUE_STRING
			= c->type_vote[ FTY_STRING ] > 0
			&& _fetch_string_values( & c->value_set, sval, 2 ) == 1;
		const bool HAS_CANDIDATE_MISSING_DATA_PLACEHOLDER
			= UNIQUE_STRING
			&& regexec( &_cre_NA, sval[0], 0, NULL, 0 ) == 0;

		if( OBSERVED_TYPE_COUNT == 2 ) {

			if( c->type_vote[ FTY_STRING ] > 0 ) {

				if( HAS_CANDIDATE_MISSING_DATA_PLACEHOLDER ) {
					if( c->type_vote[ FTY_INTEGER ] > 0 ) {
						c->stat_class = _integer_inference( c );
					} else {
						assert( c->type_vote[ FTY_FLOAT ] > 0 );
						c->stat_class = STC_QUA;
					}
				}

			} else { // no string, just ints and floats

				assert( c->type_vote[ FTY_INTEGER ] > 0 &&
						c->type_vote[ FTY_FLOAT ]   > 0 );

				c->stat_class = STC_QUA;
			}

		} else { // Column contains int(s), float(s) AND string(s).

			if( HAS_CANDIDATE_MISSING_DATA_PLACEHOLDER )
				c->stat_class = STC_QUA;

		} // 3 types observed

	} // > 1 type observed.
}

