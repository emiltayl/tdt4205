#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "tree.h"

/*
 * Macro for dereferencing data in a node_t that contain an integer.
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
 * of the parent node and move the child node into the parent's position.
 * Then we free the space previously used by the child, but only after changing
 * the data- and children-fields at that position with those of the parents so
 * we don't lose any data.
 */
static void collapse_node(node_t* node) {
    node_t parent = *node;

    *node = *parent.children[0];
    parent.children[0]->data = parent.data;
    parent.children[0]->children = parent.children;

    node_finalize(parent.children[0]);
}

/*
 * A function to make the various list-types flat. It works by increasing the
 * size of the left child's array of children by one, and then adding the right
 * child to the end of that array. Then we collapse the current node, bringing
 * the left child up as the current node.
 */
static void collapse_list(node_t* node) {
    node->children[0]->n_children++;
    node->children[0]->children = realloc(node->children[0]->children, sizeof(*node->children) * node->children[0]->n_children);

    if (node->children[0]->children == NULL) {
        fputs("Failed to reallocate space to make flat lists\n", stderr);
        abort();
    }

    node->children[0]->children[node->children[0]->n_children - 1] = node->children[1];

    collapse_node(node);
}

node_t* simplify_tree ( node_t* node ){
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
                    collapse_list(node);
                }
                break;

            // Declaration lists should also be flattened, but their stucture is sligthly
            // different, so they need their own case
            case DECLARATION_LIST:
                if (node->children[0] == NULL) {
                    /*
                     * node's left-most child is NULL, so we must shift all the
                     * elements in the array one position to the left.
                     * Afterwards we can shrink the children array by one.
                     */
                    node->n_children--;
                    memmove(node->children, &node->children[1], sizeof(*node->children) * node->n_children);

                    node->children = realloc(node->children, sizeof(*node->children) * node->n_children);

                    if (node->children == NULL) {
                        fputs("Failed to reallocate space to make flat lists\n", stderr);
                        abort();
                    }
                } else if (node->children[0]->type.index == node->type.index) {
                    collapse_list(node);
                }
                break;

            // These have only one child, so they are not needed
            case STATEMENT: case PARAMETER_LIST: case ARGUMENT_LIST:
                collapse_node(node);
                break;

            // Expressions where both children are integers can be evaluated (and replaced with
            // integer nodes). Expressions whith just one child can be removed (like statements etc above)
            case EXPRESSION:
                if (node->n_children == 1 && node->data == NULL) {
                    collapse_node(node);
                } else if (node->n_children == 1 && node->children[0]->type.index == INTEGER && strcmp(node->data, "-") == 0) {
                    /*
                     * Unary minus, multiply the value stored in the only child
                     * node with -1 and collapse this node.
                     */
                    INTVAL(node->children[0]) *= -1;
                    collapse_node(node);
                } else if (node->n_children == 2 && node->children[0]->type.index == INTEGER && node->children[1]->type.index == INTEGER) {
                    /*
                     * We perform all operations on the first child's data.
                     */

                    /* Ugly section, just calculations. */
                    if (strcmp(node->data, "+") == 0) { INTVAL(node->children[0]) += INTVAL(node->children[1]); }
                    else if (strcmp(node->data, "-") == 0) { INTVAL(node->children[0]) -= INTVAL(node->children[1]); }
                    else if (strcmp(node->data, "*") == 0) { INTVAL(node->children[0]) *= INTVAL(node->children[1]); }
                    else if (strcmp(node->data, "/") == 0) { INTVAL(node->children[0]) /= INTVAL(node->children[1]); }
                    else if (strcmp(node->data, ">") == 0) { INTVAL(node->children[0]) = INTVAL(node->children[0]) > INTVAL(node->children[1]); }
                    else if (strcmp(node->data, "<") == 0) { INTVAL(node->children[0]) = INTVAL(node->children[0]) < INTVAL(node->children[1]); }
                    else if (strcmp(node->data, "<=") == 0) { INTVAL(node->children[0]) = INTVAL(node->children[0]) <= INTVAL(node->children[1]); }
                    else if (strcmp(node->data, ">=") == 0) { INTVAL(node->children[0]) = INTVAL(node->children[0]) >= INTVAL(node->children[1]); }
                    else if (strcmp(node->data, "==") == 0) { INTVAL(node->children[0]) = INTVAL(node->children[0]) == INTVAL(node->children[1]); }
                    else if (strcmp(node->data, "!=") == 0) { INTVAL(node->children[0]) = INTVAL(node->children[0]) != INTVAL(node->children[1]); }

                    /*
                     * Free the current node data, copy the result to the
                     * current node and remove the pointer to the result from
                     * the first child, before free-ing up the space used by
                     * the children.
                     */
                    free(node->data);
                    node->data = node->children[0]->data;
                    node->children[0]->data = NULL;

                    node_finalize(node->children[0]);
                    node_finalize(node->children[1]);
                    free(node->children);

                    /* Write new data over current node */
                    node_init(node, integer_n, node->data, 0);
                }
                break;
        }

        return node;
    }
}
