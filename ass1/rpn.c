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
    // TODO push n on stack, expand stack if neccesary
}

void performOp(RpnCalc* rpnCalc, char op){
    // TODO perform operation
}

double peek(RpnCalc* rpnCalc){
    // TODO return top element of stack
}
