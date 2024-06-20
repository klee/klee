// RUN: %clang %s -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --size-model=range --capacity=10 %t.bc 2>&1 | FileCheck %s

// CHECK: KLEE: allocating symbolic size (capacity = 10,{{.*}})
// CHECK: KLEE: done: completed paths = 2

#include <stdio.h>
#include <stdlib.h>

#include "klee/klee.h"

void nop() { }

int main(int argc, char** argv) {
  unsigned n;
  klee_make_symbolic(&n, sizeof(n), "n");

  char *s = malloc(n);
  if (n > 1) {
    nop();
  }

  return 0;
}
