
#include "util.h"

int count( const int value, const int *array, int i ) {
	int n = 0;
	while( i-- > 0 )
		if( array[i] == value ) n++;
	return n;
}

int count_nonzero( const int *array, int i ) {
	int n = 0;
	while( i-- > 0 )
		if( array[i] != 0 ) n++;
	return n;
}

int find_nonzero( const int *array, const int N ) {
	int i = 0;
	while( i < N ) {
		if( array[i] != 0 )
			return i;
		i++;
	}
	return -1;
}

