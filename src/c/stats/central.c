
#include <math.h>

#include "central.h"

double mean( const double *x, int n ) {
	long double m = 0;
	for(int i = 0; i < n; i++)
		m += (*x++ - m) / (i + 1.0);
	return m;
}

double variance( const double *x, int n, double mean ) {
	long double v = 0.0;
	for(int i = 0; i < n; i++) {
		const long double delta = *x++ - mean;
		v += (delta * delta - v) / (i + 1.0);
	}
	return v;
}

double sd( const double *x, int n ) {
	return sqrt( variance( x, n, mean( x, n ) ) );
}

