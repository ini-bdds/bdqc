
#include <stdio.h>
#include <stdlib.h>
/**
  * This both indicates whether a byte is the first of a multi-byte sequence
  * (assumed to be a UTF-8 character ) and how many following bytes are part
  * of the same character.
  * Success yields the count of additional bytes to read (which may be 0).
  * Failure is indicated by -1.
  */
int utf8_suffix_len( int first_byte ) {
	if( (0x80 & first_byte) == 0 )
		return 0; // It's ASCII, otherwise...
	// ...verify its a valid UTF8 prefix byte:
	if( (0xE0 & first_byte) == 0xC0 )
		return 1;
	if( (0xF0 & first_byte) == 0xE0 )
		return 2;
	if( (0xF8 & first_byte) == 0xF0 )
		return 3;
	// If MSBs don't agree with any of preceding we're probably in a non-
	// textual binary file...
	return -1;
}


int utf8_chrcpy( char *output, const char *input, int maxlen ) {
	const int N = 1+utf8_suffix_len( *input );
	int n = N;
	while( n > 0 && maxlen-- > 0 ) {
		*output++ = *input++;
		n--;
	}
	return n == 0 ? N : -n;
}


/**
  * Returns:
  * -1 for read failure
  *  0 for success
  * +n for failure with the count actually read.
  */
int utf8_consume_suffix( int n, FILE *fp, char *buf ) {
	const int rd
		= fread( buf, sizeof(char), n, fp );
	if( rd != n )
		return -1;
	while( n-- > 0 ) {
		// ALL UTF8 suffix bytes' MSBs should be 10xx-xxxx
		if( ( buf[n] & 0xC0 ) != 0x80 )
			return rd;
	}
	return 0;
}


