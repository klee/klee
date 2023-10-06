// RUN: %clang %s -emit-llvm -g -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --skip-not-symbolic-objects %t.bc 2>&1 | FileCheck %s

#include "klee/klee.h"

int main() {
  int *x;
  klee_make_symbolic(&x, sizeof(x), "*x");
  if ((uintptr_t)x < (uintptr_t)10) {
    // CHECK: ImpossibleAddressForLI.c:[[@LINE+1]]: memory error: null pointer exception
    *x = 20;
  }
  // CHECK-NOT: ImpossibleAddressForLI.c:[[@LINE+1]]: memory error: null pointer exception
  *x = 30;
}

// CHECK: KLEE: done: completed paths = 1
// CHECK: KLEE: done: generated tests = 3
