
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


struct column {

	/**
	  * Number of types each type has been encountered in the column.
	  */
	int type_vote[ FTY_COUNT ];

	/**
	  * The statistical class of the column, inferred from data content.
	  */
	int stat_class;

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
	  * The min and max observed values.
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
	  * Assuming that the column is categorical
	  */
	struct strset value_set;

	/**
	  * Indicates whether we encountered at least one more distinct value
	  * than the maximum allowed cardinality of a categorical variable's
	  * label set.
	  */
	bool excess_values;
};

#endif

