
/**
  * A tabular file contains at most two types of lines:
  * 1. header/metadata lines
  * 2. data lines
  * The header/metadata lines may be absent. Header lines occur as the
  * earliest non-empty lines of the file by definition. Metadata lines
  * may appear anywhere, but are assumed to be distinguished exactly as
  * header lines--that is, by the same distinguishing prefix.
  *
  * Although header lines are frequently internally delimited exactly as
  * data lines (because they are literally column headers), they may also
  * be free formatted, giving information for interpretation of data lines.
  * Thus, no assumptions are made concerning content of header and metadata
  * lines.
  *
  * Data lines are assumed to all contain the same number of fields
  * separated and/or delimited in the same way.
  *
  * The basic strategy followed in this code is
  * 1. create a histogram of each line's content
  * 2. perform targeted searches for multi-character patterns (e.g. / +/).
  * 3. analyze a collection of histograms for identity of counts
  * 4. field delimiters are those candidate characters (or multi-character
  *    sequences) that occur with identical frequency in data lines.
  *
  * The histogramming as currently implemented ought to be generalized to
  * handle Unicode.
  *
  * Highly structured data may actually make use of multiple separators--
  * that is, fields can have sub-fields--leading to different counts for
  * each separator, but a given separator may have the same count on
  * multiple lines. In this case:
  * 1. Whitespace separation, whether by TABs or space clusters, always take
  *    priority since whitspace is never used--at least, I've never seen
  *    whitespace used--as an internal separator of subfields.
  * 2. Space clusters SHOULD never co-occur with TABs, so the question of
  *    precedence of TABs versus space clusters should never arise.
  * 3. Assuming the first field (often a row identifier) rarely contains
  *    subfields then the first separator on the line is likely the primary
  *    field separator when multiple candidate graphical separators exist.
  * 4. May want to explicitly treat ASCII chars 0x1D(GS), 0x1E(RS), 0x1F(US)
  *    though I've never seen them used.
  */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>  // for fields.h
#include <string.h>
#include <sys/types.h>
#include <ctype.h>
#include <assert.h>
#ifdef HAVE_QUOTE_COMMA_QUOTE
#include <regex.h>
#include <alloca.h>
#endif

#include "tabular.h"
#include "rstrip.h"

#ifdef HAVE_QUOTE_COMMA_QUOTE

/**
  * One or (ideally) more of these patterns in a line of text indicates
  * probable "comma-separated-values" formatting...which is actually
  * comma-separated and quote-delimited values.
  * Merely comma separated will be detected by other code.
  */
static const char *_QCQ_PATTERN = "\" *, *\"";
static regex_t _re_QCQ;

static void _fini_re() {
	regfree( &_re_QCQ );
}

static int _init_re() {

	const int regerr
		= regcomp( &_re_QCQ, _QCQ_PATTERN,
			REG_EXTENDED | REG_NEWLINE );
	if( regerr ) {
		const size_t required 
			= regerror( regerr, &_re_QCQ, NULL, 0 );
		char *buf 
			= alloca( required );
		regerror( regerr, &_re_QCQ, buf, required );
		fprintf( stderr, "%s\n", buf );
		return -1;
	}
	atexit( _fini_re );
	return 0;
}


/**
  * Count quote-comma-quote patterns in the line.
  * These patterns are separators between fields and are typical in comma-
  * separated-value (CSV) files. That is, CSV often implies "quote-
  * delimited" comma-separated values.
  *
  * If the fields are truly *only* comma separated, that will be detected
  * elsewhere.
  */
static int _id_qcq( const char *line, const int N, unsigned *count ) {

	int offset = 0;
	regmatch_t m;

	//puts( line );

	if( _QCQ_PATTERN /* just-in-time compilation of regex pattern */ ) {
		if( _init_re() )
			return -1;
		_QCQ_PATTERN = NULL; // we only needed it once!
	}

	/**
	  * Implementation note: One could have regexec detect all instances of
	  * the pattern, but we don't know a priori how many might be present.
	  * It's safe to just scan the line detecting one at a time, and it
	  * obviates buffer allocation.
	  */

	while( offset < N && regexec( &_re_QCQ, line + offset, 1, &m, 0 ) != REG_NOMATCH ) {
		*count += 1;
		offset += m.rm_eo;
	}
	return 0;
}

#define P_QCQ           (129)
#define SEPARATOR_COUNT (130)

#else

#define SEPARATOR_COUNT (129)

#endif

#define P_SPC_GROUP     (128)

/**
  * This is currently ASCII-only.
  * Wouldn't be *too* hard to generalize it to UTF-8/Unicode by
  * 1. generalization character enumeration and 
  * 2. consulting Unicode character category database.
  * Main challenge would be infeasibility of a full Unicode char
  * histogram as is feasible with ASCII.
  */
int _count_candidate_separators( const char *line, const int len, unsigned *count ) {

	const char *pc = line;
	int last = 0;

#ifdef HAVE_QUOTE_COMMA_QUOTE
	if( _id_qcq( line, len, count + P_QCQ ) )
		return -1;
#endif

	while( *pc ) {

		const int C = *pc++;

		// Currently this is just blindly counting ALL ASCII characters for
		// simplicity even though only a handful of characters (control and
		// punctuation) are actually expected to be separator candidates.

		count[ C & 0x7F ] += 1;

		// Transitions from space to non-space signal ends of / +/ patterns.
		// No need to use the regular expression API for these; it is simple
		// to count transitions.

		if( last == 0x20 && C != 0x20 ) count[ P_SPC_GROUP ] += 1;

		last = C;
	}
	if( last == 0x20 )
		count[ P_SPC_GROUP ] += 1;

	return 0;
}


static int _split_line_simple_sep( char *pc, struct format *pf, 
		FIELD_PARSER process_field, void *context ) {

	const int SEP = pf->column_separator[0];
	int foff = 0;
	const char *token;
	bool eol = (*pc == 0);

	assert( SEP /* must not be the NUL terminator. */ );

	while( ! eol ) {

		/**
		  * Initialize the current field...
		  */
		token = pc;

		/**
		  * ...and NUL terminate it...
		  */
		while( *pc && (*pc != SEP) ) pc++;

		if( *pc /* ...if its end does not coincide with line's end... */ )
			*pc++ = '\0';
		else
			eol = true;

		/**
		  * ...before sending it to the field processor.
		  * Don't process excess fields (pf->column_count column data
		  * accumulators have already been allocated).
		  */
		if( foff < pf->column_count )
			process_field( token, foff, context );

		foff++;
	}
	return foff;
}


static int _split_line_coalesce_ws( char *line, struct format *pf, 
		FIELD_PARSER process_field, void *arg ) {
	fputs( "TODO: unimplemented: parsing of coalescable whitespace.\n", stderr ); abort();
	return -1;
}

/*
static int _split_line_csv( char *line, struct format *pf, 
		FIELD_PARSER process_field, void *arg ) {
	fputs( "TODO: unimplemented: parsing of RFC4180 CSV files.\n", stderr ); abort();
	return -1;
}
*/

/**
  * Localizes the decision making re: what constitutes an admissable
  * separator.
  */
static bool _is_admissable_separator( int c ) {
	return ( ! isalnum(c) );
}


/**
  * The delimiter character (for the purposes of getdelim) is the *last*
  * character in the (possibly multi-character) sequence line-termination
  * sequence. 
  */
int _format_infer( FILE *fp, int delim, int nlines, struct format *table ) {

	int status = -1; // Assume failure until success below...

	/**
	  * We ping-pong between two buffers when reading successive lines in
	  * order to ALWAYS preserve the last processed DATA line. Otherwise,
	  * fseek'ing backward in fp might be required which would preclude fp
	  * being a pipe...something we don't want to preclude.
	  */
	char *lines[2]
		= { NULL, NULL };
	size_t blen[2]
		= { 0, 0 };
	int buffer_selector = 0; // selects one of previous two buffers

	ssize_t llen = 0;

	unsigned *reference
		= calloc( 2*SEPARATOR_COUNT, sizeof(unsigned) );
	unsigned *charcount
		= reference + SEPARATOR_COUNT;
	unsigned candidate_count = 0; // ...only on 1st and possibly final iter.

	// Just verify caller has initialized.

	assert( strlen( table->column_separator) == 0 );
	assert( table->column_count == 0 );
	assert( table->data_lines_sampled == 0 );
	assert( table->split_line == NULL );
	assert( ! table->column_separator_is_regex );

	while( nlines > 0 
		&& (llen = getdelim( lines + buffer_selector, blen + buffer_selector, delim, fp )) > 0 ) {

		char *const LINE = lines[buffer_selector]; // TODO: Verify ping-pong behavior!

		nlines --;

		/**
		  * Skip metadata lines
		  */

		if( table->metadata_line_prefix_len > 0 // ...iff a metadata prefix is defined.
				&& memcmp( LINE, table->metadata_line_prefix, table->metadata_line_prefix_len ) == 0 )
			continue;

		/**
		  * Replace line terminator(s) with NULs.
		  */

		if( (llen = rstrip( LINE, llen )) == 0 /* line was empty! */ )
			continue;

		buffer_selector = 1 - buffer_selector;
		table->data_lines_sampled += 1;

		memset( charcount, 0, SEPARATOR_COUNT*sizeof(unsigned) );
		_count_candidate_separators( LINE, llen, charcount );

		/**
		  * Following is the core algorithm.
		  */

		if( candidate_count == 0 /* The first line's... */ ) {
			/* ...non-zero separator counts DEFINE the reference. */

			int i = SEPARATOR_COUNT;
			while( i-- > 0 )
				reference[i] = charcount[i];
			candidate_count = SEPARATOR_COUNT; // Not really, just need > 0

		} else { /* Subsequently, separators REMAIN candidates only as... */
			/* ...long as every line's counts equal the reference counts. */

			int i = SEPARATOR_COUNT;
			candidate_count = 0;
			while( i-- > 0 ) {
				if( reference[i] > 0 /* Is sep_i still a candidate? */ ) {
					if( charcount[i] == reference[i] )
						candidate_count += 1;
					else
						reference[i] = 0; // sep_i is no longer a candidate.
				}
			}
			if( candidate_count < 2 )
				break; // early exit
		}
	}

	/**
	  * If exactly 1 candidate remains, it wins if it's admissable.
	  * If multiple candidates remain, choose the winner based on
	  * precendence rules and heuristics described at top.
	  */

	if( candidate_count == 1  ) {

		int c = SEPARATOR_COUNT;
		// Find the non-zero count, and...
		while( c-- > 0 ) { if( reference[c] > 0 ) break; }
		// ...if it's an admissable separator, make it official!
		if( c > 0 && _is_admissable_separator(c) ) {
			table->column_separator[0] = c;
			table->column_count = reference[c] + 1;
			table->split_line = _split_line_simple_sep;
			status = 0;
		}

	} else
	if( candidate_count > 1 ) {

		// Whitespace separation takes priority.

		if( reference[ P_SPC_GROUP ] > 0 ) {
			strcpy( table->column_separator, " +" );
			table->column_count = reference[ P_SPC_GROUP ] + 1;
			table->split_line = _split_line_coalesce_ws;
			table->column_separator_is_regex = true;
			status = 0;
		} else
		if( reference[ '\t' ] > 0 ) {
			table->column_separator[0] = '\t';
			table->column_count = reference[ '\t' ] + 1;
			table->split_line = _split_line_simple_sep;
			status = 0;

		} else { // ...it's a non-whitespace pattern.

			int n = 0;
			// Reuse charcount buffer for a different purpose now.
			char *candidate_seps
				= (char*)charcount;
			const char *line
				= lines[ 1-buffer_selector ]; // the last examined line
			// Make a list of candidate separator characters.
			for(int c = 0; c < 127 /* ASCII DEL */ ; c++ ) {
				if( _is_admissable_separator(c) && reference[c] > 0 )
					candidate_seps[n++] = c;
			}
			candidate_seps[n] = '\0'; // Terminate the char[]
			// Find 1st candidate separator on the last processed data line.
			if( n > 0 ) {
				size_t sep_offset = strcspn( line, candidate_seps );
				const int C = line[sep_offset];
				assert( strchr( candidate_seps, C ));
				table->column_separator[0] = C;
				table->column_count = reference[ C ] + 1;
				table->split_line = _split_line_simple_sep;
				status = 0;
			}
		}
	}

	/**
	  * Clean up.
	  */

	if( lines[0] )
		free( lines[0] );
	if( lines[1] )
		free( lines[1] );
	if( reference )
		free( reference );

	assert( status != 0 || table->column_count > 0 );
	return status;
}


#ifdef UNIT_TEST_FORMAT

#include <err.h>

static void _dump_format( struct format * s, FILE *fp ) {
	fprintf( fp,
		"metadata_line_prefix: %s (%d)\n"
		"         separator: \"%s\"\n"
		"      column_count: %d\n"
		"data_lines_sampled: %d\n",
	s->metadata_line_prefix, s->metadata_line_prefix_len,
	s->column_separator, 
	s->column_count,
	s->data_lines_sampled );
}

int main( int argc, char *argv[] ) {

	struct format table;
	const int NLINES
		= argc > 2
		? atoi( argv[2] )
		: 0;
	const char *FNAME
		= argc > 3
		? argv[3]
		: NULL;
	FILE *fp
		= FNAME
		? fopen( FNAME, "r" )
		: stdin;

	memset( &table, 0, sizeof(table) );
	table.metadata_line_prefix_len = strlen( argv[1] );

	if( ! ( table.metadata_line_prefix_len < sizeof(table.metadata_line_prefix) ) )
		errx( -1, "metadata prefix (\"%s\") too big", argv[1] );
	else
		strncpy( table.metadata_line_prefix, argv[1], sizeof(table.metadata_line_prefix) );

	if( argc < 3 )
		err( -1, "%s <metadata prefix> <nlines> [ <file> ]", argv[0] );

	if( fp == NULL )
		err( -1, "opening %s", FNAME );

	if( format_infer( fp, "\n", NLINES, &table ) == 0 )
		_dump_format( &table, stdout );

	return EXIT_SUCCESS;
}
#endif

