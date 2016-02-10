
#ifndef _strset_h_
#define _strset_h_

typedef unsigned int (*string_hash_fx)(const char *str, uint32_t len, uint32_t seed );

struct entry;

/**
  * This is publicly declared only so that its size may be determined
  * (so that it can be stack-resident or otherwise incorporated into
  * fixed size objects).
  * NONE OF THIS STRUCT SHOULD BE USED DIRECTLY.
  */
struct strset {

	/**
	  * Maximum number of strings containable.
	  */
	int capacity;

	/**
	  * Number of strings actually contained in the table.
	  */
	int occupancy;

	unsigned int mask;

	/**
	  * Non-zero indicates that the table "owns" the strings--that is,
	  * strings are duplicated on insertion and must be freed on removal.
	  */
	int dup;

	/**
	  * The hashing function and seed, provided by client.
	  */
	string_hash_fx hash;
	unsigned int   seed;

	/**
	  * 
	  */
	struct entry *array;
};


/**
  * Create (heap allocate) and initialize the root data structure for
  * the string set.
  */
struct strset *set_create( unsigned int max, int dup, string_hash_fx fxn, uint32_t seed );

/**
  * Merely initialize an otherwise-allocated root data structure.
  * This supports stack-allocation or incorporation into other
  * datastructures.
  */
int set_init( struct strset *, unsigned int max, int dup, string_hash_fx fxn, unsigned int seed );

#define SZS_ADDED          (+1)
#define SZS_PRESENT        ( 0)
#define SZS_ZERO_KEY       (-1)
#define SZS_TABLE_FULL     (-2)

int set_insert( struct strset *, const char * str );


#ifdef HAVE_SET_GROW
#define SZS_RESIZE_FAILED  (-1)
#define SZS_OUT_OF_MEMORY  (-2)
int    set_grow( struct strset * );
#endif

int    set_count( const struct strset * );

/**
  * Begin iterating strings in the set by initializing a cookie.
  * Returns non-zero if and only if the arguments are valid and
  * the set is non-empty; otherwise 0.
  */
int    set_iter( const struct strset *, void **cookie );

/**
  * Copy a pointer to the next string in the set into the output argument
  * returning non-zero if and only if the arguments are valid and there IS
  * a next string. The cookie is updated.
  */
int    set_next( const struct strset *, void **, const char ** /* out */);

/**
  * Clears the content without freeing the array. Memory allocated for
  * individual strings -is- released.
  * This is expressly to reuse existing array for new content.
  */
void   set_clear( struct strset * );


/**
  * Frees heap-allocated memory within the set, but not the set object
  * itself.
  */
void set_fini( struct strset * );

/**
  * This frees -everything-.
  */
void   set_destroy( struct strset * );

#ifdef _DEBUG
void set_dump( struct strset *, FILE *fp );
#endif

#endif

