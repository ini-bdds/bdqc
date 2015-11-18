
#include <stdlib.h>

const char *bstring( unsigned int n, int digits ) {
	static char buf[33];
	char *pc = buf;
	if( digits > 32 )
		return NULL;
	while( digits-- > 0 ) 
		*pc++ = ( (n & (1<<digits)) ? '1' : '0' );
	*pc = 0;
	return buf;
}

#ifdef UNIT_TEST_BSTRING
#include <stdio.h>
int main( int argc, char *argv[] ) {
	const unsigned int N
		= strtol( argv[1], NULL, 0 );
	printf( "%s\n", bstring( N, argc > 2 ? atoi(argv[2]) : 32 ) );
	return EXIT_SUCCESS;
}
#endif

