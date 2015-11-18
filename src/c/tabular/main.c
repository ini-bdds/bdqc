
/**
  * This program emits either JSON or an ad hoc format. To get
  * "beautified" JSON, use Python like:
  * ./ccscan -J <filename> | python3 -c "import sys;import json;\
  * print(json.dumps(json.load(sys.stdin),sort_keys=True,indent=4))"
  * ./ccscan $1 | python3 -c "import sys;import json;print(json.dumps(json.load(sys.stdin),indent=4))"
  */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <getopt.h>
#include "format.h"
#include "scan.h"

extern const char *bstring( unsigned int n, int digits );

int main( int argc, char *argv[] ) {

	int exit_status = EXIT_SUCCESS;
	FILE *fp = stdin;
	struct file_analysis A;

	memset( &A, 0, sizeof(A) );

	do {
		static const char *CHAR_OPTIONS 
			= "h";
		static struct option LONG_OPTIONS[] = {

			{"help",       0,0,'h'},
			{ NULL,        0,0, 0 }
		};

		int opt_offset = 0;
		const int c = getopt_long( argc, argv, CHAR_OPTIONS, LONG_OPTIONS, &opt_offset );
		switch (c) {

		case -1: // ...signals no more options.
			break;
		default:
			printf ("error: unknown option: %c\n", c );
			exit(-1);
		}
		if( -1 == c ) break;

	} while( true );

	fp = optind < argc ? fopen( argv[optind], "r" ) : stdin;

	if( fp ) {

		const int status = scan( fp, &A );
		fclose( fp );

		if( status == E_FILE_IO ) {

			fprintf( stdout, "Failure (%s error) at file offset %ld...\n",
				error_string[status], A.ordinal-1 );
			exit_status = EXIT_FAILURE;

		} else {

			write_json( status, &A, stdout );

			/*for(int i = 0; i < A.len; i++ ) {
				const unsigned int B
					= (unsigned int)(A.utf8[i] & 0xFF);
				fprintf( stdout, "%s0x%02x(%s)",
					i > 0 ? " " : "", B, bstring(B,8) );
			}
			fputc( '\n', stdout );*/
		}
		fini_analysis( &A );
	}
	return exit_status;
}

