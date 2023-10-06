// RUN: %clang %s -emit-llvm -g -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --skip-not-symbolic-objects %t1.bc 2>&1 | FileCheck %s

#include "klee/klee.h"
#include <assert.h>
#include <stdlib.h>

int *make_ptr() {
  int **x = malloc(sizeof(int *));
  klee_make_symbolic(x, sizeof(int *), "*x");
  return *x;
}

int main() {
  int *x = make_ptr();
  int *y = make_ptr();

  // CHECK: TwoObjectsInitialization.c:[[@LINE+1]]: memory error: null pointer exception
  *x = 10;
  // CHECK: TwoObjectsInitialization.c:[[@LINE+1]]: memory error: null pointer exception
  *y = 20;

  if (*x == 20) {
    // One object

    // CHECK-NOT: TwoObjectsInitialization.c:[[@LINE+1]]: memory error: out of bound pointer
    *x = 30;
    // CHECK-NOT: TwoObjectsInitialization.c:[[@LINE+2]]: memory error: out of bound pointer
    // CHECK-NOT: TwoObjectsInitialization.c:[[@LINE+1]]: ASSERTION FAIL
    assert(*x == *y && *y == 30);

  } else if (*x == 10) {
    // Two LI objects

    // CHECK-NOT: TwoObjectsInitialization.c:[[@LINE+2]]: memory error: out of bound pointer
    // CHECK-NOT: TwoObjectsInitialization.c:[[@LINE+1]]: ASSERTION FAIL
    assert(*y == 20);
  } else {
    // CHECK: TwoObjectsInitialization.c:[[@LINE+1]]: ASSERTION FAIL
    assert(0 && "x points to itself!");
  }
  return 0;
}
