
/**
  *
  * From https://en.wikipedia.org/wiki/Medcouple
  *
  * function naïve_medcouple(vector X):
  *     // X is a vector of size n.
  *     
  *     //Sorting in decreasing order can be done in-place in O(n log n) time
  *     sort_decreasing(X)
  *     
  *     xm := median(X)
  *     xscale := 2*max(X)
  *     
  *     // define the upper and lower centred and rescaled vectors
  *     // they inherit X's own decreasing sorting
  *     Zplus  := [(x - xm)/xscale | x in X such that x >= xm]
  *     Zminus := [(x - xm)/xscale | x in X such that x <= xm]
  *     
  *     p := size(Zplus)
  *     q := size(Zminus)
  *     
  *     // define the kernel function closing over Zplus and Zminus
  *     function h(i,j):
  *         a := Zplus[i]
  *         b := Zminus[j]
  *         
  *         if a == b:
  *             return signum(p - 1 - i - j)
  *         else:
  *             return (a + b)/(a - b)
  *         endif
  *     endfunction
  *     
  *     // O(n^2) operations necessary to form this vector
  *     H := [h(i,j) | i in [0, 1, ..., p - 1] and j in [0, 1, ..., q - 1]]
  *     
  *     return median(H)
  * endfunction
  */

#include <stdlib.h>
#ifdef _DEBUG
#include <stdio.h>
#endif
#include <stdbool.h>
#include <assert.h>
#include <math.h>

typedef double value_t;

/**
  * Invert the usual comparator.
  */
static int _cmp_value_t_inv( const void *pvl, const void *pvr ) {

	const value_t *pl = (const value_t*)pvl;
	const value_t *pr = (const value_t*)pvr;
	if( *pl == *pr )
		return 0;
	else
		return *pl < *pr ? +1 : -1; // inverted!
}

static int _cmp_dbl( const void *pvl, const void *pvr ) {

	const double *pl = (const double*)pvl;
	const double *pr = (const double*)pvr;
	if( *pl == *pr )
		return 0;
	else
		return *pl < *pr ? -1 : +1;
}

static inline bool _is_odd(int n ) {
	return n % 2;
}

value_t medcouple_naive( const value_t *VALUES, int n ) {

	const int MIDBASE = n/2; // We *want* truncation here!
	const int SUBSETSIZE
		= _is_odd(n)
		? n/2 + 1
		: n/2;
	const int SSS_SQ
		= SUBSETSIZE*SUBSETSIZE;
	double SCALE, MEDIAN, mc;
	double *matrix = NULL;
	int i, j, k = 0;
	value_t *values = calloc( n, sizeof(value_t) );
	memcpy( values, VALUES, n*sizeof(value_t) );
	qsort( values, n, sizeof(value_t), _cmp_value_t_inv /* for descending sort */ );

	SCALE
		= 2.0*values[0];
	MEDIAN
		= _is_odd(n)
		? (values[MIDBASE])
		: (values[MIDBASE] + values[MIDBASE-1])/2.0;

	for(i = 0; i < SUBSETSIZE; i++ ) {
		assert(values[           i] >= MEDIAN );
		assert(values[SUBSETSIZE+i] <= MEDIAN );
	}
	// Scale the values.

	for(i = 0; i < n; i++ )
		values[i] = (values[i]-MEDIAN)/SCALE;

	// ...so head is > 0 ("Z+") and tail < 0 ("Z-").

	if( (matrix = calloc( SSS_SQ, sizeof(double) )) ) {

		for(i = 0; i < SUBSETSIZE; i++ ) {
			for(j = 0; j < SUBSETSIZE; j++ ) {
				const value_t A = values[            i ]; // Z+
				const value_t B = values[ SUBSETSIZE+j ]; // Z-
				assert( _is_odd(n) ? 0.0 <= A : 0.0 < A );
				assert( _is_odd(n) ? B <= 0.0 : B < 0.0 );
				if( A == B )
					matrix[k] // sgn(p-1-i-j)
						= ((SUBSETSIZE - 1 - i - j) < 0)
						? -1.0
						: +1.0;
				else
					matrix[k] = (A + B)/(A - B);
				k++;
			}
		}
#if defined(_DEBUG) && defined(HAVE_EMIT_KERNEL_RESULT)
		for(i = 0; i < SUBSETSIZE; i++ ) {
			printf( "%.3e", matrix[i*SUBSETSIZE] );
			for(j = 1; j < SUBSETSIZE; j++ ) {
				printf( "\t%.3e", matrix[i*SUBSETSIZE+j] );
			}
			fputc( '\n', stdout );
		}
#endif
		// TODO: Use qsel here...
		qsort( matrix, SSS_SQ, sizeof(double), _cmp_dbl );
		mc	= _is_odd( SSS_SQ )
			? (matrix[ SSS_SQ/2 ])
			: (matrix[ SSS_SQ/2 ] + matrix[ (SSS_SQ/2)-1 ])/2.0;

		free( matrix );
	} else
		abort();
	free( values );
	return mc;
}

#ifdef UNIT_TEST_NMC

#include <stdio.h>
#include <err.h>

int main( int argc, char *argv[] ) {

	if( argc > 1 ) {
		const int N = atoi( argv[1] );
		char *line = NULL;
		size_t blen = 0;
		value_t *v = calloc( N, sizeof(value_t) );
		int n = 0;

		while( (getline( &line, &blen, stdin ) > 0 ) && n < N ) {
			if( sizeof(value_t) == sizeof(float) )
				v[n++] = strtof( line, NULL );
			else
				v[n++] = strtod( line, NULL );
		}
		if( line )
			free( line );

		printf( "%.3e\n",  medcouple_naive( v, n ) );

		if( v )
			free( v );
	} else
		err( -1, "%s <value count>", argv[0] );
	return EXIT_SUCCESS;
}

#endif

