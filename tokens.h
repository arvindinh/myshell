#ifndef tokens_h
#define tokens_h

//get_tokens:
//  parses line into individual (space-separated) tokens
//  input: null-terminated string
//  output: array of null-terminated strings
//          output array terminated by a NULL pointer
char ** get_tokens( const char * line );

void free_tokens( char ** tokens );

#endif /* tokens_h */
