
#ifndef _column_h_
#define _column_h_

enum STATISTICAL_CLASS {
	STC_UNK = 0, // UNKnown
	STC_CAT,     // CATegorical
	STC_QUA,     // QUAntitative
	STC_ORD,     // ORDinal
	STC_COUNT
};

enum FIELD_TYPE {
	FTY_EMPTY = 0,
	FTY_STRING,
	FTY_INTEGER,
	FTY_FLOAT,
	FTY_COUNT
};

enum ROW_TYPE {
	RTY_EMPTY = 0,
	RTY_META, // or header or comment, we don't distinguish
	RTY_DATA
};


/**
  * The column structure accumulates statistics and otherwise
  * observations about the values in a column in lieu of actually
  * storing and inspecting them exhaustively. We hope to infer
  * the statistical class of data in a column from these
  * observations, though this relies heavily on HEURISTICS.
  */
struct column {

	/**
	  * The statistical class of the column, inferred from data content.
	  * Heuristic inference of this class is the primary purpose of all
	  * the following fields, though many of the other fields may be
	  * useful on their own downstream.
	  */
	int stat_class;

	/**
	  * Number of types each type has been encountered in the column.
	  */
	int type_vote[ FTY_COUNT ];

	/**
	  * The length of the longest field (treated as a string) in the
	  * column.
	  * Excessively long fields argue against any standard statistical
	  * class and may even suggest format hasn't been correctly
	  * ascertained (or corruption has occurred).
	  */
	unsigned int max_field_len;
	unsigned int long_field_count;

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
	  * The min and max observed values of all numeric fields.
	  */
	double extrema[2];

	/**
	  * Presence of negative numbers argues against the categorical
	  * statistical class...unless the value set is very small.
	  */
	bool has_negative_integers;

	/**
	  * Integers of multiple magnitudes strongly argue against the
	  * categorical statistical class...
	  * Bits are set by floorf(log10f(1.0+abs(value)))
	  */
	unsigned int integer_magnitudes;

	/**
	  * Assuming that the column is categorical, this set accumulates
	  * the "labels" or "levels" of the category. All values:
	  * 1. not "obviously" representing floating-point numbers
	  * 2. that are sufficiently short (treated as strings) to
	  *    constitute a (probably human-given) "label"
	  * ...are cache in this set as the table is scanned.
	  */
	struct strset value_set;

	/**
	  * Indicates whether we encountered at least one more distinct value
	  * than the maximum allowed cardinality of a categorical variable's
	  * label set.
	  * This is used as a bool and as an integer table offset:
	  *  0 => max cardinality was not exceeded
	  * >0 => the table line (0-based) on which max cardinality was
	  *       exceeded (which is necessarily >= MAX_CATEGORY_CARDINALITY)
	  */
	int excess_values;
};

int  init_column_analysis( void );
void fini_column_analysis( void );
void analyze_column( struct column *c );

#endif

