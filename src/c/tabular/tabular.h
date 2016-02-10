
/**
  * This header constitutes the full public interface to table analysis.
  */

#ifndef _tabular_h_
#define _tabular_h_

/**
  * Forward decls
  */
struct column;

/**
  * These character class labels are used to index two histograms:
  * 1. a simple linear histogram that exactly reflects these labels, and
  * 2. a 3x3 transition matrix THAT ONLY MAKES USE OF THE FIRST THREE
  *    LABELS. In this case, [CC_CHAR..CC_UTF8_4] all map onto CC_CHAR.
  */
enum {
	CC_LF = 0,
	CC_CR,
	CC_CHAR,   // Any ASCII character except { LF, CR }
	CC_ASCII  = CC_CHAR,  // ASCII is synonym for CC_CHAR
	CC_UTF8_1 = CC_ASCII, // CC_UTF8_1 is synonym for CC_ASCII
	CC_COARSE_COUNT,
	CC_UTF8_2 = CC_COARSE_COUNT,
	CC_UTF8_3,
	CC_UTF8_4,
	CC_COUNT
};

extern const char *CC_NAME[ CC_COUNT ];

enum TabularStatus {

	E_COMPLETE = 0,
	/**
	  * Analysis completed fully.
	  * Character content AND table analysis is valid.
	  */

	E_NO_TABLE,
	/**
	  * Analysis of character content completed, but no table was detected.
	  * Only character content analysis is valid.
	  */

	E_UTF8_PREFIX,
	/**
	  * Analysis terminated early for non UTF-8 byte sequence.
	  * Implies file is binary, not text.
	  * Only offset of offending byte is valid.
	  */

	E_UTF8_SUFFIX,
	/**
	  * Analysis terminated early for non UTF-8 byte sequence.
	  * Implies file is binary, not text.
	  * Only offset of offending byte is valid.
	  */

	E_FILE_IO,
	/**
	  * Analysis terminated early for non content-related error.
	  * Nothing in analysis is completely valid.
	  */

	E_UNINITIALIZED_OUTPUT,
	/**
	  * The output container (a struct table_description) was not zeroed.
	  */

	E_COUNT
};


/**
  * We currently assume tables in which rows are objects and columns are
  * attributes. This implies each column is a vector of a discernible type,
  * but rows may be mixed.
  * TODO: As part of the analysis of the file's initial lines we ought to
  * infer whether the above *or its inverse* applies. 
  */

struct format; // ...forward decl for following type decls.
typedef void (*FIELD_PARSER)( const char *field, int offset, void *context );
typedef int (*LINE_SPLITTER)( char *pc, struct format *ps, FIELD_PARSER, void *context );

/**
  * There are only three (supported) possibilities for parsing fields from
  * lines:
  * 1. single ASCII character separators (e.g. tab, comma)
  * 2. whitespace clusters (described by the regex / +/)
  *    These are explicitly "coalescing;" single whitespace separators
  *    may also satisfy #1.
  * 3. CSV as defined by RFC4180.
  */
struct format {

#define MAXLEN_METADATA_PREFIX (7)

	/**
	  * UTF-8 character believed to indicate header/metadata/comment rows.
	  * We don't try to distinguish between them.
	  * A line beginning with this sequence is not parsed.
	  */
	char metadata_line_prefix[MAXLEN_METADATA_PREFIX+1];
	char metadata_line_prefix_len;

	/**
	  * Usually only the first character of this buffer is used.
	  */
	char column_separator[8];
	bool column_separator_is_regex;

	/**
	  * The fields per line.
	  */
	unsigned column_count;

	/**
	  * Number of (data) lines that contributed to the inference.
	  */
	unsigned data_lines_sampled;

	/**
	  * The function selected during scanning to chop up a data line
	  * into a series of NUL-terminated tokens.
	  */
	LINE_SPLITTER split_line;
};


/**
  * Every row is classified as
  * 1. empty
  * 2. metadata, header, or comment
  * 3. data
  */
struct table_description {

	/**
	  * One of enum TabularStatus codes.
	  */
	long status;

	/**
	  * Holds the most recently consumed UTF8 character
	  * WITHOUT NUL-termination! (TODO verify)
	  */
	char utf8[8];

	/**
	  * The number of valid bytes in utf8.
	  */
	int len;

	/**
	  * The 1-based position in the input file for the first byte in utf8.
	  */
	long ordinal;

	/**
	  * A histogram of the 6 character classes:
	  * { LF, CR, ASCII(UTF8/1), UTF8/2, UTF8/3, UTF8/4 }
	  */
	unsigned long char_class_counts[6];

	/**
	  * A 3x3 row-major matrix giving the count of transitions between the
	  * 3 "coarse" character classes.
	  *       LF   CR CHAR
	  *   LF   0    1    2
	  *   CR   3    4    5
	  * CHAR   6    7    8
	  */
	unsigned long char_class_transition_matrix[9];

	/**
	  * The information for how to parse a row inferred from the initial
	  * data lines of the file.
	  */
	struct format table;

	struct counts {
		unsigned empty; // contain ONLY line terminator(s)
		unsigned meta;  // lines prefixed by metadata_line_prefix
		unsigned data;  // all lines not matching one of above.
		// INVARIANT: sum of above == total line count
		unsigned aberrant; // *data* rows with any parse failure
		// INVARIANT: aberrant <= data
	} rows;

	/**
	  * This member not only contains column summaries, it also serves
	  * as an indicator of whether or not table analysis succeeded.
	  * Typically callers will hold a <struct table_description> on the stack,
	  * but this one member is dynamically allocated and must be freed!
	  */
	struct column *column;
};

/**
  * 
  */
int tabular_scan( FILE *fp, struct table_description *summary );

/**
  * Even though struct table_description may be allocated on the stack, the
  * struct contains dynamically-allocated members which require cleanup!
  */
void tabular_free( struct table_description *a );

/**
  * If <len> == 0 and buf is NULL this returns the size of the buffer
  * required to contain the full error message. (See snprintf.)
  */
int tabular_error( struct table_description *analysis, int len, char *buf );

/**
  * Write a table description as JSON.
  */
int tabular_as_json( const struct table_description *a, FILE *fp );

#endif

