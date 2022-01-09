// REQUIRES: z3
// RUN: %clang %s -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out -solver-backend=z3 -max-solver-time=3 %t1.bc
// RUN: cat %t.klee-out/messages.txt | FileCheck --check-prefix=TEST %s

#include <stdio.h>
#include <stdint.h>

typedef struct _gbuff {
    int idx;
    int fd;
    uint8_t buffer[0xFF];
} gbuff;

int
main() { 
    int trans = 0;
    int idx = 1;
    int alloc_size = 0;
    int n = 0;

    klee_make_symbolic(&trans, sizeof(trans), "trans"); 
    klee_make_symbolic(&idx, sizeof(idx), "idx"); 

    n = trans < 0 ? 2 : 1;
    alloc_size = sizeof(gbuff) + n*100*100;
    gbuff *gif = calloc(1, alloc_size);
    if (!gif)
        return NULL;
        
    gif->idx = idx > 1 ? idx : 2;
    gif->buffer[gif->idx] =  0xFF;

    if (gif->fd >=0) {
        gif->buffer[gif->idx] =  0;
    }
    
// TEST: Query timed out (fork)
    return 0;
}

