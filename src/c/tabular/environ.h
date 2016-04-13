
#ifndef _environ_h_
#define _environ_h_

/**
  * Following apply to potentially categorical variables of integer type.
  */
extern int MAX_CATEGORY_CARDINALITY;
extern int MAX_ABSOLUTE_CATEGORICAL_VALUE;

/**
  * Following apply to potentially categorical variables of string type.
  */
extern int MAXLEN_CATEGORY_LABEL;

void read_environment_overrides( void );

#endif

