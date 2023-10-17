// RUN: %clang %s -g -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --use-sym-size-alloc %t1.bc 2>&1 | FileCheck %s

#include "klee/klee.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

int main() {
  int n;
  klee_make_symbolic(&n, sizeof(n), "n");
  if (n >= 5) {
    char *c1 = malloc(n);
    char *c2 = malloc(9 - n);
    // CHECK: LowerOutOfBound.c:[[@LINE+1]]: memory error: out of bound pointer
    c2[9 - n - 1] = 20;
  } else {
    n = 0;
  }
}

// KLEE: done: completed paths = 2
// KLEE: done: generated tests = 3
