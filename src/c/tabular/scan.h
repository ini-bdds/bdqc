
#ifndef _scan_h_
#define _scan_h_

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

enum {
	E_COMPLETE = 0,
	/**
	  * Analysis completed fully.
	  * Character content AND table analysis is valid.
	  */
	E_NO_TABLE,
	/**
	  * Analysis completed on character content, but no table was detected.
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
	E_COUNT
};

extern const char *error_string[ E_COUNT ];

/**
  * We currently assume tables in which rows are objects and columns are
  * attributes. This implies each column is a vector of a discernible type,
  * but rows may be mixed.
  * TODO: As part of the analysis of the file's initial lines we ought to
  * infer whether the above *or its inverse* applies. 
  */

enum FIELD_TYPE {
	FTY_EMPTY,
	FTY_INTEGER,
	FTY_FLOAT,
	FTY_STRING,
	FTY_COUNT
};

enum ROW_TYPE {
	RTY_EMPTY,
	RTY_META, // or header or comment, we don't distinguish
	RTY_DATA
};


struct column {

	/**
	  * Number of types each type has been encountered in the column.
	  */
	int type_vote[ FTY_COUNT ];

	/**
	  * These are computed on all numeric values (integer or float).
	  * Integers are coerced to doubles. The sample count is
	  *
	  *			type_vote[ FTY_INTEGER ] + type_vote[ FTY_FLOAT ]
	  *
	  * Computations are recursive, so should be numerically stable.
	  */
	double statistics[2]; // mean, variance

	/**
	  * The label set is create just-in-time when a field is
	  * encountered that can't be interpreted as numeric.
	  */
	void *label_set;
	bool  label_set_exhausted;
};

/**
  * Every row is classified as
  * 1. empty
  * 2. metadata, header, or comment
  * 3. data
  */
struct file_analysis {

	/**
	  * Holds the up to four bytes of a UTF8 character WITHOUT NUL-
	  * terimination!
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

	/**
	  * Number of empty lines ("empty" means containing only line
	  * terminators).
	  */
	unsigned empty_rows;

	/**
	  * INVARIANT: meta_rows + data_rows + empty_rows == total lines.
	  */
	unsigned meta_rows;

	/**
	  * We only distinguish two types of rows: data, non-data. The non-data
	  * are implicitly total row count (available from 
	  * char_class_transition_matrix) minus data_rows.
	  */
	unsigned data_rows;

	/**
	  * Number of *data* rows for which parsing failed for any reason.
	  * INVARIANT: .aberrant_rows <= .data_rows
	  * (...though this might be transiently violated in _analyze_row
	  *  because of order of operations.)
	  */
	unsigned aberrant_rows;

	/**
	  * This member not only contains column summaries, it also serves
	  * as an indicator of whether or not table analysis succeeded.
	  * Typically callers will hold a <struct file_analysis> on the stack,
	  * but this one member is dynamically allocated and must be freed!
	  */
	struct column *column;
};

void scan_pretty_print( struct file_analysis *fs, FILE *fp );

/**
  * 
  */
long scan( FILE *fp, struct file_analysis *summary );
/**
  * Even though struct file_analysis may be allocated on the stack, the
  * struct contains dynamically-allocated members which require cleanup!
  */
void fini_analysis( struct file_analysis *a );

#endif

