
/**
 * The quickselect algorithm and a test that demonstrates both its
 */

#include "quicksel.h"

static inline void _swap( qselnum_t *a, qselnum_t *b ) {
	const qselnum_t t = *a;
	*a = *b;
	*b = t;
}

/* select the k-th smallest item in array x of length len */

qselnum_t qselect( qselnum_t *x, const int N, const int K ) {
 
	qselnum_t pivot;
	int pos, i;

	int left = 0, right = N - 1;

	while( left < right ) {
		pivot = x[K];
		_swap( x+K, x+right );
		for(i = pos = left; i < right; i++) {
			if( x[i] < pivot ) {
				_swap( x+i, x+pos );
				pos++;
			}
		}
		_swap( x+right, x+pos );
		if( pos == K)
			break;
		if( pos < K )
			left  = pos + 1;
		else
			right = pos - 1;
	}
	return x[K];
}

#if 0

/**
  * This recursive version was copied from:
  * http://rosettacode.org/wiki/Quickselect_algorithm#C
  * ...just for reference. My iterative version is preferable.
  */
qselnum_t qselect(qselnum_t *v, int len, int k) {
	
	int i, st;// tmp;
	 
	for (st = i = 0; i < len - 1; i++) {
		if (v[i] > v[len-1]) continue;
		_swap(v+i, v+st);
		st++;
	}
 
	_swap(v+len-1, v+st);
 
	return k == st? v[st]
			:st > k	? qselect(v, st, k)
				: qselect(v + st, len - st, k - st);
}
#endif
