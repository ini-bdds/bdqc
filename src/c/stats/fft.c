
#include <stdlib.h>
#include <math.h>
#include <complex.h>

#include "fft.h" 

#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif

static void _fft( complex buf[], complex out[], int n, int step ) {

	if (step < n) {
		_fft(out, buf, n, step * 2);
		_fft(out + step, buf + step, n, step * 2);
 
		for (int i = 0; i < n; i += 2 * step) {
			complex t = cexp(-I * M_PI * i / n) * out[i + step];
			buf[i / 2]     = out[i] + t;
			buf[(i + n)/2] = out[i] - t;
		}
	}
}
 
void fft_fwd( complex buf[], int n) {

	complex *out = calloc( n, sizeof(complex) );
	if( out ) {
		for (int i = 0; i < n; i++) out[i] = buf[i];
		_fft(buf, out, n, 1);
		free( out );
	}
}
 
 
void fft_inv( complex buf[], int n, double scalar ) {

	for(int i = 0; i < n; i++ )
		buf[i] = conj( buf[i] );
 
    fft_fwd( buf, n );

	for(int i = 0; i < n; i++ )
		buf[i] = conj( buf[i] ) * scalar;
}

