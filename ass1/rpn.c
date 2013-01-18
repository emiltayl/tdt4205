#include <stdio.h>
#include <stdlib.h>

#include "rpn.h"

RpnCalc newRpnCalc(){
    RpnCalc rpnCalc;

    rpnCalc.size = 0;
    /* -1 as in there is no top-most element */
    rpnCalc.top = -1;
    /*
     * Since we can't store anything on a stack with a size of 0, it makes
     * sense to set stack to a NULL-pointer. If we realloc a NULL-pointer,
     * it will be just like a normal malloc.
     */
    rpnCalc.stack = NULL;

    return rpnCalc;
}

void push(RpnCalc* rpnCalc, double n){
    /*
     * We want to add an element to the (rpnCalc.top + 1)-nth position and
     * increase rpnCalc.top by one. To keep things easy we first increase top
     * and then check whether we need to realloc before adding n to the stack.
     */

    rpnCalc->top++;

    if (rpnCalc->top >= rpnCalc->size) {
        rpnCalc->size = rpnCalc->top + 1;
        rpnCalc->stack = realloc(rpnCalc->stack, sizeof(*(rpnCalc->stack)) * rpnCalc->size);

        if (rpnCalc->stack == NULL) {
            /*
             * We failed to increase the size of the stack as needed, and abort
             * the process. Freeing the stack is not required, as the process
             * aborts anyways.
             */
            fputs("Failed to increase stack size, aborting.\n", stderr);
            abort();
        }
    }

    rpnCalc->stack[rpnCalc->top] = n;
}

void performOp(RpnCalc* rpnCalc, char op){
    /*
     * Decrease rpnCalc-top and return the old top as we fetch the two top-most
     * numbers. Should segfault if we try this with only one item in the stack,
     * by design.
     *
     * First we have to check that there are two or more elements in the stack
     * in order to avoid unspecified behaviour.
     */

    if (rpnCalc->top < 1) {
        fputs("Tried to perform operation without enough numbers in the stack.\n", stderr);
        abort();
    }

    double S = rpnCalc->stack[(rpnCalc->top)--],
           T = rpnCalc->stack[rpnCalc->top],
           result;

    switch (op) {
        case '+':
            result = T + S;
            break;
        case '-':
            result = T - S;
            break;
        case '*':
            result = T * S;
            break;
        case '/':
            result = T / S;
            break;
        default:
            /* Unknown operator, abort */
            fprintf(stderr, "Unknown operator: \"%c\".\n", op);
            abort();
    }

    rpnCalc->stack[rpnCalc->top] = result;
}

double peek(RpnCalc* rpnCalc){
    /* We should return 0 if the stack is empty */
    if (rpnCalc->top == -1) {
        return 0;
    } else {
        return rpnCalc->stack[rpnCalc->top];
    }
}
