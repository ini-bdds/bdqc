
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "tabular.h"
#include "strset.h"

/**
  * First level items:
  * 1. offending_byte: the ordinal of binary byte, or 0 if file is text.
  * 2. transition_histogram: 3x3 matrix, if file is entirely text
  * 3. character_histogram: 6 character class counts if file entirely text.
  * 4. table: if file contains a table/matrix
*/

static inline const char *_json_bool_value( bool b ) {
	return b ? "true" : "false";
}


static void _json_encode_ascii( const char *pc, FILE *fp ) {
	while( *pc ) {
		const int C = *pc++;
		switch( C ) {
		case '"':
			fputs( "\\\"", fp );
			break;
		/*case '/':
			*out++ = '\\';
			*out++ = '/';
			break;*/
		case '\\':
			fputs( "\\\\", fp );
			break;
		case '\b':
			fputs( "\\b", fp );
			break;
		case '\f':
			fputs( "\\f", fp );
			break;
		case '\n':
			fputs( "\\n", fp );
			break;
		case '\r':
			fputs( "\\r", fp );
			break;
		case '\t':
			fputs( "\\t", fp );
			break;
		default:
			fputc( C, fp );
		}
	}
}


/**
  */
int tabular_as_json( const struct table_description *a, FILE *fp ) {

	const int NC = a->table.column_count;
	const struct column *c = a->column;

	/**
	  * If analysis failed because of an I/O error there is possibly
	  * nothing valid. TODO: Perhaps write a status string anyway?
	  */
	if( a->status != E_FILE_IO )
		return -1;

	if( fputc( '{', fp ) < 0 )
		return -1;

	/**
	  * Any error entirely precludes histograms.
	  */

	if( a->status == E_UTF8_PREFIX || a->status == E_UTF8_SUFFIX ) {

		if( fprintf( fp,
			"\"offending_byte\":%ld,"
			"\"character_histogram\":null,"
			"\"transition_histogram\":null,",
			a->ordinal ) < 0 )
			return -1;

	} else {

		const unsigned long *C
			= a->char_class_counts;
		if( fprintf( fp,
			"\"offending_byte\":0,"
			"\"character_histogram\":{"
			"\"lf\":%ld,"
			"\"cr\":%ld,"
			"\"ascii\":%ld,"
			"\"utf8-2\":%ld,"
			"\"utf8-3\":%ld,"
			"\"utf8-4\":%ld},",
			C[ CC_LF ],
			C[ CC_CR ],
			C[ CC_ASCII ],
			C[ CC_UTF8_2 ],
			C[ CC_UTF8_3 ],
			C[ CC_UTF8_4 ] ) < 0 )
			return -1;

		C = a->char_class_transition_matrix;
		if( fprintf( fp,
			"\"transition_histogram\":{"
			"\"lf\":{\"lf\":%ld,\"cr\":%ld,\"oc\":%ld},"
			"\"cr\":{\"lf\":%ld,\"cr\":%ld,\"oc\":%ld},"
			"\"oc\":{\"lf\":%ld,\"cr\":%ld,\"oc\":%ld}"
			"},",
			C[0], C[1], C[2],
			C[3], C[4], C[5],
			C[6], C[7], C[8] ) < 0 )
			return -1;
	}

	if( fputs( "\"table\":", fp ) < 0 )
		return -1;

	if( a->column == NULL ) {
		fputs( "null", fp );
	} else {
	
		int nrem;
		if( fputs( "{\"metadata_prefix\":\"", fp ) < 0 )
			return -1;
			_json_encode_ascii( a->table.metadata_line_prefix, fp );
		if( fputs( "\",", fp ) < 0 )
			return -1;

		if( fputs( "\"column_separator\":\"", fp ) < 0 )
			return -1;
			_json_encode_ascii( a->table.column_separator, fp );
		if( fputs( "\",", fp ) < 0 )
			return -1;

		if( fprintf( fp,
			"\"separator_is_regex\":%s,"
			"\"column_count\":%d,"
			"\"empty_lines\":%d,"
			"\"meta_lines\":%d,"
			"\"data_lines\":%d,"
			"\"aberrant_lines\":%d,"
			"\"columns\":[",
			_json_bool_value( a->table.column_separator_is_regex ),
			NC,
			a->rows.empty,
			a->rows.meta,
			a->rows.data,
			a->rows.aberrant ) < 0 )
			return -1;

		/**
		  * Emit data for each column.
		  */

		nrem = NC; while( nrem-- > 0 ) {

			void *cookie;
			const int NL
				= c->label_set == NULL
				? 0
				: set_count( c->label_set );

			if( fprintf( fp, "{"
				"\"votes\":{\"empty\":%d,\"integer\":%d,\"float\":%d,\"string\":%d},"
				"\"stats\":{\"mean\":%f,\"variance\":%f},"
				"\"labels\":[",
				c->type_vote[FTY_EMPTY],
				c->type_vote[FTY_INTEGER],
				c->type_vote[FTY_FLOAT],
				c->type_vote[FTY_STRING],
				c->statistics[0],
				c->statistics[1] ) < 0 )
				return -1;

			if( set_iter( c->label_set, &cookie ) ) {
				const char *s;
				int j = 0;
				while( set_next( c->label_set, &cookie, &s ) ) {
					if( fputs( "\"", fp ) < 0 )
						return -1;

						_json_encode_ascii( s, fp );

					if( fprintf( fp, "\"%s", j+1 < NL ? "," : "" ) < 0 )
						return -1;
					j++;
				}
			}

			if( fprintf( fp, "],\"max_labels_exceeded\":%s}%s",
					_json_bool_value(c->label_set_exhausted),
					nrem > 0 ? "," : "" ) < 0 )
				return -1;
			c++;
		}
		fputs( "]}" /* end of table "analysis" */ , fp );
	}

	return fputc( '}', fp ) < 0 ? -1 : 0;
}

