#include <stdarg.h>

#include "tree.h"


#ifdef DUMP_TREES
void
node_print ( FILE *output, node_t *root, uint32_t nesting )
{
    if ( root != NULL )
    {
        fprintf ( output, "%*c%s", nesting, ' ', root->type.text );
        if ( root->type.index == INTEGER )
            fprintf ( output, "(%d)", *((int32_t *)root->data) );
        if ( root->type.index == VARIABLE || root->type.index == EXPRESSION )
        {
            if ( root->data != NULL )
                fprintf ( output, "(\"%s\")", (char *)root->data );
            else
                fprintf ( output, "%p", root->data );
        }
        fputc ( '\n', output );
        for ( int32_t i=0; i<root->n_children; i++ )
            node_print ( output, root->children[i], nesting+1 );
    }
    else
        fprintf ( output, "%*c%p\n", nesting, ' ', root );
}
#endif


node_t * node_init ( node_t *nd, nodetype_t type, void *data, uint32_t n_children, ... ) {
    va_list children;

    /* Initialise the struct. As I don't know much about entry other than the
     * fact that it is a pointer, I initialise it to NULL in order to avoid
     * side-effects du to junk data.
     */
    nd->type = type;
    nd->data = data;
    nd->entry = NULL;
    nd->n_children = n_children;
    nd->children = malloc(sizeof(*nd->children) * n_children);

    va_start(children, n_children);

    for (int i = 0; i < n_children; i++) {
        nd->children[i] = va_arg(children, node_t *);
    }

    va_end(children);

    return nd;
}


void node_finalize ( node_t *discard ) {
    /* Not much to see here, just some free-ing */
    if (discard != NULL) {
        free(discard->children);
        free(discard->data);
        free(discard);
    }
}


void destroy_subtree ( node_t *discard ){
    if (discard != NULL) {
        for (int i = 0; i < discard->n_children; i++) {
            destroy_subtree(discard->children[i]);
            node_finalize(discard->children[i]);
        }
        /* The assignment specifies that destory_subtree should finalize all the
         * children of *discard, but not discard itself, which is what this
         * code does. Valgrind (called like this: valgrind -v --tool=memcheck\
         *  --leak-check=full --show-reachable=yes\
         *  bin/vslc < vsl_programs/all_lexical.vsl) does report that there is
         * some memory that is not beeing freed, which could be fixed by calling
         * node_finalize on discard after destroy_subtree has been called on all
         * of *discard's children. However, the memory is still reachable, and I
         * feel that this codes best represents the textual description in the
         * assignment, so I have left if this way.
         */
    }
}


