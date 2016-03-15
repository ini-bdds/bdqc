#include <stdio.h>
#include <stdlib.h>
int main( int argc, char *argv[] ) {
	double stats[2];
	double *mu = stats + 0;
	double *ss = stats + 1;
	double delta;
	unsigned N = 0;
	char *line = NULL;
	size_t blen = 0;
	*mu = 0.0;
	*ss = 0.0;
	while( getline( &line, &blen, stdin ) > 0 ) {
		const double X = atof( line );
		// The code between the following BEGIN/END comments should be
		// copied VERBATIM from reduce.
		// BEGIN
		*mu = ( (N-0.0)*(*mu) + X )
			/   (N+1.0);
		delta = X - (*mu);
		if( N > 0 ) {
			*ss = ( (N-1.0)*(*ss) + (N+1.0) *delta*delta / N )
				/   (N-0.0);
		}
		// END
		N ++;
	}
	if( line )
		free( line );

	printf( "%.3e\t%.3e\n", *mu, *ss );
	return EXIT_SUCCESS;
}

