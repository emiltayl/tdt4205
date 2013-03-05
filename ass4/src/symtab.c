#include "symtab.h"

// static does not mean the same as in Java.
// For global variables, it means they are only visible in this file.

// Pointer to stack of hash tables 
static hash_t **scopes;

// Pointer to array of values, to make it easier to free them
static symbol_t **values;

// Pointer to array of strings, should be able to dynamically expand as new strings
// are added.
static char **strings;

// Helper variables for manageing the stacks/arrays
static int32_t scopes_size = 16, scopes_index = -1;
static int32_t values_size = 16, values_index = -1;
static int32_t strings_size = 16, strings_index = -1;


void
symtab_init ( void )
{
}


void
symtab_finalize ( void )
{
}


int32_t
strings_add ( char *str )
{
}


void
strings_output ( FILE *stream )
{
}


void
scope_add ( void )
{
}


void
scope_remove ( void )
{
}


void
symbol_insert ( char *key, symbol_t *value )
{

// Keep this for debugging/testing
#ifdef DUMP_SYMTAB
fprintf ( stderr, "Inserting (%s,%d)\n", key, value->stack_offset );
#endif

}


symbol_t *
symbol_get ( char *key )
{
    symbol_t* result = NULL;

// Keep this for debugging/testing
#ifdef DUMP_SYMTAB
    if ( result != NULL )
        fprintf ( stderr, "Retrieving (%s,%d)\n", key, result->stack_offset );
#endif
}
