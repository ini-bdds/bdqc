
#include <math.h>
#include <assert.h>

#include "interp.h"

static inline double _linterp( 
		const double x,
		const double lx, const double ux,
		const double ly, const double uy ) {

	double m;
	assert( lx < ux );
	assert( lx <= x && x <= ux );
	m = (uy-ly)/(ux-lx);
	return ly + m*(x-lx);
}


void linterp(
		const double *rx, const double *ry, int rn,
		const double *x, double *y, int n ) {

	int ri = 0; // Reference and Interpolation Indices

	/**
	  * Verify it's really only an INTERpolation. Extrapolation is easy
	  * but it requires commitment to a type of extrapolation.
	  */
	assert( *rx    <= *x );
	assert( x[n-1] <= rx[rn-1] );

	/**
	  * Verify reference points are ordered by x-coordinate.
	  */
#ifdef _DEBUG
	for(int i = 1; i < rn; i++ ) assert( rx[i-1] <= rx[i] );
#endif

	/**
	  * For each new point
	  */
	for(int ii = 0; ii < n; ii++ ) {

		/**
		  * Insure we're working with the glb of x[ii] among reference points.
		  */
		while( ri+1 < rn && rx[ri+1] <= x[ii] )
			ri++;

		if( ! ( rx[ri] <= x[ii] ) ) break;

		/**
		  * Because rx[ri] is GLB of x[ii] and
		  *
		  *         min(rx) <= min(x) <= max(x) <= max(rx)
		  *
		  * ...there must be a point of rx to the right of x[ii].
		  */

		assert( ri+1 < rn && x[ii] <= rx[ri+1] );
		y[ii] = _linterp( x[ii],
			rx[ri], rx[ri+1],
			ry[ri], ry[ri+1] );
	}
}

