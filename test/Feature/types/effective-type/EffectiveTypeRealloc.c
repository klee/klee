// RUN: %clang %s -emit-llvm -g -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --use-advanced-type-system --use-tbaa --use-lazy-initialization=none --use-gep-opt %t.bc 2>&1 | FileCheck %s

#include "klee/klee.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

enum { INIT = 4,
       NEXT = 8 };

void *createArea() {
  int32_t *preresult = malloc(INIT);
  *preresult = (int32_t)10;
  float *result = realloc(preresult, NEXT);
  *(result + 1) = (float)1.f;
  return result;
}

int main() {
  void *area = createArea();

  int *ptr_int;
  klee_make_symbolic(&ptr_int, sizeof(ptr_int), "ptr_int");
  *ptr_int = 100;

  // CHECK-DAG: x
  if ((void *)ptr_int == area) {
    printf("x\n");
    return 0;
  }

  float *ptr_float;
  klee_make_symbolic(&ptr_float, sizeof(ptr_float), "ptr_float");
  *ptr_float = 100.0;
  // CHECK-DAG: y
  if ((void *)ptr_float >= area + 4) {
    printf("y\n");
    return 0;
  }

  double *ptr_double;
  klee_make_symbolic(&ptr_double, sizeof(ptr_double), "ptr_double");
  *ptr_double = 100.0;

  // CHECK-NOT: ASSERTION FAIL
  assert(ptr_double != area);
  return 0;
}
