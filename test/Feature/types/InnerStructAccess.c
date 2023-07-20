// RUN: %clang %s -emit-llvm -g -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --use-advanced-type-system --use-tbaa --use-lazy-initialization=none --use-gep-opt %t.bc 2>&1 | FileCheck %s

#include "klee/klee.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

typedef struct A {
  int32_t x;
} A;

typedef struct B {
  A a;
  float y;
} B;

int main() {
  B b;

  int32_t *ptr;
  klee_make_symbolic(&ptr, sizeof(ptr), "ptr");
  *ptr = 10;

  // CHECK: x
  if ((void *)ptr == (void *)&b) {
    printf("x\n");
    return 0;
  }

  int16_t *short_ptr;
  klee_make_symbolic(&short_ptr, sizeof(short_ptr), "short_ptr");
  *short_ptr = 12;

  // CHECK-NOT: ASSERTION FAIL
  assert((void *)short_ptr < (void *)&b || (void *)short_ptr >= (void *)(&b + 1));

  return 1;
}
