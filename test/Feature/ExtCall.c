// This test checks that symbolic arguments to a function call are correctly concretized
// RUN: %clang %s -emit-llvm %O0opt -g -c -o %t.bc

// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --external-calls=all --exit-on-error %t.bc 2>&1 | FileCheck %s

#include "klee/klee.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

int main() {
  int x;
  klee_make_symbolic(&x, sizeof(x), "x");
  klee_assume(x >= 0);

  int y = abs(x);
  printf("y = %d\n", y);
  // CHECK: calling external: abs((ReadLSB w32 0 x))

  assert(x == y);
}
