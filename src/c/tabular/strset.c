
/**
  * A SIMPLE string set implementation:
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
};

struct table {
	int capacity;
	int occupancy;
	unsigned int mask;
	int dup;

	string_hash_fx hash;
	unsigned int   seed;

	struct entry *array;
};

#define ENTRY_IS_OCCUPIED(t,p) ((t)->array[(p)].str != NULL)
#define ENTRY_IS_EMPTY(t,p)    ((t)->array[(p)].str == NULL)

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
static int _rehash( struct entry *cur, int n, struct table *t ) {
	int error = 0;
	// Turn OFF string duplicating while rehashing...
	const int DUPING = t->dup;
	t->dup = 0;
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
	// WARNING: Don't return without resetting t->dup!
	t->dup = DUPING;
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

void * set_create( unsigned int max, int dup, string_hash_fx fxn, unsigned int seed ) {

	struct table *table
		= calloc( 1, sizeof(struct table) );

	if( table ) {
		if( set_init( table, max, dup, fxn, seed ) ) {
			free( table );
			table = NULL;
		}
	}
	return table;
}


int set_init( void *ht, unsigned int max, int dup, string_hash_fx fxn, unsigned int seed ) {

	if( ht ) {

		struct table *t
			= (struct table *)ht;

		t->capacity
			= _power_of_2_upper_bound(max);

		assert( _is_power_of_2(t->capacity) );

		t->array
			= calloc( t->capacity, sizeof(struct entry) );

		if( t->array ) {
			t->mask
				= (t->capacity-1);
			t->dup
				= dup;
			t->hash
				= fxn;
			t->seed
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
int set_grow( void *ht ) {

	struct table *t 
		= (struct table*)ht;

	struct entry *cur
		= t->array;

	t->array
		= calloc( t->capacity*2, sizeof(struct entry) );

	if( t->array ) {

		const int OCCUPANCY // Hold the current sizes in case of failure.
			= t->occupancy;
		const int CAPACITY
			= t->capacity;

		t->occupancy = 0;   // Update the table's vital stats...
		t->capacity *= 2;
		t->mask = (t->capacity-1);

		if( _rehash( cur, CAPACITY, t ) ) {
			// On failure reset everything to its original state.
			free( t->array );
			t->occupancy = OCCUPANCY;
			t->capacity  = CAPACITY;
			t->mask = (t->capacity-1);
			t->array = cur;
			return SZS_RESIZE_FAILED;
		}
		assert( OCCUPANCY == t->occupancy );
		free( cur );
		return 0;
	}
	return SZS_OUT_OF_MEMORY;
}
#endif


int set_insert( void *ht, const char *str ) {

	struct table *t 
		= (struct table*)ht;

	assert( (t != NULL) && (str != NULL) );

	if( *str ) {

		const int L
			= strlen( str );
		const unsigned int K
			= t->hash( str, L, t->seed );
		const int IDEAL
			= K & t->mask;
		int pos
			= IDEAL;
		struct entry *ent
			= NULL;

		// Find either an empty slot or a slot with the same key.

		while( ENTRY_IS_OCCUPIED(t,pos) && strcmp( t->array[pos].str, str ) ) {
			pos = (pos + 1) % t->capacity;
			if( pos == IDEAL )
				break;
		}

		ent = t->array + pos;
#ifdef HAVE_BAG
		ent->count += 1;
#endif
		if( ENTRY_IS_EMPTY(t,pos) ) {
			ent->str    = t->dup ? strdup(str) : str;
			t->occupancy += 1;
			return SZS_ADDED;
		} else
		if( strcmp( ent->str, str ) == 0 ) {
			return SZS_PRESENT;
		}
	} else
		return SZS_ZERO_KEY;

	return SZS_TABLE_FULL;
}


int set_iter( void *ht, void **cookie ) {

	assert( ht );

	if( cookie ) {
		struct table *t 
			= (struct table*)ht;
		*cookie = NULL;
		if( set_count( ht ) > 0 ) {
			*cookie = t->array;
			return 1;
		}
	}
	return 0;
}


/**
  * Return the next string.
  */
int set_next( void *ht, void **cookie, const char **pstr ) {

	assert( ht );

	if( cookie ) {
		struct table *t 
			= (struct table*)ht;
		struct entry *ent = (struct entry *)(*cookie);
		while( ent < t->array + t->capacity ) {
			if( ent->str ) {
				*pstr = ent->str;
				*cookie = ent++;
				return 1;
			}
		}
	}
	return 0;
}


int set_count( void *ht ) {
	return ((struct table *)ht)->occupancy;
}


/**
  * Frees strings owned by the table--that is, those that were copied on
  * insertion--and NULLs all pointers.
  */
void set_clear( void *ht ) {
	struct table *t 
		= (struct table*)ht;
	if( t->dup ) {
		for(int i = 0; i < t->capacity; i++ ) {
			if( t->array[i].str )
				free( (void*)(t->array[i].str) );
		}
	}
	memset( t->array, 0, t->capacity*sizeof(struct entry) );
	t->occupancy = 0;
}


/**
  * Frees the array
  */
void set_fini( void *ht ) {

	struct table *t 
		= (struct table*)ht;

	assert( t != NULL );

	set_clear( ht );
	free( t->array );
	t->array = NULL;
}


void set_destroy( void *ht ) {

	if( ht ) {
		set_fini( ht );
		free( ht );
	}
}


#ifdef _DEBUG
void set_dump( void *ht, FILE *fp ) {
	struct table *t 
		= (struct table*)ht;
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
	void *h;

	if( NULL == fp ) {
		fprintf( stderr, "no input\n" );
		exit(-1);
	}

	h = set_create( CAP, DUP, murmur3_32, 17 );

	while( exit_status == 0 
			&& (llen = getline( &line, &blen, fp )) > 0 ) {
		line[llen-1] = '\0'; // lop off the newline.
#ifdef HAVE_SET_GROW
retry:
#endif
		switch( set_insert( h, line ) ) {
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
			if( set_grow( h ) == 0 )
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
			= set_count(h);
		struct table *t
			= (struct table*)h;
		int i;

		qsort( t->array, t->capacity, sizeof(struct entry), _cmp_entry );

		// Verify in a table with N entries,...
		// 1. no other non-null entries exist in the table.

		for(i = 0; i < N; i++ ) {
			fprintf( stdout, "%s\n", t->array[i].str );
		}

		for(     ; i < t->capacity; i++ ) {
			if( t->array[i].str != NULL ) {
				exit_status = 6;
				goto EXIT;
			}
		}
	}

EXIT:
	set_destroy( h );
	return exit_status;
}

#elif defined(UNIT_TEST_STRSET)

#include <stdint.h>
#include <stdio.h>
#include <err.h>
#include "murmur3.h"

extern unsigned int hash( const char *s, unsigned int );

static void command_loop( void *h ) {
	char *line = NULL;
	size_t blen = 0;
	ssize_t llen;

	struct table *t
		= (struct table *)h;
	fprintf( stderr,
		"table capacity %d, mask %08x\n", t->capacity, t->mask );

	while( (llen = getline( &line, &blen, stdin )) > 0 ) {
		if( line[0] == '?' ) {
			printf( "%d/%d\n", set_count(h), t->capacity );
			continue;
		} else
#ifdef HAVE_SET_GROW
		if( line[0] == '+' ) {
			if( set_grow( h ) )
				errx( -1, "failed growing" );
			continue;
		} else
#endif
		if( line[0] == '-' ) {
			set_clear( h );
			continue;
		}
		line[llen-1] = '\0'; // lop off the newline.
		switch( set_insert( h, line ) ) {
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

	void *h = NULL;

	if( argc < 2 )
		errx( -1, "%s <capacity>", argv[0] );

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
			CAP = atoi( argv[1] );
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

	h = set_create( CAP, 1 /* duplicate */, murmur3_32, 0 );

	if( h ) {
		if( opt_interactive )
			command_loop( h );
		else {
			char *line = NULL;
			size_t blen = 0;
			ssize_t llen;
			while( (llen = getline( &line, &blen, stdin )) > 0 ) {
				line[llen-1] = '\0'; // lop off the newline.
#ifdef HAVE_SET_GROW
retry:
#endif
				switch( set_insert( h, line ) ) {
				case SZS_ADDED:
					break;
				case SZS_PRESENT:
					break;
				case SZS_ZERO_KEY:
					break;
				case SZS_TABLE_FULL:
#ifdef HAVE_SET_GROW
					if( set_grow( h ) == 0 )
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

		printf( "cardinality = %d\n", set_count( h ) );
		set_destroy( h );

	} else
		err( -1, "failed creating hash table with capacity %d", CAP );

	return EXIT_SUCCESS;
}
#endif

