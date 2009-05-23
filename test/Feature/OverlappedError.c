// RUN: %llvmgcc %s -g -emit-llvm -O0 -c -o %t1.bc
// RUN: %klee %t1.bc
// RUN: ls klee-last/ | grep .ktest | wc -l | grep 4
// RUN: ls klee-last/ | grep .ptr.err | wc -l | grep 2

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
