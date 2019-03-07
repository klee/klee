// RUN: %clang %s -g -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out %t1.bc
// RUN: ls %t.klee-out/ | grep .ktest | wc -l | grep 4
// RUN: ls %t.klee-out/ | grep .ptr.err | wc -l | grep 2

#include <stdlib.h>

int main() {
  if (klee_range(0,2, "range")) {
    char *x = malloc(8);
    *((int*) &x[klee_range(0,6, "range")]) = 1;
    free(x);
  } else {
    char *x = malloc(8);
    *((int*) &x[klee_range(-1,5, "range")]) = 1;
        free(x);
  }
  return 0;
}
