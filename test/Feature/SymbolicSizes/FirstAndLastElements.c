// RUN: %clang %s -g -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --check-out-of-memory --use-sym-size-alloc --use-merged-pointer-dereference=true --max-sym-alloc=128 %t1.bc 2>&1 | FileCheck %s

#include "klee/klee.h"
#include <assert.h>
#include <stdlib.h>

int main() {
  int n = klee_int("n");
  char *s = (char *)malloc(n);
  // CHECK: FirstAndLastElements.c:[[@LINE+1]]: ASSERTION FAIL
  assert(s);

  klee_make_symbolic(s, n, "s");
  // CHECK: FirstAndLastElements.c:[[@LINE+1]]: memory error: out of bound pointer
  int sum = s[n - 1] + s[0];
  // CHECK: FirstAndLastElements.c:[[@LINE+1]]: ASSERTION FAIL
  assert(sum % 2 == 1);
}

// CHECK: KLEE: done: completed paths = 1
// CHECK: KLEE: done: generated tests = 4
