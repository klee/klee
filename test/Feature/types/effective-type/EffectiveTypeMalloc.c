// RUN: %clang %s -emit-llvm -g -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --use-advanced-type-system --use-tbaa --use-lazy-initialization=none --use-gep-opt %t.bc 2>&1 | FileCheck %s

#include "klee/klee.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

void fillBytes(void *area) {
  *((int32_t *)area) = 10;
  *((float *)(area + 4)) = 1.0;
}

enum { AREA_BLOCKS = 8 };

int main() {
  void *area = malloc(AREA_BLOCKS * (sizeof(int32_t) + sizeof(float)));
  for (int i = 0; i < AREA_BLOCKS; ++i) {
    fillBytes(area + i * (sizeof(int32_t) + sizeof(float)));
  }

  double *doubleArea = malloc(2 * sizeof(double));
  doubleArea[0] = 1.0;
  doubleArea[1] = 1.1;

  double *ptr_double;
  klee_make_symbolic(&ptr_double, sizeof(ptr_double), "ptr_double");
  *ptr_double = 0.0;

  // CHECK-NOT: ASSERTION FAIL
  assert((void *)ptr_double < (void *)area || (void *)ptr_double >= (void *)area + AREA_BLOCKS * (sizeof(int32_t) + sizeof(float)));

  *ptr_double = 1.2;
  // CHECK: x
  if ((void *)ptr_double >= (void *)doubleArea && (void *)ptr_double < (void *)doubleArea + 2) {
    printf("x\n");
    return 0;
  }
  return 1;
}
