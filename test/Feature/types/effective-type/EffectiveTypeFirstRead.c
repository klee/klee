// RUN: %clang %s -emit-llvm -g -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --use-advanced-type-system --use-tbaa --use-lazy-initialization=none --use-gep-opt %t.bc 2>&1 | FileCheck %s

#include "klee/klee.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

enum { SIZE = 4 };

int main() {
  char *area = malloc(SIZE);
  for (int i = 0; i < SIZE; ++i) {
    area[i] = '0';
  }

  int *pointer;
  klee_make_symbolic(&pointer, sizeof(pointer), "pointer");
  *pointer = 1;

  // CHECK-DAG: x
  if ((void *)pointer == (void *)area) {
    printf("x\n");
    // CHECK-NOT: ASSERTION FAIL
    assert((area[0] == 1) ^ (area[3] == 1));
  }

  return 0;
}
