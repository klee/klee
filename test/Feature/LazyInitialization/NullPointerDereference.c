// RUN: %clang %s -emit-llvm -g -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --skip-local --skip-global %t.bc 2>&1 | FileCheck %s

#include "klee/klee.h"

int main() {
  int *x;
  klee_make_symbolic(&x, sizeof(x), "*x");
  if ((uintptr_t)x < (uintptr_t)10) {
    // CHECK: NullPointerDereference.c:[[@LINE+1]]: memory error: null pointer exception
    x[0] = 20;
  } else {
    // CHECK: NullPointerDereference.c:[[@LINE+1]]: memory error: null pointer exception
    x[0] = 30;
  }

  // CHECK-NOT: NullPointerDereference.c:[[@LINE+1]]: memory error: null pointer exception
  x[1] = 40;
}

// CHECK: KLEE: done: completed paths = 2
// CHECK: KLEE: done: generated tests = 7
