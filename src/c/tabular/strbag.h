
#ifndef _strbag_h_
#define _strbag_h_

typedef unsigned int (*string_hash_fx)(const char *str, uint32_t len, uint32_t seed );

void * sbag_create( unsigned int max, int dup, string_hash_fx fxn, uint32_t seed );

#define SZS_ADDED          (+1)
#define SZS_PRESENT        ( 0)
#define SZS_ZERO_KEY       (-1)
#define SZS_TABLE_FULL     (-2)

int    sbag_insert( void *, const char * str, unsigned int * );


#define SZS_RESIZE_FAILED  (-1)
#define SZS_OUT_OF_MEMORY  (-2)
int    sbag_grow( void * );


/**
  */
#define SZS_NOT_FOUND      (-1)
int    sbag_tag( void *, const char * str );

const char *sbag_string( void *, unsigned );

int    sbag_count( void * );
/**
  * Clears the content without freeing the array. Memory allocated for
  * individual strings -is- released.
  * This is expressly to reuse existing array for new content.
  */
void   sbag_clear( void * );

/**
  * This frees -everything-.
  */
void   sbag_destroy( void * );
#ifdef _DEBUG
void sbag_dump( void *ht, FILE *fp );
#endif
#endif

