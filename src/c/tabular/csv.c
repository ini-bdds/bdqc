
/**
  * This faithfully implements RFC4180 with one exception:
  * lines may be termined by any of { CR, LF, CRLF, LFCR }.
  */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
  * CSV state machine
  */

enum {
	CSV_ENTRY,
	CSV_IN_QUOTED_FIELD,
	CSV_IN_BARE_FIELD,
	CSV_POSSIBLE_EXIT,
#ifdef HAVE_ALLOW_WS_TRAILING_QUOTED_FIELD
	CSV_EXITED,
#endif
	CSV_COUNT
};

struct csv_state_machine {

	int state;

	char *field;
	int cap; // maximum char content of field
	int len; // number of chars in field

	char lineterm[4];
};

static void _accum( struct csv_state_machine *sm, const char *utf8, int L ) {
	if( ! ( sm->len + L < sm->cap ) ) {
		void *p = realloc( sm->field, 2 * sm->cap );
		if( p ) {
			sm->field = p;
			sm->cap *= 2;
		} else {
			abort();
		}
	}
	strncpy( sm->field + sm->len, utf8, L );
	sm->len += L;
}


static void _flush( struct csv_state_machine *sm ) {
	fprintf( stdout, "%s\n", sm->field );
	memset( sm->field, 0, sm->cap );
	sm->len = 0;
}


struct csv_state_machine *csv_state_create( int initial_field_capacity ) {

	struct csv_state_machine *sm
		= calloc( 1, sizeof(struct csv_state_machine) );
	if( sm ) {
		sm->field
			= calloc( initial_field_capacity, sizeof(char) );
		if( sm->field ) {
			sm->cap = initial_field_capacity;
			return sm;
		}
		free( sm );
		sm = NULL;
	}
	return sm;
}


/**
  * Returns positive value when line is complete
  */
int csv_state_update( struct csv_state_machine *sm, const char *utf8, int len, char *lineterm ) {

#define QUOTE '"'
#define COMMA ','
#define SPACE ' '
#define ENDSZ '\0'
#define LF    0x0A
#define CR    0x0D

	const int C = *utf8;

	switch( sm->state ) {

	case CSV_ENTRY:
		if( C == QUOTE ) {
			sm->state = CSV_IN_QUOTED_FIELD;
		} else
		if( C == COMMA ) {
			_flush( sm );            // ...an empty field.
		} else
			_accum( sm, utf8, len ); // ...start of non-empty field.
		break;

	case CSV_IN_QUOTED_FIELD:
		if( C == QUOTE ) {
			sm->state = CSV_POSSIBLE_EXIT;
		} else
		if( C == ENDSZ ) {
			goto error; // end-of-string/line inside quote field!
		} else
			_accum( sm, utf8, len );
		break;

	case CSV_IN_BARE_FIELD:
		if( C == COMMA ) {
			_flush( sm );
			sm->state = CSV_ENTRY;
		} else
		if( C == ENDSZ ) {
			_flush( sm );
		} else
		if( C == LF || C == CR ) {
		} else
			_accum( sm, utf8, len );
		break;

	case CSV_POSSIBLE_EXIT:
		if( C == QUOTE ) {
			_accum( sm, "\"", 1 );
			// Not an exit. It was an escaped double-quote, so reenter...
			sm->state = CSV_IN_QUOTED_FIELD;
		} else
		if( C == COMMA ) {
			_flush( sm );
			sm->state = CSV_ENTRY;
		} else
		if( C == ENDSZ ) {
			_flush( sm );
#ifdef HAVE_ALLOW_WS_TRAILING_QUOTED_FIELD
		} else
		if( C == SPACE ) {
			_flush( sm );
			sm->state = CSV_EXITED;
#endif
		} else
			goto error; // end-of-string/line inside quote field!
		break;

#ifdef HAVE_ALLOW_WS_TRAILING_QUOTED_FIELD
	/**
	  * This case permits whitespace after a closing quote.
	  * This is not specified by RFC4180. In any even, as a
	  * corner case it should be last in the switch for performance.
	  */
	case CSV_EXITED:
		if( C == COMMA ) {
			// _flush already happened.
			sm->state = CSV_ENTRY;
		} else
		if( C != SPACE ) {
			abort(); // error
		}
		break;
#endif
	default:
		abort(); // Something's really bloody wrong!
	}

	return 0;

error:
	abort();
}


#ifdef UNIT_TEST_CSV
int main( int argc, char *argv[] ) {
	struct csv_state_machine *sm
		= csv_state_create( 8 );
	const char *pc = argv[1];
	do {
		csv_state_update( sm, pc, 1, NULL );
	} while( *pc++ );
	free( sm );
	return EXIT_SUCCESS;
}
#endif

