
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define _max(x,y) ((x)<(y)?(y):(x))

/**
  * Pretty print a table of counts by
  * 1. determining the minimum field width to accomodate everything,
  *    (headers, labels and data)
  * 2. printing with the proper space padding.
  */
void format_table( 
		int nrow, const char *rlabel[],
		int ncol, const char *clabel[],
		const unsigned long*data /* row-major order*/,
		FILE *fp ) {

	char format_label[16];
	char format_data[16];

	int i, j, m = 0;

	// Determine the minimum cell width for all to line up.

	i = nrow;
	while( i-- )
		m = _max(m,strlen(rlabel[i]));
	if( clabel != rlabel ) {
		i = ncol;
		while( i-- ) m = _max(m,strlen(rlabel[i]));
	}

	// Minimum field width for data.

	for(i = 0; i < nrow; i++ ) {
		for(j = 0; j < ncol; j++ ) {
			const int n
				= snprintf( NULL, 0, "%ld", data[i*ncol+j] );
			if( m < n )
				m = n;
		}
	}

	m += 1;

	sprintf( format_label, "%% %ds", m );
	sprintf( format_data,  "%% %dld", m );

	// We now know the minimum cell width to equally space all row and
	// column labels as well as data contents.

	fprintf( fp, format_label, "" );
	for(j = 0; j < ncol; j++ )
		fprintf( fp, format_label, clabel[j] );
	fputc( '\n', fp );
	for(i = 0; i < nrow; i++ ) {
		fprintf( fp, format_label, rlabel[i] );
		for(j = 0; j < ncol; j++ ) {
			fprintf( fp, format_data, data[ i*ncol+j ] );
		}
		fputc( '\n', fp );
	}
}

#ifdef UNIT_TEST_TABLE
#include <time.h>
int main( int argc, char *argv[] ) {
	const char *COL[] = { "FOO", "BAXX", "B" };
	const char *ROW[] = {
		"POF",
		"MOZZE",
		"FOBBER",
		"FPT"
	};
	unsigned long DAT[3*4];
	int i;

	srandom( time(NULL) );
	for(i = 0; i < 3*4; i++ ) DAT[i] = random();

	format_table( 4, ROW, 3, COL, DAT, stdout );

	return 0;
}
#endif

