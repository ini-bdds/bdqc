
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "utf8.h"
#include "sspp.h"
#include "tabular.h"
#include "rstrip.h"

/**
  * Some of the scan subsystem is 
  */
extern int _format_infer( FILE *fp, int delim, int nlines, struct format *inferred );
extern int _init_analysis( struct table_description * );
extern int _analyze_line( struct table_description *, char *line, int len );
extern void _analyze_columns( struct table_description * );

#define MAX_COUNT_HEADER_LINES (256)
#define MAX_COUNT_SAMPLE_LINES (16)

/**
  * Used by all calls to getdelim().
  */
static size_t _blen = 0;
static char *_line = NULL;

enum {
	ASTAT_SYSERR = -1,
	// A system error (e.g. malloc failure) precluded (further) analysis.
	ASTAT_ABORT,
	// The data is not tabular, so just fall back to character counting.
	ASTAT_CONTINUE
	// We seem to have a table and are proceeding on that assumption.
};

/**
  * Names of Character Classes declared in header.
  */
const char *CC_NAME[ CC_COUNT ] = {
	"LF",
	"CR",
	"CHAR",  // Any ASCII character except { LF, CR }
	// Transition matrix only uses above 3.
	"UTF8/2",
	"UTF8/3",
	"UTF8/4"
};

/**
  * Notice this deals with Character Classes as defined above, NOT ASCII
  * chars like \n and \r!
  */
static inline bool _is_line_terminator_cc( unsigned cc ) {
	return cc < CC_CHAR;
}


/**
  * AUDIT: tabular_error count must match error code count.
  */
static const char *_error_template[E_COUNT] = {
	"OK",
	"no table detected, only character stats are valid",
	"not UTF8: invalid UTF8 prefix",
	"not UTF8: invalid UTF8 suffix",
	"file I/O error",
	"uninitialized output struct"
};


struct state {

	/**
	  * Count of "suffix" bytes of a UTF8 character.
	  * ASCII characters have suffix_len == 0. In general, the part of
	  * the preceding buffer that is valid is 1 + .suffix_len.
	  */
	int      suffix_len;

	/**
	  * The character class in utf8[].
	  */
	unsigned cc_curr;

	/**
	  * The character class of the previous contents of utf8[].
	  */
	unsigned cc_last;

	/**
	  * Counts of bytes and characters consumed.
	  * These are always equal for ASCII content, differ only in the
	  * presence of non-trivial (non-ASCII) UTF8 characters.
	  */
	long nbytes;
	long nchars;

	/**
	  * Line Prefix Analysis State
	  */
	struct sspp_analysis_state *lpas;

	/**
	  * Lines may be separated by 1- or 2-character sequences.
	  * This member is the *final* character regardless of the length.
	  * (This is inferred from the first line and *assumed* for the purposes
	  * of parsing to be consistent throughout the file; inconsistencies, if
	  * present, will be reflected in the counts in the transition matrix
	  * above.)
	  */
	int final_line_separator;

	/**
	  * Number of lines consumed
	  * This is updated ONLY in the _cs_* functions.
	  */
	unsigned lines;

	/**
	  * Used during read of initial part of file to cache the assumed-
	  * representative part for out-of-band analysis.
	  */
	FILE *cache;

	/**
	  * Warning: these functions WILL be called on each and every character
	  * read, including the first...in which .last will not be meaningful.
	  * 1. .cc_last MUST be initialized to invalid value, and
	  * 2. implementations of these functions must recognize that.
	  * This member is also a boolean indicating (at EOF) whether an
	  * analysis was aborted or fully carried out.
	  */
	int (*check_state)( struct state * );
	/**
	  * Number of lines remaining to be consumed before a forced state
	  * transition occurs.
	  */
	int state_lines;

	struct table_description *analysis;
};


/**
  * 1. Analyzes the cached initial lines of file to infer table properties.
  * 2. based on #1 creates structs/buffers required for subsequent content
  *    analysis
  * 3. immediately carries out content analysis on cached lines if #1 and
  *    #2 succeeded.
  * Return is one of the ternary ASTAT_x codes.
  */

static int _analyze_top_lines( struct state *s ) {

	ssize_t llen;

	rewind( s->cache );

	if( _format_infer( s->cache, s->final_line_separator, s->lines, & s->analysis->table ) )
		return ASTAT_ABORT;

	assert( s->analysis->table.column_count > 0 );

	if( _init_analysis( s->analysis ) )
		return ASTAT_SYSERR;

	rewind( s->cache );

	while( (llen = getdelim( &_line, &_blen, s->final_line_separator, s->cache )) > 0 )
		_analyze_line( s->analysis, _line, llen );

	rewind( s->cache );

	return ASTAT_CONTINUE;
}


/**
  * The possible values for state.check_state.
  */
static int _cs_infer_lineterm( struct state *s );
static int _cs_discard_header( struct state *s );
static int _cs_acquire_sample( struct state *s );
static int _cs_analyze_content( struct state *s );


/**
  * This detects the end of the first line which is signalled by the first
  * encounter of successive bytes matching /[\n\r]./.
  *
  * We recognize (allow) four possible line endings:
  * 1. LF
  * 2. CR
  * 3. LFCR
  * 4. CRLF
  *
  * Note that we don't know how lines are terminated until this function
  * ascertains it, so it is necessary to read one char past the first
  * candidate (ASCII) line termination char, which might be:
  * 1. the first character of the next line, or
  * 2. the terminator of the 2nd line if the 2nd line is empty!
  *
  * TODO: Handle case where the '.' in the regex is EOF.
  */
static int _cs_infer_lineterm( struct state *s ) {

	assert( s->lines == 0 /* else shouldn't be in this function! */ );

	if( _is_line_terminator_cc( s->cc_last ) ) {

		const char LAST
			= (s->cc_last == CC_LF) ? '\n' : '\r';

		if( _is_line_terminator_cc( s->cc_curr ) ) {

			// Valid terminations: \n, \r, \r\n, \n\r, but \r\r and \n\n
			// indicate the ends of TWO lines, the 2nd of them empty!

			if( s->cc_last == s->cc_curr ) {

				// We've consumed a 2nd empty line!

				s->final_line_separator = LAST;

				// This is for the preceding line...
				sspp_flush( s->lpas );
				s->lines += 1;
				// ...and current is handled at bottom.

			} else { // Two-character termination is in use...

				// ...and the current char is the final!
				s->final_line_separator = s->analysis->utf8[0];
			}

		} else {

			s->final_line_separator = LAST;
		}

		sspp_flush( s->lpas );
		s->lines += 1;

		// The final act of this cache monitor is to transfer 
		// control to the next...
		s->check_state = _cs_discard_header;
		s->state_lines = MAX_COUNT_HEADER_LINES - 1;
	}

	if( ! _is_line_terminator_cc( s->cc_curr ) )
		sspp_push( s->lpas, s->analysis->utf8, 1 + s->suffix_len );

	return ASTAT_CONTINUE;
}


/**
  * Localize the decision about what makes a valid header/metadata prefix.
  */
static bool _is_admissable_prefix( const char *prefix ) {
	// TODO: Make following UTF8-aware using Unicode char categories
	return ispunct( *prefix );
}


/**
  * Monitors the caching of the file's initial lines, assumed to include a
  * header. Table format (column separation and count) must be inferred from
  * data lines, so the intent is to skip a header, if present. Generally
  * speaking, a change in the prefix character of lines (along with some
  * other heuristics) is assumed to indicate completion of a header (which
  * is allowed to be multi-line).
  *
  * This activity ends as soon as either:
  * 1) the end of a putative header is detected, or
  * 2) we've read far enough into a file without detecting a change in
  *    prefix character that we assume there was no header.
  *
  * Note that false positives are generally harmless here. That is, it's
  * harmless to skip a number of data lines as if they were header lines
  * AS LONG AS immediately subsequent lines are still data; table structure
  * can still be properly inferred. Only false negatives--*failing* to skip
  * lines that are genuinely part of a header (or otherwise metadata)--is
  * Bad since table structure likely cannot be correctly inferred from them.
  *
  * Note that the reliability of _format_infer() is roughly proportional
  * to the size of the line group it analyzes because the probability of
  * the counts of any candidate separator character being equal on every
  * line *just by chance* decreases as the line count increases.
  */
static int _cs_discard_header( struct state *s ) {

	if( s->analysis->utf8[0] == s->final_line_separator /* we've finished another line */ ) {

		s->lines  ++;
		s->state_lines --;

		if( sspp_flush( s->lpas ) == SSPP_GROUP_COMPLETION ) {

			struct sspp_group *g
				= sspp_group_ptr( s->lpas, -1 );

			/**
			  * We've identified a set of lines potentially constituting a
			  * header. The prefix that distinguished these lines may or may
			  * not be an actual header/metadata indicator; it may just be a
			  * prefix common to initial data lines. Setting it into the
			  * analysis->table causes all lines with this prefix to be
			  * skipped in all subsequent data analysis, including the 
			  * analysis of the cache!
			  * Multiple problems here:
			  * 1) Some formats (e.g. "PROSITE" protein format on 
			  *    expasy.org) use strictly alnum prefixes.
			  * 2) "True" CSV format may have quoted data anywhere, so
			  *    initial quotes (ispunct==true) will appear to be metadata
			  *    flags unless they're explicitly excluded. However, in
			  *    some file formats initial quotes are comment indicators.
			  *
			  *    A comprehensive solution requires histogramming rows'
			  *    initial characters across entire file. Usually, metadata
			  *    character will be a tiny minority among line initiators.
			  */

			if( _is_admissable_prefix( g->prefix ) ) {

				struct format *t
					= & s->analysis->table;
				int i = 0;
				// TODO: Following is not UTF8-aware, and the heuristic
				// TODO: should be revisted.
				while( i < MAXLEN_METADATA_PREFIX && ispunct(g->prefix[i]) ) {
					t->metadata_line_prefix[i] = g->prefix[i];
					i++;
				}
				t->metadata_line_prefix[i] = 0;
				t->metadata_line_prefix_len = i;
			}

			s->check_state = _cs_acquire_sample;
			s->state_lines = MAX_COUNT_SAMPLE_LINES - 1;

		} else
		if( s->state_lines == 0 ) {

			/**
			  * We exhausted the maximum line count without discovering a
			  * change in line prefixes that signals the end of a header,
			  * so just analyze what we got...
			  */

			const int ASTAT
				= _analyze_top_lines( s );
			if( ASTAT_CONTINUE == ASTAT )
				s->check_state = _cs_analyze_content;
			else
				return ASTAT;
		}

	} else
	if( ! _is_line_terminator_cc( s->cc_curr ) )
		sspp_push( s->lpas, s->analysis->utf8, 1 + s->suffix_len );

	return ASTAT_CONTINUE;
}


/**
  * Acquire a minimum number of (hopefully) data lines from which to
  * infer table format.
  */
static int _cs_acquire_sample( struct state *s ) {

	if( s->analysis->utf8[0] == s->final_line_separator /* we've finished another line */ ) {

		s->lines  ++;
		s->state_lines --;

		if( s->state_lines == 0 ) {
			const int ASTAT
				= _analyze_top_lines( s );
			if( ASTAT_CONTINUE == ASTAT )
				s->check_state = _cs_analyze_content;
			else
				return ASTAT;
		}

	}
	return ASTAT_CONTINUE;
}


static int _cs_analyze_content( struct state *s ) {

	if( s->analysis->utf8[0] == s->final_line_separator /* we've finished another line */ ) {

		ssize_t llen;
		s->lines  ++;

		rewind( s->cache );

		if( (llen = getdelim( &_line, &_blen, s->final_line_separator, s->cache )) > 0 )
			_analyze_line( s->analysis, _line, llen ); // ...whether empty or not!

		rewind( s->cache );
	}
	return ASTAT_CONTINUE;
}


/**
  * This is the primary entry point for tabular file analysis.
  * The <analysis> parameter should point to a zeroed struct.
  *
  * Count and classify characters and transitions between character classes
  * in a byte stream assumed to represent UTF8-encoded text.
  * Counts 6 character types: LF, CR, and non-line-terminating characters,
  * where non-line-terminating characters are further divided by their UTF-8
  * sizes 1-4. UTF-8/1 are ASCII; UTF-8/2-4 are non-ASCII--that is, multibyte
  * characters.
  *
  * terminate early
  *		file I/O error, no results					no JSON
  *		UTF-8 error, offending byte offset			JSON
  * complete file
  *		with only character content					JSON
  *		with character content and table analysis	JSON
  *
  * Returns:
  *  0 if at least some analysis was completed and output is expected to
  *    contain some valid information
  * -1 otherwise 
  */
int tabular_scan( FILE *fp, struct table_description *d /* out */ ) {

	static struct state s;
	memset( &s, 0, sizeof(s) );

	/**
	  * Verify output container is empty before allocating anything.
	  * Yes, I could simply zero it here, but it contains pointers to heap-
	  * allocated elements, the validity of which can't be ascertained here.
	  * Thus, it's caller's responsibility to insure it's initialized...and
	  * our responsibility to trust but verify.
	  */
	{
		int i = sizeof(struct table_description);
		const char *raw = (const char *)d;
		while( i-- > 0 ) {
			if( *raw++ ) {
				d->status = E_UNINITIALIZED_OUTPUT;
				return EXIT_FAILURE; // This is the ONLY early return!
			}
		}
	}

	s.cc_last = CC_COUNT;  // an invalid value
	s.cache = tmpfile();
	s.check_state = _cs_infer_lineterm;
	s.lpas = sspp_create_state( 2 );
	s.analysis = d;

	do {
		s.suffix_len = 0;
		*s.analysis->utf8 = fgetc( fp );

		if( feof(fp) )
			break;
		if( ferror(fp) ) {
			d->status = E_FILE_IO;
			goto leave;
		}

		s.nbytes ++;

		// We might consume more bytes inside switch to parse UTF8/2+.

		switch( *s.analysis->utf8 ) {
		case '\n':
			s.cc_curr = CC_LF;
			break;
		case '\r':
			s.cc_curr = CC_CR;
			break;
		default:
			s.cc_curr = CC_CHAR;
			// Set up error info speculatively.
			d->len     = 1;
			d->ordinal = s.nbytes;
			s.suffix_len = utf8_suffix_len( *s.analysis->utf8 );
			if( s.suffix_len > 0 ) {
				const int n
					= utf8_consume_suffix( s.suffix_len, fp, s.analysis->utf8+1 );
				if( n != 0 /* some kind of failure */ ) {
					if( n > 0 ) {
						d->len += n;
						d->status = E_UTF8_SUFFIX;
					} else
						d->status = E_FILE_IO;
					goto leave;
				}
				s.nbytes += s.suffix_len;
			} else
			if( s.suffix_len < 0 ) {
				d->status = E_UTF8_PREFIX;
				goto leave;
			}
		}

		/**
		  * One more character consisting of one OR MORE bytes has now been
		  * fully consumed...
		  */

		s.nchars                                                += 1;
		d->char_class_counts[ s.cc_curr + s.suffix_len ] += 1;

		if( s.nchars >= 2 /* character transitions have occurred */ )
			d->char_class_transition_matrix[ s.cc_last*CC_COARSE_COUNT + s.cc_curr ] += 1;

		if( s.cache ) {

			const int N = 1+s.suffix_len;
			if( fwrite( s.analysis->utf8, sizeof(char), N, s.cache ) != N ) {
				d->status = E_FILE_IO;
				goto leave;
			}

			/**
			  * ...then give the current analysis method a chance to act.
			  * Failure is graceful--that is, failure to ascertain table
			  * format, causes analysis to fall back to just byte content.
			  */

			if( s.check_state( &s ) != ASTAT_CONTINUE ) {
				s.check_state = NULL;
				// The cache is only for analysis. If it has failed,
				// we no longer need the cache...
				fclose( s.cache );
				s.cache = NULL;
				d->status = E_NO_TABLE;
			}
		}

		s.cc_last = s.cc_curr;

	} while( 1 );

	if( s.check_state /* If we didn't abort analysis... */ ) {
		/** ...but we didn't reach the actual analysis phase,
		  * do it now...
		  */
		if( s.check_state != _cs_analyze_content ) {
			if( _analyze_top_lines( &s ) != ASTAT_CONTINUE )
				s.check_state = NULL;
		}
	}

	if( s.check_state /* If we didn't abort analysis... */ ) {
		_analyze_columns( d );
	}

	/**
	  * Check invariants
	  */

	assert( (d->table.column_separator[0] != 0) == (d->table.column_count > 0) );
	assert( s.check_state == NULL /* we aborted analysis */ ||
			( s.lines == d->rows.empty
			  + d->rows.meta
			  + d->rows.data ) );
leave:
	if( _line )
		free( _line );

	if( s.cache )
		fclose( s.cache );

	if( s.lpas )
		sspp_destroy( s.lpas );

	// In case this is used in context of a library in which case BSS might
	// NOT be reinitialized before another call to scan...
	_line = NULL;
	_blen = 0;

	return d->status < E_FILE_IO ? EXIT_SUCCESS : EXIT_FAILURE;
}


int tabular_error( struct table_description *d, int len, char *buf ) {
	int n = 0;
	const char *template = _error_template[ d->status ];
	switch( d->status ) {
	// Cases are only for purpose of supplying analysis state parameters
	// to error message template, but (TODO) currently none are defined.
	case E_COMPLETE:
	case E_NO_TABLE:
	case E_UTF8_PREFIX:
	case E_UTF8_SUFFIX:
	case E_FILE_IO:
	case E_UNINITIALIZED_OUTPUT:
		n = snprintf( buf, len, "%s", template );
		break;
	default:
		;
	}
	return n;
}

