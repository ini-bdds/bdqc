
#ifndef _strset_h_
#define _strset_h_

typedef unsigned int (*string_hash_fx)(const char *str, uint32_t len, uint32_t seed );

void * set_create( unsigned int max, int dup, string_hash_fx fxn, uint32_t seed );

/**
  */
int set_init( void *, unsigned int max, int dup, string_hash_fx fxn, unsigned int seed );

#define SZS_ADDED          (+1)
#define SZS_PRESENT        ( 0)
#define SZS_ZERO_KEY       (-1)
#define SZS_TABLE_FULL     (-2)

int set_insert( void *, const char * str );


#ifdef HAVE_SET_GROW
#define SZS_RESIZE_FAILED  (-1)
#define SZS_OUT_OF_MEMORY  (-2)
int    set_grow( void * );
#endif

int    set_count( void * );

/**
  * Begin iterating strings in the set by initializing a cookie.
  * Returns non-zero if and only if the arguments are valid and
  * the set is non-empty; otherwise 0.
  */
int    set_iter( void *, void **cookie );

/**
  * Copy a pointer to the next string in the set into the output argument
  * returning non-zero if and only if the arguments are valid and there IS
  * a next string. The cookie is updated.
  */
int    set_next( void *, void **, const char ** /* out */);

/**
  * Clears the content without freeing the array. Memory allocated for
  * individual strings -is- released.
  * This is expressly to reuse existing array for new content.
  */
void   set_clear( void * );


/**
  * Frees heap-allocated memory within the set, but not the set object
  * itself.
  */
void set_fini( void * );

/**
  * This frees -everything-.
  */
void   set_destroy( void * );

#ifdef _DEBUG
void set_dump( void *ht, FILE *fp );
#endif

#endif

