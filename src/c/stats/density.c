
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <complex.h>

#include "bounds.h"
#include "fft.h"
#include "quantile.h"
#include "central.h"
#include "gaussian.h"
#include "interp.h"

/**
  * The bw.nrd0 method of R.
  * function (x) {
  *     if (length(x) < 2L) 
  *         stop("need at least 2 data points")
  *     hi <- sd(x)
  *     if (!(lo <- min(hi, IQR(x)/1.34))) 
  *         (lo <- hi) || (lo <- abs(x[1L])) || (lo <- 1)
  *     0.9 * lo * length(x)^(-0.2)
  * }
  */
double bw( double *x, int n ) {
	assert( n >= 2 );
	const double SD = sd( x, n );
	const double IQRratio
		= ( quantile( x, n, 0.75 )
		-   quantile( x, n, 0.75 ) )/1.34;
	double lo = SD < IQRratio ? SD : IQRratio;

	if( lo == 0.0 ) {
		if( SD != 0 ) 
			lo = SD;
		else
		if( fabs(x[0]) != 0.0 )
			lo = fabs(x[0]);
		else
			lo = 1.0;
	}
	return 0.9 * lo * pow( (double)n, -0.2 );
}


static void _BinDist( const double *x, int nx,
		double xlo, double xhi, unsigned int n, complex *y ) {

	const double W = 1.0/(double)nx;

    int ixmin = 0, ixmax = n - 2;
    double xdelta = (xhi - xlo) / (n - 1);

	for(int i = 0; i < nx ; i++) {
		if( isfinite(x[i]) ) {
			double xpos = (x[i] - xlo) / xdelta;
			int ix = (int) floor(xpos);
			double fx = xpos - ix;
			if(ixmin <= ix && ix <= ixmax) {
				y[ix] += (1 - fx) * W;
				y[ix + 1] += fx * W;
			} else
			if(ix == -1)
				y[0] += fx * W;
			else
			if(ix == ixmax + 1)
				y[ix] += (1 - fx) * W;
		}
	}
}


/**
  * Calculate a Gaussian kernel density estimate for the given
  * point set.
  *
  * This was motivated by the need to produce simple, small-ish
  * SVG plots for use in HTML...nothing too fancy. For this
  * reason, it elides lots of potentially useful options. Goal
  * is to be as simple as possible for a specific purpose and
  * no simpler...
  * - The only kernel function supported is the Gaussian.
  * - The bandwidth calculation is a fixed function.
  * - A single resolution is used for the kernel convolution: 512.
  */

#define K (512)

void gkde( double *x, const int NX,
		double *xd, double *yd, int nd ) {

	double delta, *pd, *xords;
	double minmax[2];
	double LO, UP, SPAN, DELTA;

	const double BW = bw( x, NX );

	complex *ky = calloc( 2*K, sizeof(complex) );
    complex *hy = calloc( 2*K, sizeof(complex) );
	double *kords = NULL;

	assert( nd == K );

	/**
	  * Determine the bounds of the data and calculate the plot bounds from
	  * the data bounds.
	  */

	bounds( x, NX, minmax );
	const double from = minmax[0] - 3.0*BW;
	const double to   = minmax[1] + 3.0*BW;
	LO = from - 4.0*BW;
	UP = to   + 4.0*BW;
	SPAN = UP-LO;
	DELTA = 2.0 * SPAN / (2*K-1.0);

	/**
	  * Build a histogram 
	  */
	_BinDist( x, NX, LO, UP, K, hy );

	fft_fwd( hy, 2*K );

	/**
	  * Create the kernel function on an array of the form
	  *
	  *     [ 0, a, b, c, d, e, -d, -c, -b, -a ]
	  */

	ky[0] = 0.0;
	for(int i = 1; i <= K; i++ )
		ky[i] = ky[i-1]+DELTA;
	for(int i = 0; i < K-1; i++ )
		ky[K+1+i] = -ky[K-1-i];
	for(int i = 0; i < 2*K; i++ )
		ky[i] = gaussian( ky[i], BW );

	fft_fwd( ky, 2*K ); // Fourier transform it for convolution.

	// Convolve my multiplying (with conjugation)...
	for(int i = 0; i < 2*K; i++ )
		ky[i] = conj(ky[i]) * hy[i];

	free( hy );

	/**
	  * Normalize the inverse, extract the real parts and clamp
	  * to be non-negative.
	  */
	kords = calloc( K, sizeof(double) );

	fft_inv( ky, 2*K, 1.0 / (2*K) ); // Final inverse FFT.

	// Normalize inverse FFT and extract the real part for the answer.

	for(int i = 0; i < K; i++ ) {
		kords[i] = creal(ky[i]);
		kords[i] = kords[i] < 0.0 ? 0.0 : kords[i];
	}

	free( ky );

	xords = calloc( K, sizeof(double));

	// Set up the xords paralleling kords
	delta = (UP-LO)/(K-1);
	pd = xords;
	pd[0] = LO;
	for(int i = 1; i < K; i++ ) pd[i] = pd[i-1] + delta;

	// Set up the output's abcissas (which might not be K points).
	delta = (to-from)/(K-1);
	xd[0] = from;
	for(int i = 1; i < K; i++ ) xd[i] = xd[i-1] + delta;

	linterp( xords, kords, K, xd, yd, K );
	free( xords );

	free( kords );
}

