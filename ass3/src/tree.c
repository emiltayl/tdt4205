#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "tree.h"

/*
 * Macro for fetching integer-values for node_t's that contain integers.
 */
#define INTVAL(n) *((int*)n->data)

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

/*
 * A function to change a node with it's first child. To do this we store a copy
 * of the parent node, move the child node into the parent's position.
 * Then we free the space previously used by the child, but only after changing
 * the data- and children-fields of the child with those of the parents so we
 * don't lose any data.
 */
static void collapse_node(node_t* node) {
    node_t parent = *node;

    *node = *parent.children[0];
    parent.children[0]->data = parent.data;
    parent.children[0]->children = parent.children;

    node_finalize(parent.children[0]);
}

node_t* simplify_tree ( node_t* node ){
    /* Temporary pointer used for constant expression calulations */
    int* exp_tmp;

    if ( node != NULL ){
        // Recursively simplify the children of the current node
        for ( uint32_t i=0; i<node->n_children; i++ ){
            node->children[i] = simplify_tree ( node->children[i] );
        }

        // After the children have been simplified, we look at the current node
        // What we do depend upon the type of node
        switch ( node->type.index ) {
            // These are lists which needs to be flattened. Their structure
            // is the same, so they can be treated the same way.
            case FUNCTION_LIST: case STATEMENT_LIST: case PRINT_LIST:
            case EXPRESSION_LIST: case VARIABLE_LIST:
                if (node->children[0]->type.index == node->type.index) {
                    node->children[0]->n_children++;
                    node->children[0]->children = realloc(node->children[0]->children, sizeof(*node->children) * node->children[0]->n_children);

                    if (node->children[0]->children == NULL) {
                        fputs("Failed to reallocate space to make flat lists\n", stderr);
                        abort();
                    }

                    node->children[0]->children[node->children[0]->n_children - 1] = node->children[1];

                    collapse_node(node);
                }
                break;

            // Declaration lists should also be flattened, but their stucture is sligthly
            // different, so they need their own case
            case DECLARATION_LIST:
                 if (node->children[0] == NULL) {
                    node->children[0] = node->children[1];
                    node->children[1] = NULL;
                    collapse_node(node);
                } else if (node->children[0]->type.index == node->type.index) {
                    node->children[0]->n_children++;
                    node->children[0]->children = realloc(node->children[0]->children, sizeof(*node->children) * node->children[0]->n_children);

                    if (node->children[0]->children == NULL) {
                        fputs("Failed to reallocate space to make flat lists\n", stderr);
                        abort();
                    }

                    node->children[0]->children[node->children[0]->n_children - 1] = node->children[1];

                    collapse_node(node);
                }               break;

            // These have only one child, so they are not needed
            case STATEMENT: case PARAMETER_LIST: case ARGUMENT_LIST:
                collapse_node(node);
                break;

            // Expressions where both children are integers can be evaluated (and replaced with
            // integer nodes). Expressions whith just one child can be removed (like statements etc above)
            case EXPRESSION:
                if (node->n_children == 1 && node->data == NULL) {
                    collapse_node(node);
                } else if (node->n_children == 2 && node->children[0]->type.index == INTEGER && node->children[1]->type.index == INTEGER) {
                    exp_tmp = malloc(sizeof(*exp_tmp));

                    /* Ugly section, just calculations. */
                    if (strcmp(node->data, "+") == 0) { *exp_tmp = INTVAL(node->children[0]) + INTVAL(node->children[1]); }
                    else if (strcmp(node->data, "-") == 0) { *exp_tmp = INTVAL(node->children[0]) - INTVAL(node->children[1]); }
                    else if (strcmp(node->data, "*") == 0) { *exp_tmp = INTVAL(node->children[0]) * INTVAL(node->children[1]); }
                    else if (strcmp(node->data, "/") == 0) { *exp_tmp = INTVAL(node->children[0]) / INTVAL(node->children[1]); }
                    else if (strcmp(node->data, ">") == 0) { *exp_tmp = INTVAL(node->children[0]) > INTVAL(node->children[1]); }
                    else if (strcmp(node->data, "<") == 0) { *exp_tmp = INTVAL(node->children[0]) < INTVAL(node->children[1]); }
                    else if (strcmp(node->data, "<=") == 0) { *exp_tmp = INTVAL(node->children[0]) <= INTVAL(node->children[1]); }
                    else if (strcmp(node->data, ">=") == 0) { *exp_tmp = INTVAL(node->children[0]) >= INTVAL(node->children[1]); }
                    else if (strcmp(node->data, "==") == 0) { *exp_tmp = INTVAL(node->children[0]) == INTVAL(node->children[1]); }
                    else if (strcmp(node->data, "!=") == 0) { *exp_tmp = INTVAL(node->children[0]) != INTVAL(node->children[1]); }

                    node_finalize(node->children[0]);
                    node_finalize(node->children[1]);
                    free(node->data);
                    free(node->children);

                    /* Write new data over current node */
                    node_init(node, integer_n, exp_tmp, 0);
                }
                break;
        }

        return node;
    }
}
