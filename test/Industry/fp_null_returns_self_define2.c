#include <stdio.h>


unsigned int *g_status;
unsigned int *MyValue(unsigned short index)
{
    if (index > 5) {
        return NULL;                                  // (1)
    }
    return g_status;
}

void sink(unsigned int *arg)
{
    if (*arg== 0) { /* expect NULL_RETURNS */  // CHECK: KLEE: WARNING: 100.00% NullPointerException False Positive at trace 1
        return;
    }
}

void TEST_NullReturns004(unsigned short index)
{
    if (index > 5) {
        return;
    }
    unsigned int *value = MyValue(index);           // (2)
    sink(value);
}

// RUN: %clang %s -emit-llvm -c -g -O0 -Xclang -disable-O0-optnone -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --use-guided-search=error --annotations=%annotations --mock-policy=all --libc=klee --skip-not-symbolic-objects --skip-not-lazy-initialized --use-lazy-initialization=only --analysis-reproduce=%s.json %t1.bc
// RUN: FileCheck -input-file=%t.klee-out/warnings.txt %s
