
/**
  * A SIMPLE hashtable-based string set implementation.
  */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <getopt.h>
#include <assert.h>

#include "strset.h"

struct entry {

	const char *str;

#ifdef HAVE_BAG
	/**
	  * A "bag" is a set which allows duplicates, so a set can
	  * be turned into a bag just by maintaining a count.
	  */
	int count;
#endif
};

#define ENTRY_IS_OCCUPIED(s,p) ((s)->array[(p)].str != NULL)
#define ENTRY_IS_EMPTY(s,p)    ((s)->array[(p)].str == NULL)

#ifdef _DEBUG
static int _is_power_of_2( int v ) {
	return 0 == (v & (v-1));
}
#endif

static int _power_of_2_upper_bound( int v ) {
	int i = 0;
	while( (1<<i) < v && i < 31 ) i++;
	return (1<<i);
}


#ifdef HAVE_SET_GROW
/**
  * Rehash all valid entries in the entry array. There is no reason why 
  * every entry that fit in the previous (half-sized) table should not go
  * in the new table.
  * Warning: Disable strdup'ing temporarily
  */
static int _rehash( struct entry *cur, int n, struct strset *t ) {
	int error = 0;
	// Turn OFF string duplicating while rehashing...
	const int DUPING = s->dup;
	s->dup = 0;
	// ...because either:
	// 1. we weren't duplicating anyway, or
	// 2. we're rehashing strings already duplicated once that we own!
	// set_insert doesn't know which of these is true; but we do here.
	for(int i = 0; i < n; i++ ) {
		if( cur[i].str != NULL ) {
			if( set_insert( t, cur[i].str, NULL ) != SZS_ADDED ) {
				error = -1;
				break;
			}
		}
	}
	// WARNING: Don't return without resetting s->dup!
	s->dup = DUPING;
	return error;
}
#endif

/***************************************************************************
  * Public
  */

/**
  * Only RAM exhaustion can cause failure, so that is implied if return is 
  * NULL...no need for more error reporting.
  */

struct strset * set_create( unsigned int max, int dup, string_hash_fx fxn, unsigned int seed ) {

	struct strset *table
		= calloc( 1, sizeof(struct strset) );

	if( table ) {
		if( set_init( table, max, dup, fxn, seed ) ) {
			free( table );
			table = NULL;
		}
	}
	return table;
}


int set_init( struct strset *s, unsigned int max, int dup, string_hash_fx fxn, unsigned int seed ) {

	if( s ) {

		s->capacity
			= _power_of_2_upper_bound(max);

		assert( _is_power_of_2(s->capacity) );

		s->array
			= calloc( s->capacity, sizeof(struct entry) );

		if( s->array ) {
			s->mask
				= (s->capacity-1);
			s->dup
				= dup;
			s->hash
				= fxn;
			s->seed
				= seed;
			return 0;
		}
	}
	return -1;
}

#ifdef HAVE_SET_GROW
/**
  * Doubles the capacity of the table.
  * TODO: Currently indices are not stable--that is, after resizing strings
  * are mapped to different indices.
  */
int set_grow( struct strset *s ) {

	struct entry *cur
		= s->array;

	s->array
		= calloc( s->capacity*2, sizeof(struct entry) );

	if( s->array ) {

		const int OCCUPANCY // Hold the current sizes in case of failure.
			= s->occupancy;
		const int CAPACITY
			= s->capacity;

		s->occupancy = 0;   // Update the table's vital stats...
		s->capacity *= 2;
		s->mask = (s->capacity-1);

		if( _rehash( cur, CAPACITY, t ) ) {
			// On failure reset everything to its original state.
			free( s->array );
			s->occupancy = OCCUPANCY;
			s->capacity  = CAPACITY;
			s->mask = (s->capacity-1);
			s->array = cur;
			return SZS_RESIZE_FAILED;
		}
		assert( OCCUPANCY == s->occupancy );
		free( cur );
		return 0;
	}
	return SZS_OUT_OF_MEMORY;
}
#endif


/**
  * !!! Warning !!!
  * If this is a "bag" (ifdef HAVE_BAG), we need to try the insertion even
  * when the table is full in order to update the count.
  * !!! Warning !!!
  */
int set_insert( struct strset *s, const char *str ) {

	if( str != NULL && *str != '\0' ) {

		const int L
			= strlen( str );
		const unsigned int K
			= s->hash( str, L, s->seed );
		const int IDEAL
			= K & s->mask;
		int pos
			= IDEAL;
		struct entry *ent
			= NULL;

		// Find either an empty slot or a slot with the same key.

		while( ENTRY_IS_OCCUPIED(s,pos) && strcmp( s->array[pos].str, str ) ) {
			pos = (pos + 1) % s->capacity;
			if( pos == IDEAL )
				break;
		}

		ent = s->array + pos;
#ifdef HAVE_BAG
		ent->count += 1; // ...whatever it's current occupancy state.
#endif
		if( ENTRY_IS_EMPTY(s,pos) ) {
			ent->str    = s->dup ? strdup(str) : str;
			s->occupancy += 1;
			return SZS_ADDED;
		} else
		if( strcmp( ent->str, str ) == 0 ) {
			return SZS_PRESENT;
		}
	} else
		return SZS_ZERO_KEY;

	return SZS_TABLE_FULL;
}


int set_iter( const struct strset *s, void **cookie ) {

	if( cookie ) {
		*cookie = NULL;
		if( set_count( s ) > 0 ) {
			*cookie = s->array;
			return 1;
		} else
			return 0;
	}
	return 0;
}


/**
  * Return the next string.
  */
int set_next( const struct strset *s, void **cookie, const char **pstr ) {

	if( cookie ) {
		struct entry *ent = (struct entry *)(*cookie);
		while( ent < s->array + s->capacity ) {
			if( ent->str 
#ifdef HAVE_BAG
					&& ent->count > 0
#endif
					) {
				*pstr = ent->str;
				*cookie = ++ ent;
				return 1;
			}
			ent++;
		}
	}
	return 0;
}


int set_count( const struct strset *s ) {
	return s->occupancy;
}


/**
  * Frees strings owned by the table--that is, those that were copied on
  * insertion--and NULLs all pointers.
  */
void set_clear( struct strset *s ) {
	if( s->dup ) {
		for(int i = 0; i < s->capacity; i++ ) {
			if( s->array[i].str ) {
				free( (void*)(s->array[i].str) );
				s->array[i].str = NULL;
			}
		}
	}
	memset( s->array, 0, s->capacity*sizeof(struct entry) );
	s->occupancy = 0;
}


/**
  * Frees the array
  */
void set_fini( struct strset *s ) {

	set_clear( s );
	free( s->array );
	s->array = NULL;
}


void set_destroy( struct strset *s ) {

	if( s ) {
		set_fini( s );
		free( s );
	}
}


#ifdef _DEBUG
void set_dump( struct strset *s, FILE *fp ) {
	void *cookie;
	if( set_iter( s, &cookie ) ) {
		const char *sz;
		while( set_next( s, &cookie, &sz ) ) fprintf( fp, "%s\n", sz );
	}
}
#endif


#ifdef AUTO_TEST_STRSET

#include <stdio.h>
#include "murmur3.h"

/**
  * Sort the struct entries of the hash table such that all entries with
  * valid strings precede all entries without strings (.str==NULL), and
  */
static int _cmp_entry( const void *pvl, const void *pvr ) {

	const struct entry *l = (const struct entry *)pvl;
	const struct entry *r = (const struct entry *)pvr;
	int result = 0;

	if( l->str != NULL ) {
		if( r->str != NULL ) {
			result =  0;
		} else
			result = -1;
	} else
		result = r->str ? +1 : 0;

	return result;
}

int main( int argc, char *argv[] ) {

	int exit_status
		= 0;
	const int CAP
		= argc > 1 ? atoi( argv[1] ) : 10;
	const int DUP
		= argc > 2 ? atoi( argv[2] ) : 1;
	const int verbose
		= (int)( getenv("VERBOSE") != NULL );
	FILE *fp
		= argc > 3
		? fopen( argv[3], "r" )
		: stdin;

	char *line = NULL;
	size_t blen = 0;
	ssize_t llen;
	struct strset *s;

	if( NULL == fp ) {
		fprintf( stderr, "no input\n" );
		exit(-1);
	}

	s = set_create( CAP, DUP, murmur3_32, 17 );

	while( exit_status == 0 
			&& (llen = getline( &line, &blen, fp )) > 0 ) {
		line[llen-1] = '\0'; // lop off the newline.
#ifdef HAVE_SET_GROW
retry:
#endif
		switch( set_insert( s, line ) ) {
		case SZS_ADDED:
		case SZS_PRESENT:
			if( verbose )
				fprintf( stdout, "%s: OK\n", line );
			break;

		case SZS_ZERO_KEY:
			exit_status = 2;
			break;

		case SZS_TABLE_FULL:
#ifdef HAVE_SET_GROW
			if( set_grow( s ) == 0 )
				goto retry;
#endif
			exit_status = 3;
			break;

		default:
			exit_status = 4;
		}
	}
	if( line )
		free( line );

	fclose( fp );

	if( exit_status == 0 ) {

		const int N
			= set_count(s);
		int i;

		qsort( s->array, s->capacity, sizeof(struct entry), _cmp_entry );

		// Verify in a table with N entries,...
		// 1. no other non-null entries exist in the table.

		for(i = 0; i < N; i++ ) {
			fprintf( stdout, "%s\n", s->array[i].str );
		}

		for(     ; i < s->capacity; i++ ) {
			if( s->array[i].str != NULL ) {
				exit_status = 6;
				goto EXIT;
			}
		}
	}

EXIT:
	set_destroy( s );
	return exit_status;
}

#elif defined(UNIT_TEST_STRSET)

#include <stdint.h>
#include <stdio.h>
#include <err.h>
#include "murmur3.h"

static void command_loop( struct strset *s ) {

	char *line = NULL;
	size_t blen = 0;
	ssize_t llen;

	fprintf( stderr,
		"table capacity %d, mask %08x\n", s->capacity, s->mask );

	while( (llen = getline( &line, &blen, stdin )) > 0 ) {
		if( line[0] == '?' ) {
			printf( "%d/%d...\n", set_count(s), s->capacity );
			set_dump( s, stdout );
			printf( "--\n" );
			continue;
		} else
#ifdef HAVE_SET_GROW
		if( line[0] == '+' ) {
			if( set_grow( s ) )
				errx( -1, "failed growing" );
			continue;
		} else
#endif
		if( line[0] == '-' ) {
			set_clear( s );
			continue;
		}
		line[llen-1] = '\0'; // lop off the newline.
		switch( set_insert( s, line ) ) {
		case SZS_ADDED:
			printf( "+ %s\n", line );
			break;
		case SZS_PRESENT:
			printf( "P %s\n", line );
			break;
		case SZS_ZERO_KEY:
			printf( "! empty string\n" );
			break;
		case SZS_TABLE_FULL:
			printf( "! set full\n" );
			break;
		default:
			err( -1, "unexpected scanf count" );
		}
	}
	if( line )
		free( line );
}


int main( int argc, char *argv[] ) {

	int CAP = 32;

	int opt_interactive = 0;

	struct strset *s = NULL;

	if( argc < 2 )
		errx( -1, "%s [ -i -c <capacity>[%d] ]", argv[0], CAP );

	do {
		static const char *CHAR_OPTIONS 
			= "c:ih";
		static struct option LONG_OPTIONS[] = {

			{"cap",        1,0,'c'},
			{"interactive",0,0,'i'},
			{"help",       0,0,'h'},
			{ NULL,        0,0, 0 }
		};

		int opt_offset = 0;
		const int c = getopt_long( argc, argv, CHAR_OPTIONS, LONG_OPTIONS, &opt_offset );
		switch (c) {

		case 'c':
			CAP = atoi( optarg );
			break;
		case 'i':
			opt_interactive = 1;
			break;
		case -1: // ...signals no more options.
			break;
		default:
			printf ("error: unknown option: %c\n", c );
			exit(-1);
		}
		if( -1 == c ) break;

	} while( 1 );

	s = set_create( CAP, 1 /* duplicate */, murmur3_32, 0 );

	if( s ) {

		if( opt_interactive )
			command_loop( s );
		else {
			char *line = NULL;
			size_t blen = 0;
			ssize_t llen;
			while( (llen = getline( &line, &blen, stdin )) > 0 ) {
				line[llen-1] = '\0'; // lop off the newline.
#ifdef HAVE_SET_GROW
retry:
#endif
				switch( set_insert( s, line ) ) {
				case SZS_ADDED:
					break;
				case SZS_PRESENT:
					break;
				case SZS_ZERO_KEY:
					break;
				case SZS_TABLE_FULL:
#ifdef HAVE_SET_GROW
					if( set_grow( s ) == 0 )
						goto retry;
#endif
					break;
				default:
					err( -1, "unexpected scanf count" );
				}
			}
			if( line )
				free( line );
		}

		set_destroy( s );

	} else
		err( -1, "failed creating hash table with capacity %d", CAP );

	return EXIT_SUCCESS;
}
#endif

