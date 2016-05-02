
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#ifndef EXHAUSTIVE_OUTPUT
#include <string.h>
#endif

#include "tabular.h"
#include "strset.h"
#include "column.h"

#ifndef EXHAUSTIVE_OUTPUT
#include "murmur3.h"
#endif

#define MAGIC_TOO_MANY_LABELS (0xFFFFFFFF)

static const char *STAT_CLASS_NAME[] = {
	"unknown",
	"categorical",
	"quantitative",
	"ordinal"
};

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


#ifdef EXHAUSTIVE_OUTPUT

/**
  * Emit select parts of the table description as JSON.
  * The command line utility version emits everything available.
  */
int tabular_as_json( const struct table_description *a, FILE *fp ) {

	const int NC = a->table.column_count;

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
			"}",
			C[0], C[1], C[2],
			C[3], C[4], C[5],
			C[6], C[7], C[8] ) < 0 )
			return -1;
	}

	if( a->column != NULL ) {

		if( fputs( ",\"table\":{\"metadata_prefix\":\"", fp ) < 0 )
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

		for(int colnum = 0; colnum < NC; colnum++ ) {

			const struct column *c = a->column + colnum;
			void *cookie;
			const int NL
				= set_count( & c->value_set );

			if( fprintf( fp, "{"
				"\"inferred_class\":\"%s\","
				"\"votes\":{\"empty\":%d,\"integer\":%d,\"float\":%d,\"string\":%d},"
				"\"stats\":{\"mean\":%.3e,\"stddev\":%.3e},"
				"\"extrema\":{\"min\":%f,\"max\":%f},"
				"\"max_field_length\":%d,"
				"\"long_field_count\":%d,"
				"\"labels\":[",
				STAT_CLASS_NAME[ c->stat_class ],
				c->type_vote[FTY_EMPTY],
				c->type_vote[FTY_INTEGER],
				c->type_vote[FTY_FLOAT],
				c->type_vote[FTY_STRING],
				c->statistics[0], c->statistics[1],
				c->extrema[0],    c->extrema[1],
				c->max_field_len,
				c->long_field_count
			   	) < 0 )
				return -1;

			if( set_iter( & c->value_set, &cookie ) ) {
				const char *s;
				int j = 0;
				while( set_next( & c->value_set, &cookie, &s ) ) {
					if( fputs( "\"", fp ) < 0 )
						return -1;

						_json_encode_ascii( s, fp );

					if( fprintf( fp, "\"%s", j+1 < NL ? "," : "" ) < 0 )
						return -1;
					j++;
				}
			}

			if( fprintf( fp, "],\"max_labels_exceeded\":%s}%s",
					_json_bool_value( c->excess_values ),
					colnum + 1 < NC ? "," : "" ) < 0 )
				return -1;
		}
		fputs( "]}" /* end of table "analysis" */ , fp );
	}

	return fputc( '}', fp ) < 0 ? -1 : 0;
}

#else // ! EXHAUSTIVE_OUTPUT (abridged for BDQC)

static int _cmp_str( const void *pvl, const void *pvr ) {
	const char *l = *(const char **)pvl;
	const char *r = *(const char **)pvr;
	return strcmp( l, r );
}

static uint32_t _hash_labels( const struct strset *label_set ) {

	const int N = set_count( label_set );
	const char **plabel = calloc( N, sizeof(char*) );
	uint32_t hash = 0x19682112;
	if( plabel ) {

		void *cookie;
		int n = 0;

		if( set_iter( label_set, &cookie ) ) {
			// Fill the array with string pointers
			while( set_next( label_set, &cookie, plabel + n ) )
				n++;
		}

		// Insure that two sets with same content hash to same value.
		qsort( plabel, N, sizeof(char*), _cmp_str );

		for(n = 0; n < N; n++ ) {
			hash = murmur3_32( plabel[n], strlen(plabel[n]), hash );
		}
		free( plabel );
	}
	return hash;
}


/**
  * "Dominant" type is *not* majority type! Since only one type is reported,
  * dominance is defined as follows...
  * 1. Presence of *any* value not interpretable as numeric forces
  *    interpretation of the column as string.
  * 2. Otherwise, all values are numeric, and presence of any value not
  *    interpretable as integer forces floating-point.
  * 3. Otherwise, presence of any integers forces interpretation as
  *    integral.
  * 4. Finally, column could be entirely empty.
  */
static const char *_dominant_type_name( const int vote[] ) {
	if( vote[ FTY_STRING ]  > 0 )
		return "string";
	if( vote[ FTY_FLOAT ]   > 0 )
		return "float";
	if( vote[ FTY_INTEGER ] > 0 )
		return "int";
	if( vote[ FTY_EMPTY ]   > 0 )
		return "empty"; // meaning "entirely" empty
	return "bug";       // should never happen
}


/**
  * The plugin version only emits metrics that are expected to produce the
  * same or a tightly normally distributed set of value(s) for all files in
  * a set of "similar" files.
  *
  * metadata_prefix		string   typically "#"
  * non_utf8            integer  ordinal (1-based) of first non-UTF8 byte
  *                              0 if file is entirely UTF8
  *
  * lines_data			integer
  * lines_empty			integer
  * lines_meta			integer
  * lines_aberrant		integer
  *
  * column_count		integer
  *
  * columns/./type		one of {'string','float','int','empty','mixed'}
  * columns/./stat		one of {'unknown','categorical','quantitative','ordinal'}
  *
  * EITHER
  * columns/./label_set_hash	string
  * ...OR...
  * columns/./mean				float
  * columns/./stddev			float
  */
int tabular_as_json( const struct table_description *a, FILE *fp ) {

	const int NC = a->table.column_count;

	if( fputc( '{', fp ) < 0 )
		return -1;

	/**
	  * Any error entirely precludes histograms.
	  */

	if( fprintf( fp,
		"\"non_utf8\":%d",
		a->status == E_UTF8_PREFIX || a->status == E_UTF8_SUFFIX ) < 0 )
		return -1;

	if( a->column != NULL ) {

		if( fputs( ",\"table\":{\"metadata_prefix\":\"", fp ) < 0 )
			return 0;
		_json_encode_ascii( a->table.metadata_line_prefix, fp );
		if( fputs( "\",", fp ) < 0 )
			return -1;

		if( fprintf( fp,
			"\"lines_empty\":%d,"
			"\"lines_data\":%d,"
			"\"lines_meta\":%d,"
			"\"lines_aberrant\":%d,"
			"\"column_count\":%d,"
			"\"columns\":[",
			a->rows.empty,
			a->rows.data,
			a->rows.meta,
			a->rows.aberrant,
			NC ) < 0 )
			return -1;

		/**
		  * Emit data for each column.
		  */

		for(int colnum = 0; colnum < NC; colnum++ ) {

			const struct column *c = a->column + colnum;

			if( fprintf( fp, "{" /* opening column summary */
				"\"type\":\"%s\","
				"\"class\":\"%s\"",
				_dominant_type_name( c->type_vote ),
				STAT_CLASS_NAME[ c->stat_class ] ) < 0 )
				return -1;

			/**
			  * Emit EITHER label_set_hash (for categorical features)
			  * or quantitative stats for quantitative
			  */

			if( c->stat_class == STC_CAT || c->stat_class == STC_QUA ) {
				fputc( ',', fp );
			}

			if( c->stat_class == STC_CAT ) {
				const int NL
					= set_count( & c->value_set );
				const uint32_t HASH
					= (c->excess_values || NL == 0 )
					? (c->excess_values ? MAGIC_TOO_MANY_LABELS: 0 )
					: _hash_labels( & c->value_set );
				if( fprintf( fp, "\"label_set_hash\":\"%08X\"", HASH ) < 0 )
					return -1;
			} else
			if( c->stat_class == STC_QUA ) {
				if( fprintf( fp,
					"\"stats\":{\"mean\":%.3e,\"stddev\":%.3e}",
					c->statistics[0], c->statistics[1] ) < 0 )
					return -1;
			}

			fputc( '}' /* closing column summary */ , fp );
			if( colnum+1 < NC )
				fputc( ',', fp );
		}
		fputs( "]}" /* end of columns array and "table" */ , fp );
	}

	return fputc( '}' /* end of file" */, fp ) < 0 ? -1 : 0;
}

#endif

