// RUN: %clang %s -emit-llvm -g -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --use-advanced-type-system --use-tbaa --use-lazy-initialization=none --use-gep-opt %t.bc 2>&1 | FileCheck %s

#include "klee/klee.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { SIZE = 8 };

int main() {
  int *area = malloc(SIZE);
  memset(area, '1', SIZE);

  float *float_ptr;
  klee_make_symbolic(&float_ptr, sizeof(float_ptr), "float_ptr");
  *float_ptr = 10;

  int created = 0;

  // CHECK-DAG: x
  if ((void *)float_ptr == (void *)area) {
    ++created;
    printf("x\n");
  }

  int *int_ptr;
  klee_make_symbolic(&int_ptr, sizeof(int_ptr), "int_ptr");
  *int_ptr = 11;

  // CHECK-DAG: y
  // CHECK-DAG: z
  if ((void *)int_ptr == (void *)area + sizeof(float)) {
    ++created;
    printf(created == 2 ? "z\n" : "y\n");
    return 1;
  }
  return 2;
}
