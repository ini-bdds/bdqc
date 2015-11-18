
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include "fopenx.h"

enum Codec {
	CODEC_NONE = 0,
	CODEC_GZIP,
	CODEC_BZIP,
	CODEC_XZ,
	CODEC_COUNT,
	CODEC_UNKNOWN = CODEC_COUNT
};

/**
  * Following must correspond to CODEC_x constants.
  */
static const char *_decompress_cmd[] = {
	"",
	"gunzip --decompress --stdout",
	"bunzip2 --decompress --stdout --keep",
	"unxz  --decompress --stdout --keep"
};

/*
static const char *codec_system_exe( int type ) {
	return ( 0 < type && type < CODEC_COUNT )
		? _decompress_cmd[ type ]
		: "UNKNOWN";
}
*/

/**
  * GZIP: 1F 8B
  * BZIP: 42 5A 68          ('B', 'Z', 'h')
  *   XZ: FD 37 7A 58 5A 00 ( FD, '7', 'z', 'X', 'Z', 0x00)
  */
const unsigned char SIG_GZIP[] = { 0x1F, 0x8B };
const unsigned char SIG_BZIP[] = { 0x42, 0x5A, 0x68 };
const unsigned char SIG_XZ[]   = { 0xFD, 0x37, 0x7A, 0x58, 0x5A, 0x00 };

/**
  * Return < 0 implies error in determining the signature.
  */
static int codec_identify_by_sig( FILE *fp ) {

	char buf[6];
	int n, type = -1;

	// Good behavior dictates not changing the state of the stream, so
	// find out where it is first...
	const long POS
		= ftell( fp );

	// If that failed, nothing else will succeed...
	if( POS < 0 )
		return -1;

	// ...otherwise, seek to the beginning and read the 1st 6.
	fseek( fp, 0, SEEK_SET );
	n = fread( buf, sizeof(unsigned char), 6, fp );

	if( n >= 2 && memcmp( SIG_GZIP, buf, 2 ) == 0 )
		type = CODEC_GZIP;
	else
	if( n >= 3 && memcmp( SIG_BZIP, buf, 3 ) == 0 )
		type = CODEC_BZIP;
	else
	if( n >= 6 && memcmp( SIG_XZ,   buf, 6 ) == 0 )
		type = CODEC_XZ;
	else
		type = CODEC_UNKNOWN;

	// Restore the original file position
	if( fseek( fp, POS, SEEK_SET ) < 0 )
		return -1;

	return type;
}

#ifdef HAVE_REDUNDANT_INFERENCE
static int codec_identify_by_ext( const char *fname ) {

	const char *ext
		= paths_extension( fname );
	int type = -1;

	// The initial equality tests just speed things up a bit...

	if( ext ) {
		if( strcasecmp( ext, "gz" )  == 0 )
			type = CODEC_GZIP;
		else
		if( strcasecmp( ext, "bz2" ) == 0 )
			type = CODEC_GZIP;
		else
		if( strcasecmp( ext, "xz" )  == 0 )
			type = CODEC_XZ;
		else
			type = CODEC_UNKNOWN;
	}
	return type;
}

static int codec_identify( const char *fname ) {
	const int id_by_sig
		= codec_identify_by_sig( fname );
	const int id_by_ext
		= codec_identify_by_ext( fname );
	return id_by_ext == id_by_sig
		? id_by_ext
		: CODEC_UNKNOWN;
}
#endif

#ifdef _DEBUG
static int compressed = 0;
#endif

/**
  * Open a file, implicitly decompressing it if it's on of the three
  * recognized types of compression.
  */
FILE *fopenx( const char *fname, const char *mode ) {

	FILE *fp = fopen( fname, mode );
	if( fp ) {
		char *cmd = NULL;
		const int CODEC
		   = codec_identify_by_sig( fp );
		if( CODEC < 0 ) {
			int e = errno; // preserve current errno across fclose.
			fclose( fp );
			errno = e;
			return NULL;
		} else
		if( CODEC == CODEC_UNKNOWN ) {
			return fp; // ASSUMING it's plaintext
		}
		fclose( fp );
		fp = NULL;
		assert( CODEC < CODEC_COUNT );
		cmd = malloc( FILENAME_MAX+32 );
		if( cmd ) {
			sprintf( cmd, "%s %s", _decompress_cmd[CODEC], fname );
			fp = popen( cmd, "r" );
			if( NULL == fp ) {
				fprintf( stderr, "popen( \"%s\", \"r\" ): %s", cmd, strerror(errno) );
			}
#ifdef _DEBUG
			compressed = 1;
#endif
			free( cmd );
		}
		return fp;
	}
	return NULL; // ...and leave errno alone.
}


int fclosex( FILE *fp ) {
	// This is a possibly dangerous way to detect whether or not we need
	// to use pclose or fclose.
	const int ISPIPE
	   = ( ftell(fp) < 0 ) && ( errno == ESPIPE );
	assert( compressed == ISPIPE );
	return ISPIPE ? pclose( fp ) : fclose( fp );
}

#ifdef _UNIT_TEST_FOPENX_
int main( int argc, char *argv[] ) {

	if( argc > 1 ) {
		FILE *fp = fopenx( argv[1], "r" );
		if( fp ) {
			char *line = NULL;
			size_t n = 0;
			while( getline( &line, &n, fp ) > 0 )
				fputs( line, stdout );
			if( line )
				free( line );
			fclosex( fp );
			return EXIT_SUCCESS;
		}
	}
	return EXIT_FAILURE;
}
#endif

