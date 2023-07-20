// RUN: %clang %s -emit-llvm -g -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --use-advanced-type-system --use-tbaa --use-lazy-initialization=none --use-gep-opt %t.bc 2>&1 | FileCheck %s

#include "klee/klee.h"
#include <assert.h>
#include <stdio.h>

typedef union U {
  int x;
  float y;
  double z;
} U;

int main() {
  U u;
  u.x = 0;
  int *pointer;
  klee_make_symbolic(&pointer, sizeof(pointer), "pointer");
  *pointer = 100;

  // CHECK: x
  if (u.x == 100) {
    printf("x");
    return 0;
  }

  return 1;
}
