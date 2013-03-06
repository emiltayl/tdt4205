#include <stdio.h>
#include <stdlib.h>

#include "tree.h"
#include "symtab.h"


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


node_t *
node_init ( node_t *nd, nodetype_t type, void *data, uint32_t n_children, ... )
{
    va_list child_list;
    *nd = (node_t) { type, data, NULL, n_children,
        (node_t **) malloc ( n_children * sizeof(node_t *) )
    };
    va_start ( child_list, n_children );
    for ( uint32_t i=0; i<n_children; i++ )
        nd->children[i] = va_arg ( child_list, node_t * );
    va_end ( child_list );
    return nd;
}


void
node_finalize ( node_t *discard )
{
    if ( discard != NULL )
    {
        free ( discard->data );
        free ( discard->children );
        free ( discard );
    }
}


void
destroy_subtree ( node_t *discard )
{
    if ( discard != NULL )
    {
        for ( uint32_t i=0; i<discard->n_children; i++ )
            destroy_subtree ( discard->children[i] );
        node_finalize ( discard );
    }
}




void bind_names(node_t *root) {
    /* Temporary pointer used when making new symbols. */
    symbol_t *tmp;
    /*
     * Temporary variable used when setting the offset for symtab entries or
     * the index for strings.
     */
    int tmp_offset;

    /* "NULL-guard" */
    if (root == NULL) {
        return;
    }

    /* First we check whether we should add a new scope to the stack */
    if (root->type.index == FUNCTION_LIST || root->type.index == FUNCTION|| root->type.index == BLOCK) {
        scope_add();
    }

    /*
     * Now begins the ugliest part of this code, where all the magic for the
     * symbol table happens. Basically it is a list of several special cases
     * where something should happen, and just a simple recursion of the tree
     * otherwise.
     */
    if (root->type.index == FUNCTION_LIST) {
        /* First we need to add all the functions to the symbol table. */
        for (int i = 0; i < root->n_children; i++) {
            tmp = malloc(sizeof(*tmp));
            if (tmp == NULL) {
                fprintf(stderr, "Failed to allocate heap for symbol.\n");
                abort();
            }

            /*
             * root->children[i]->children[0] is the node containing the name of
             * the function.
             */
            tmp->stack_offset = 0;
            tmp->label = root->children[i]->children[0]->data;
            symbol_insert(root->children[i]->children[0]->data, tmp);
        }

        /*
         * Now that all the function names are added, we want to search for the
         * remaining symbols.
         */
        for (int i = 0; i < root->n_children; i++) {
            bind_names(root->children[i]);
        }
    } else if (root->type.index == FUNCTION) {
        /*
         * First we need to check whether the function has any parameters, and
         * in that case add them all.
         */
        if (root->children[1] != NULL) {
            /*
             * The parameters are all children of the current node's second
             * child. This code is not very maintainable, luckily, that's not a
             * requirement, even though I should try to make the code as
             * maintainable as possible.
             */
            tmp_offset = 4 + 4 * root->children[1]->n_children;

            for (int i = 0; i < root->children[1]->n_children; i++, tmp_offset -= 4) {
                tmp = malloc(sizeof(*tmp));
                if (tmp == NULL) {
                    fprintf(stderr, "Failed to allocate heap for symbol.\n");
                    abort();
                }

                /*
                 * We don't need to set the depth as symbol_insert handles that
                 * automatically.
                 */
                tmp->stack_offset = tmp_offset;
                symbol_insert(root->children[1]->children[i]->data, tmp);
            }
        }

        /*
         * The current node's third child contains the function body, whick is
         * where we will have to look for more symbol references.
         */
        bind_names(root->children[2]);
    } else if (root->type.index == BLOCK) {
        /*
         * We need to check whether the current block has any variables we
         * should add to the stack, and add them if that's the case.
         */
        if (root->children[0] != NULL) {
            tmp_offset = -4;

            /*
             * We need to iterate over all the declaration nodes in the
             * declaration list. Every declaration has a variable list with
             * potentially several variables which we also need to iterate over.
             */
            for (int i = 0; i < root->children[0]->n_children; i++) {
                for (int n = 0; n < root->children[0]->children[i]->children[0]->n_children; n++, tmp_offset -= 4) {
                    tmp = malloc(sizeof(*tmp));
                    if (tmp == NULL) {
                        fprintf(stderr, "Failed to allocate heap for symbol.\n");
                        abort();
                    }

                    tmp->stack_offset = tmp_offset;
                    symbol_insert(root->children[0]->children[i]->children[0]->children[n]->data, tmp);
                }
            }
        }

        /* Now we need to recurse through the statement list. */
        bind_names(root->children[1]);
    } else if (root->type.index == VARIABLE) {
        /*
         * We have reached a reference to a variable and insert the pointer to
         * the symtab entry.
         */
        root->entry = symbol_get(root->data);
    } else if (root->type.index == TEXT) {
        /*
         * We have reached a text node and have to add it to the string list.
         * As we don't want to store the string two places, we exchange the text
         * node's data pointer with a pointer to a variable with the index of
         * this string in the string array.
         */
        tmp_offset = strings_add(root->data);
        root->data = malloc(sizeof(tmp_offset));
        *((int*) root->data) = tmp_offset;
    } else {
        for (int i = 0; i < root->n_children; i++) {
            bind_names(root->children[i]);
        }
    }

    if (root->type.index == FUNCTION_LIST || root->type.index == FUNCTION|| root->type.index == BLOCK) {
        scope_remove();
    }
}
