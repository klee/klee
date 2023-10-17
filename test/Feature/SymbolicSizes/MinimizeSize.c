// RUN: %clang %s -g -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --check-out-of-memory --use-sym-size-alloc --use-merged-pointer-dereference=true %t1.bc 2>&1 | FileCheck %s

#include "klee/klee.h"
#include <stdlib.h>

int main() {
  int n = klee_int("n");
  if (n >= 3 && n != 4) {
    char *storage = (char *)malloc(n);
    storage[0] = 'a';
    storage[1] = 'b';
    storage[2] = 'c';
    // CHECK: MinimizeSize.c:[[@LINE+1]]: memory error: out of bound pointer
    storage[3] = 'd';
  }
}

// CHECK: KLEE: done: completed paths = 2
// CHECK: KLEE: done: partially completed paths = 2
// CHECK: KLEE: done: generated tests = 4
