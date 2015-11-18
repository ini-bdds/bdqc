
#ifndef _format_h_
#define _format_h_

struct format;

typedef void (*FIELD_PARSER)( const char *field, int offset, void *context );
typedef int (*LINE_PARSER)( char *pc, struct format *ps, FIELD_PARSER, void *context );

/**
  * There are only three (supported) possibilities for parsing fields from
  * lines:
  * 1. single ASCII character separators (e.g. tab, comma)
  * 2. whitespace clusters (described by the regex / +/)
  *    These are explicitly "coalescing;" single whitespace separators
  *    may also satisfy #1.
  * 3. CSV as defined by RFC4180.
  */
struct format {

#define MAXLEN_METADATA_PREFIX (7)

	/**
	  * UTF-8 character believed to indicate header/metadata/comment rows.
	  * We don't try to distinguish between them.
	  * A line beginning with this sequence is not parsed.
	  */
	char metadata_line_prefix[MAXLEN_METADATA_PREFIX+1];
	char metadata_line_prefix_len;

	/**
	  * Usually only the first character of this buffer is used.
	  */
	char column_separator[8];
	bool column_separator_is_regex;

	/**
	  * The fields per line.
	  */
	unsigned column_count;

	/**
	  * Number of (data) lines that contributed to the inference.
	  */
	unsigned data_lines_sampled;

	/**
	  * This merely chops up the line into a series of NUL-terminated tokens
	  * according to the parse_spec, and calls the given callback function.
	  */
	LINE_PARSER parse_line;
};

int format_infer( FILE *fp, int delim, int nlines, struct format *inferred );

#endif

