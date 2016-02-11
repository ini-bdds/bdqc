
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

#include "tabular.h"

extern const char *bstring( unsigned int n, int digits );

int main( int argc, char *argv[] ) {

	int exit_status = EXIT_FAILURE;
	FILE *fp = stdin;
	struct table_description d;

	memset( &d, 0, sizeof(d) );

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

		exit_status = tabular_scan( fp, &d );
		fclose( fp );

		if( d.status != E_COMPLETE ) {

			static char ebuf[ 4096 ];
			tabular_error( &d, sizeof(ebuf), ebuf );
			fputs( ebuf, stdout ); 

		}

		if( exit_status == EXIT_SUCCESS ) {

			tabular_as_json( &d, stdout );
		}

		tabular_free( &d );
	}
	return exit_status;
}

