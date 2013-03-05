#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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


void symtab_init(void) {
    scopes = malloc(sizeof(*scopes) * scopes_size);
    values = malloc(sizeof(*values) * values_size);
    strings = malloc(sizeof(*strings) * strings_size);
}


void symtab_finalize(void) {
    for (int i = 0; i <= scopes_index; i++) {
        scope_remove();
    }

    for (int i = 0; i <= values_index; i++) {
        free(values[i]);
    }

    for (int i = 0; i <= strings_index; i++) {
        free(strings[i]);
    }

    free(scopes);
    free(values);
    free(strings);
}


int32_t strings_add(char *str) {
    strings_index++;

    if (strings_index == strings_size) {
        /*
         * I double the size of this array every time, it should work decently
         * most of the time. If there are many strings we will probably have to
         * spend less time reallocing.
         */
        strings_size = strings_size << 1;
        strings = realloc(strings, sizeof(*strings) * strings_size);

        if (strings == NULL) {
            fprintf(stderr, "Failed to reallocate heap for strings array.\n");
            abort();
        }
    }

    strings[strings_index] = str;

    return strings_index;
}


void strings_output(FILE *stream) {
    fprintf(stream, ".data\n.INTEGER: .string \"%%d \"\n");
    for (int i = 0; i <= strings_index; i++) {
        fprintf(stream, ".STRING%d: .string %s\n", i, strings[i]);
    }
    fprintf(stream, ".globl main\n");
}


void scope_add(void) {
    scopes_index++;

    if (scopes_index == scopes_size) {
        /* See strings_add */
        scopes_size = scopes_size << 1;
        scopes = realloc(scopes, sizeof(*scopes) * scopes_size);

        if (scopes == NULL) {
            fprintf(stderr, "Failed to reallocate heap for the scope stack.\n");
        }
    }

    scopes[scopes_index] = ght_create(HASH_BUCKETS);
}


void scope_remove(void) {
    ght_finalize(scopes[scopes_index]);
    scopes_index--;
}


void symbol_insert(char *key, symbol_t *value) {
    values_index++;

    if (values_index == values_size) {
        /* See strings_add */
        values_size = values_size << 1;
        values = realloc(values, sizeof(*values) * values_size);

        if (values == NULL) {
            fprintf(stderr, "Failed to reallocate heap for the value array.\n");
        }
    }

    values[values_index] = value;
    value->depth = scopes_index;
    ght_insert(scopes[scopes_index], value, strlen(key), key);

// Keep this for debugging/testing
#ifdef DUMP_SYMTAB
fprintf ( stderr, "Inserting (%s,%d)\n", key, value->stack_offset );
#endif
}


symbol_t * symbol_get(char *key) {
    int32_t search_index = scopes_index;
    symbol_t* result = NULL;

    while (result == NULL && search_index >= 0) {
        result = ght_get(scopes[search_index], strlen(key), key);
        search_index--;
    }

    /*
     * Here would be a good place to print error messages if we failed to find
     * a symbol.
     */
// Keep this for debugging/testing
#ifdef DUMP_SYMTAB
    if ( result != NULL )
        fprintf ( stderr, "Retrieving (%s,%d)\n", key, result->stack_offset );
#endif

    return result;
}
