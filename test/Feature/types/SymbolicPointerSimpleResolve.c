// RUN: echo "x" > %t.res
// RUN: echo "x" >> %t.res
// RUN: echo "x" >> %t.res
// RUN: echo "x" >> %t.res
// RUN: %clang %s -emit-llvm -g -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --use-advanced-type-system --use-tbaa --use-lazy-initialization=none --skip-local=false --use-gep-opt %t.bc >%t.log
// RUN: diff %t.res %t.log

#include "klee/klee.h"
#include <stdio.h>

int main() {
  int a = 100;
  int b = 200;
  int c = 300;
  unsigned int d = 400;

  int *pointer;
  klee_make_symbolic(&pointer, sizeof(pointer), "pointer");
  *pointer += 11;

  if (pointer == &a || pointer == &b || pointer == &c || pointer == &d) {
    printf("x\n");
  }
}
