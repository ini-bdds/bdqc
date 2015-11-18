
#ifndef _rstrip_h_
#define _rstrip_h_

/**
  * Replace LF and CR chars at the end of a line with NUL and return the
  * count of characters remaining in the line after this replacement.
  */
static inline int rstrip( char *line, int n ) {
	while( n > 0 && (line[ n-1 ] == '\r' || line[ n-1 ] == '\n') )
		line[ --n ] = '\0';
	return n;
}

#endif

