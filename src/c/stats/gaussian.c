
#include <math.h>

#include "gaussian.h"

#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif

double gaussian( const double x, const double sigma ) {
	double u = x / fabs( sigma );
	return (1.0 / (sqrt( 2.0 * M_PI ) * fabs( sigma ))) * exp( -u * u / 2.0 );
}

