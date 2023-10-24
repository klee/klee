// REQUIRES: not-darwin, not-metasmt
// Disabling darwin and metasmt because test has flaky behaviour on CI
// RUN: %clang %s -g -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --use-sym-size-alloc --use-sym-size-li --min-number-elements-li=1 --skip-not-lazy-initialized %t1.bc 2>&1 | FileCheck %s

#include "klee/klee.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

struct Node {
  int x;
  struct Node *next;
};

void foo(int *array) {
  // CHECK: IntArray.c:[[@LINE+1]]: memory error: null pointer exception
  if (array[0] == 0) {
    return;
  }
  // CHECK-COUNT-2: IntArray.c:[[@LINE+2]]: memory error: out of bound pointer
  // CHECK: IntArray.c:[[@LINE+1]]: ASSERTION FAIL
  assert(array[1] + array[2] == 2);
}

int main() {
  int *ptr;
  klee_make_symbolic(&ptr, sizeof(ptr), "ptr");
  foo(ptr);
}

// CHECK: KLEE: done: completed paths = 2
