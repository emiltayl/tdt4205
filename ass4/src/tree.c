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
    /* Temporary variable used when calculating offset. */
    int tmp_offset;

    if (root == NULL) {
        return;
    }

    if (root->type.index == FUNCTION_LIST || root->type.index == FUNCTION|| root->type.index == BLOCK) {
        scope_add();
    }

    if (root->type.index == FUNCTION_LIST) {
        for (int i = 0; i < root->n_children; i++) {
            tmp = malloc(sizeof(*tmp));
            if (tmp == NULL) {
                fprintf(stderr, "Failed to allocate heap for symbol.\n");
                abort();
            }

            tmp->stack_offset = 0;
            tmp->label = root->children[i]->children[0]->data;
            symbol_insert(root->children[i]->children[0]->data, tmp);
        }

        for (int i = 0; i < root->n_children; i++) {
            bind_names(root->children[i]);
        }
    } else if (root->type.index == FUNCTION) {
        if (root->children[1] != NULL) {
            /* The function has parameters we need to add. */
            tmp_offset = 4 + 4 * root->children[1]->n_children;

            for (int i = 0; i < root->children[1]->n_children; i++, tmp_offset -= 4) {
                tmp = malloc(sizeof(*tmp));
                if (tmp == NULL) {
                    fprintf(stderr, "Failed to allocate heap for symbol.\n");
                    abort();
                }

                tmp->stack_offset = tmp_offset;
                symbol_insert(root->children[1]->children[i]->data, tmp);
            }
        }

        bind_names(root->children[2]);
    } else if (root->type.index == BLOCK) {
        if (root->children[0] != NULL) {
            /* The block has variables we should add */
            tmp_offset = -4;

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

        bind_names(root->children[1]);
    } else if (root->type.index == VARIABLE) {
        root->entry = symbol_get(root->data);
    } else if (root->type.index == TEXT) {
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
