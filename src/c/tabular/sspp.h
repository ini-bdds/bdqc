
/**
  * String Sequence Prefix Partition.
  * This file defines the interface to code that simultaneously:
  * 1. identifies the longest shared prefix in a sequence of strings and
  * 2. partitions the sequence into groups of contiguous strings that share
  *    a common positive-length prefix.
  *
  * Additionally, groups may include any number of empty (0-length) strings.
  * That is, a "group" is:
  *
  *      0 or more empty strings followed by 1 or more non-empty strings
  *
  * This implements a state machine that accumulates characters "pushed"
  * into it (up to a maximum number set at compile time). Characters
  * pushed after the maximum is reached are simply ignored. When a string
  * is "flushed" the relationship between its prefix and an existing
  * prefix is analyzed and the existing prefix' length is modified if
  * necessary so that it remains common to all strings in the group.
  * Obviously, the prefix length can only monotonically decrease.
  */

#ifndef _sspp_h_
#define _sspp_h_

/**
  * Struct to encapsulate the results of prefix analysis.
  */
struct sspp_group {
	int empty; // Count of preceding empty strings.
	int snum;  // Offset of 1st string (e.g. line) on which prefix occurs.
	int count; // Count of strings in the group.
	char *prefix;
	int prefix_len;
};

struct sspp_analysis_state;

struct sspp_analysis_state *sspp_create_state( int expected_groups );

#define SSPP_GROUP_INCOMPLETE (0)
#define SSPP_GROUP_COMPLETION (1)
void sspp_push( struct sspp_analysis_state *s, const char *buf, int len );

int  sspp_flush( struct sspp_analysis_state *s );

struct sspp_group *sspp_group_ptr( struct sspp_analysis_state *s, int i );
int  sspp_dump_group( struct sspp_analysis_state *s, int i, FILE *fp );
int  sspp_dump( struct sspp_analysis_state *s, FILE *fp );

void sspp_destroy( struct sspp_analysis_state * );

#endif

