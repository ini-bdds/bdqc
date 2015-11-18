
/**
  * This module identifies contiguous groups of strings in a sequence of
  * strings with a common (non-empty) prefix. Because the prefix must be
  * at least one character, a "group" may consist of a single string.
  *
  * Because this module was motivated by strings corresponding to *lines*
  * and lines may be empty, empty (0-length) strings are allowed as well.
  * A sequence of empty strings is NOT treated as a distinct group.
  * It is treated as belonging to the "prefix group" that it precedes.
  *
  * That is each "group" is 0 or more empty strings followed by 1 or more
  * strings with a common non-empty prefix.
  */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <alloca.h>
#include <assert.h>

#include "sspp.h"

#define MAXLEN_STRING (15)

struct sspp_analysis_state {

	/**
	  * .buf and .n constitute a (UTF-8 capable) accumulator for string
	  * prefixes.
	  */
	char buf[MAXLEN_STRING+1];
	int  n;
	int  snum;

	/**
	  * The maximum number of groups the analyzer will identify.
	  * Defines the size of the array in which they're stored!
	  */
	int expected_groups;

	/**
	  * Number of identified groups.
	  */
	int complete_groups;

	struct sspp_group group[ 0 ];
};


/**
  * Find the common prefix if any, at most maxlen chars, between the
  * strings.
  *
  * Note that the strings CAN be UTF8, though the last char in the
  * identified sequence may not be a valid (complete) UTF-8 * character if,
  * for example, it's a UTF-8/3 and the 2nd bytes agree but the 3rd don't.
  * In this case, the character will be prematurely truncated.
  */
static int _common_prefix_len( const char *a, const char *b, int maxlen ) {
	int n = 0;
	while( maxlen-- > 0
			&& (*a) // we're not at NUL-terminator
			&& (*b) // we're not at NUL-terminator
			&& (*a++ == *b++) ) n++;
	return n;
}


int sspp_analyze( struct sspp_analysis_state *s, const char *string, int snum ) {

	int status
		= SSPP_GROUP_INCOMPLETE;
	const int L
		= strlen( string );
	struct sspp_group *g
		= s->group + s->complete_groups;
	const int CUR_CPL
		= g->prefix_len;

	assert( s->complete_groups < s->expected_groups );

	if( L > 0 /* NON empty string */ ) {

		if( CUR_CPL > 0 /* If we have a prefix for group i, ... */ ) {

			const int NEW_CPL
				= _common_prefix_len( g->prefix, string, CUR_CPL );

			if( NEW_CPL > 0 /* A common prefix might... */ ) {
				/* ...require updating the current. */

				if( CUR_CPL > NEW_CPL ) {
					g->prefix_len = NEW_CPL;
					g->prefix[ NEW_CPL ] = '\0'; // ALWAYS insure NUL-termination
				}

				g->count++;
				return SSPP_GROUP_INCOMPLETE;

			} else { /* NO common prefix is another... */
				/* ...transition trigger. */
				
				status = SSPP_GROUP_COMPLETION;
				if( ++(s->complete_groups) < s->expected_groups )
					g = s->group + s->complete_groups;
				else
					goto leave;
			}
		}

		g->snum = snum;
		g->prefix = strdup( string );
		g->prefix_len = L;
		g->count = 1;

	} else { /* an EMPTY string */

		if( CUR_CPL > 0 /* If we have a prefix for group i, ... */ ) {
			/* ...then this is a transition to next group. */

			status = SSPP_GROUP_COMPLETION;
			if( ++(s->complete_groups) < s->expected_groups )
				g = s->group + s->complete_groups;
			else
				goto leave;
		}
		g->empty++;
	}
leave:
	return status;
}


struct sspp_analysis_state *sspp_create_state( int expected_groups ) {
	const size_t REQUIRED
		= (1               * sizeof(struct sspp_analysis_state))
		+ (expected_groups * sizeof(struct sspp_group));
	struct sspp_analysis_state *obj
		= malloc( REQUIRED );
	memset( obj, 0, REQUIRED );
	obj->expected_groups = expected_groups;
	return obj;
}


/**
  * Note that this handles UTF-8 characters and multi-byte sequences.
  */
void sspp_push( struct sspp_analysis_state *s, const char *buf, int len ) {

	/**
	  * EOL chars need not be prohibited in general, but in the context in
	  * which this code was developed, they shouldn't be present...
	  */
	assert( *buf != '\n' && *buf != '\r' );

	if( s->n + len <= MAXLEN_STRING ) {
		memcpy( s->buf + s->n, buf, len );
		s->n += len;
	}
}


int sspp_flush( struct sspp_analysis_state *s ) {

	int status = sspp_analyze( s, s->buf, s->snum );
	s->snum += 1;
	memset( s->buf, 0, sizeof(s->buf) );
	s->n = 0;
	return status;
}


struct sspp_group *sspp_group_ptr( struct sspp_analysis_state *s, int i ) {

	if( i < 0 )
		return (-i) <= s->expected_groups ? s->group + (s->complete_groups + i) : NULL;
	else
		return   i  <  s->expected_groups ? s->group + i : NULL;
}


int sspp_dump_group( struct sspp_analysis_state *s, int i, FILE *fp ) {
	struct sspp_group *g
		= sspp_group_ptr( s, i );
	return g && g->prefix
		? fprintf( fp, "%d:%d:%d \"%s\"\n", g->snum, g->empty, g->count, g->prefix )
		: 0;
}


int sspp_dump( struct sspp_analysis_state *s, FILE *fp ) {

	const int N // Insure 1st group is emitted even if incomplete!
		= s->complete_groups < 1 ? 1 : s->complete_groups;
	int n = 0;
	for(int i = 0; i < N; i++ )
		n += sspp_dump_group( s, i, fp );
	return n;
}


void sspp_destroy( struct sspp_analysis_state *s ) {

	for(int i = 0; i < s->expected_groups; i++ ) {
		struct sspp_group *g
			= sspp_group_ptr( s, i );
		if( g->prefix )
			free( g->prefix );
	}
	free( s );
}


#ifdef UNIT_TEST_SSPP

int main( int argc, char *argv[] ) {

	if( argc > 1 ) {

		struct sspp_analysis_state *s
			= sspp_create_state( atoi( argv[1] ) );
		char *pc = argv[2];

		do {
			if( *pc == ',' || *pc == '\0' ) {
				if( sspp_flush( s ) == SSPP_GROUP_COMPLETION 
					&& s->complete_groups == s->expected_groups )
					break;
			} else
				sspp_push( s, pc, 1 );
		} while( *pc++ != '\0' );
		sspp_dump( s, stdout );
		if( s )
			free( s );
	}

	return EXIT_SUCCESS;
}
#endif

