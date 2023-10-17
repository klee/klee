// RUN: %clang %s -g -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --check-out-of-memory --use-sym-size-alloc --use-merged-pointer-dereference=true %t1.bc 2>&1 | FileCheck %s

#include "klee/klee.h"
#include "stdlib.h"
#include <stdio.h>

int main() {
  int n = klee_int("n");

  char *s1 = (char *)malloc(n);
  // CHECK: NegativeIndexArray.c:[[@LINE+2]]: memory error: null pointer exception
  // CHECK: NegativeIndexArray.c:[[@LINE+1]]: memory error: out of bound pointer
  s1[-1] = 10;
}

// CHECK: KLEE: done: completed paths = 0
// CHECK: KLEE: done: partially completed paths = 2
// CHECK: KLEE: done: generated tests = 2
