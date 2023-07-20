// RUN: %clang %s -emit-llvm -g -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --use-advanced-type-system --use-tbaa --use-lazy-initialization=none --use-gep-opt %t.bc 2>&1 | FileCheck %s

#include "klee/klee.h"
#include <assert.h>
#include <stdio.h>

typedef struct Node {
  struct Node *next;
} Node;

int main() {
  int a = 100;
  float b = 2.0;
  long long c = 120;
  double d = 11;
  Node node;
  Node *node_ptr;

  int *pointer;
  klee_make_symbolic(&pointer, sizeof(pointer), "pointer");
  *pointer = 1;

  // CHECK-NOT: ASSERTION FAIL
  assert((void *)pointer != (void *)&pointer);

  // CHECK-NOT: ASSERTION FAIL
  assert((void *)pointer != (void *)&b);

  // CHECK-NOT: ASSERTION FAIL
  assert((void *)pointer != (void *)&c);

  // CHECK-NOT: ASSERTION FAIL
  assert((void *)pointer != (void *)&d);

  // CHECK-NOT: ASSERTION FAIL
  assert((void *)pointer != (void *)&node);

  if ((void *)pointer == (void *)&a) {
    // CHECK-NOT: ASSERTION FAIL
    assert(a == 1);
  }

  char *anyPointer;
  klee_make_symbolic(&anyPointer, sizeof(anyPointer), "anyPointer");
  *anyPointer = '0';
  // CHECK-DAG: a
  // CHECK-DAG: b
  // CHECK-DAG: c
  // CHECK-DAG: d
  if ((void *)anyPointer == (void *)&a) {
    printf("a\n");
    return 0;
  } else if ((void *)anyPointer == (void *)&b) {
    printf("b\n");
    return 1;
  } else if ((void *)anyPointer == (void *)&c) {
    printf("c\n");
    return 2;
  } else if ((void *)anyPointer == (void *)&d) {
    printf("d\n");
    return 3;
  }
  return 4;
}
