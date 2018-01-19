
/**
  * Constants in this module parameterize heuristic judgements about
  * data and (may be) overridable by environment variables.
  */

#include <stdlib.h>
#include "environ.h"

int MAX_CATEGORY_CARDINALITY       = 32;
int MAX_ABSOLUTE_CATEGORICAL_VALUE = 16;
int MAXLEN_CATEGORY_LABEL          = 63;

void read_environment_overrides() {
	if( getenv("MAX_CATEGORY_CARDINALITY") )
		MAX_CATEGORY_CARDINALITY
			= atoi(getenv("MAX_CATEGORY_CARDINALITY"));
	if( getenv("MAXLEN_CATEGORY_LABEL") )
		MAXLEN_CATEGORY_LABEL
			= atoi(getenv("MAXLEN_CATEGORY_LABEL"));
	if( getenv("MAX_ABSOLUTE_CATEGORICAL_VALUE") )
		MAX_ABSOLUTE_CATEGORICAL_VALUE
			= atoi(getenv("MAX_ABSOLUTE_CATEGORICAL_VALUE"));
}

