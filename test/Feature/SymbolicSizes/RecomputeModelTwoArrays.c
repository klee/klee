// RUN: %clang %s -g -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --check-out-of-memory --use-sym-size-alloc --use-merged-pointer-dereference=true %t1.bc 2>&1 | FileCheck %s

#include "klee/klee.h"
#include <stdlib.h>

int main() {
  int n = klee_int("n");
  if (n >= 5) {
    char *c1 = malloc(n);
    char *c2 = malloc(9 - n);
    if (n > 7) {
      // CHECK: RecomputeModelTwoArrays.c:[[@LINE+1]]: memory error: out of bound pointer
      c2[0] = 111;
      return 1;
    }
  }
}
// CHECK: KLEE: done: generated tests = 5
