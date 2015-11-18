
#include <assert.h>

#include "bounds.h"

void bounds( const double *x, int n, double mm[] ) {

	// If there is at least one element...

	if( n-- > 0 ) {

		// ...initialize results from it, and "mark it" as used.

		double m = *x;
		double M = *x++;

		// Continue only as long as there are more elements.

		assert( m < M || m == M );

		while( n-- > 0 ) {
			const double D = *x++;
			if( m > D )
				m = D;
			else // "else" justified by the assert above.
			if( M < D )
				M = D;
		}

		// Output is produce IFF there was at least one element.

		mm[0] = m;
		mm[1] = M;
	}
}

#ifdef UNIT_TEST_BOUNDS
#include <stdlib.h>
#include <stdio.h>
#include <alloca.h>
int main( int argc, char *argv[] ) {
	const int N = argc - 1;
	double *data = alloca( N*sizeof(double) );
	double minmax[2];
	for(int i = 0; i < N; i++ ) {
		data[i] = atof( argv[i+1] );
	}
	bounds( data, N, minmax );
	printf( "[%f, %f] (%d pts)\n", minmax[0], minmax[1], N );
	return EXIT_SUCCESS;
}
#endif

