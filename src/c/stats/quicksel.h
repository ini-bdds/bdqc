
#ifndef _quicksel_h_
#define _quicksel_h_

/**
  * Redeclare this as necessary per application.
  */
typedef double qselnum_t;

/**
  * Return the Kth smallest element (0-based).
  */
qselnum_t qselect( qselnum_t *x, const int N, const int K );

#endif

