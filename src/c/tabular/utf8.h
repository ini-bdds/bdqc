
#ifndef _utf8_h_
#define _utf8_h_

/**
  * Copy at most maxlen bytes of a single UTF-8 character from input to
  * output. Note that an ASCII NUL character '\0' is considered a legitimate
  * "target", so, as long as:
  * 1. pointers are all non-NULL and
  * 2. maxlen > 0
  * ...at least one byte WILL ALWAYS be copied.
  * Returns the (positive) count of bytes copied if successful, or a negative
  * number, the absolute value of which indicates a count of uncopied bytes
  * if there was not room.
  *
  * Note that UNLIKE strcpy, this deals in characters, not strings, so the
  * notion of NUL-termination does not even apply--that is, this NEVER
  * appends a character.
  */
int utf8_chrcpy( char *output, const char *input, int maxlen );

/**
  * The number of additional following the first that comprise the a UTF-8
  * character with the given first byte value.
  */
int utf8_suffix_len( int first_byte );

int utf8_consume_suffix( int n, FILE *fp, char *buf );

#endif

