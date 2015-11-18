
#include <math.h>

#include "quantile.h"
#include "quicksel.h"

/**
  * Just one of many ways of computing a quantile from a discrete set.
  * This corresponds to R's default method.
  */
double quantile( double *x, int n, double p ) {
	const double index = (n-1)*p;
	const double lo = floor( index );
	const double hi =  ceil( index );
	double xlo = qselect( x, n, (int)lo );
	double xhi = qselect( x, n, (int)hi );
	double Q = xlo;
	if( index > lo ) {
		double h = index - lo;
		Q = (1-h)*Q + h*xhi;
	}
	return Q;
}

