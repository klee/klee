// RUN: %clang %s -g -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --check-out-of-memory --use-sym-size-alloc --skip-not-lazy-initialized --use-merged-pointer-dereference=true %t1.bc 2>&1 | FileCheck %s

#include "klee/klee.h"
#include <stdio.h>
#include <stdlib.h>

struct Node {
  int x;
  struct Node *next;
};

void foo(void *s) {
  // CHECK: VoidStar.c:[[@LINE+1]]: memory error: out of bound pointer
  ((int *)s)[1] = 10;
  // CHECK-NOT: VoidStar.c:[[@LINE+1]]: memory error: out of bound pointer
  ((char *)s)[1] = 'a';
  // CHECK: VoidStar.c:[[@LINE+1]]: memory error: out of bound pointer
  struct Node *node = ((struct Node *)s)->next;
  // CHECK: VoidStar.c:[[@LINE+1]]: memory error: null pointer exception
  node->x = 20;
}

int main() {
  int n = klee_int("n");
  void *s = malloc(n);
  // KLEE: ERROR: VoidStar.c:28: memory error: invalid pointer: make_symbolic
  klee_make_symbolic(s, n, "s");
  foo(s);
}

// CHECK: KLEE: done: completed paths = 1
// CHECK: KLEE: done: partially completed paths = 4
// CHECK: KLEE: done: generated tests = 5
