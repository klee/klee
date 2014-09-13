// RUN: %llvmgcc %s -emit-llvm -g -O0 -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out -use-cex-cache=1 -check-overshift=0 %t.bc
// RUN: not grep "ASSERTION FAIL" %t.klee-out/messages.txt
// RUN: grep "KLEE: done: explored paths = 1" %t.klee-out/info
#include <stdio.h>
#include <assert.h>

typedef enum
{
    TO_ZERO,
    MASKED,
    UNKNOWN
} overshift_t;

int overshift(volatile unsigned int y, const char * type)
{
    overshift_t ret;
    volatile unsigned int x=15;
    unsigned int limit = sizeof(x)*8;

    volatile unsigned int result;
    result = x << y; // Potential overshift

    if (result ==0)
    {
        printf("%s : Overshift to zero\n", type);
        ret = TO_ZERO;
    }
    else if (result == ( x << (y % limit)) )
    {
        printf("%s : Bitmasked overshift\n", type);
        ret = MASKED;
    }
    else
    {
        printf("%s : Unrecognised behaviour.\n", type);
        ret = UNKNOWN;
    }

    return ret;
}

int main(int argc, char** argv)
{
    // Concrete overshift
    volatile unsigned int y = sizeof(unsigned int)*8;
    overshift_t conc = overshift(y, "Concrete");
    assert( conc == TO_ZERO);

    // Symbolic overshift
    volatile unsigned int y2;
    klee_make_symbolic(&y2,sizeof(y),"y2");
    // Add constraints so only one value possible
    klee_assume(y2 > (y-1));
    klee_assume(y2 < (y+1));
    overshift_t sym = overshift(y2, "Symbolic");
    assert( sym == TO_ZERO);

    // Concrete and symbolic behaviour should be the same
    assert( conc == sym);

    return 0;
}
