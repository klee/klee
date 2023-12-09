// RUN: %clang %s -g -emit-llvm %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --search=dfs --output-dir=%t.klee-out --single-object-resolution %t.bc > %t.log 2>&1
// RUN: FileCheck %s -input-file=%t.log

#include "klee/klee.h"
#include <stdlib.h>

struct A {
  long long int y;
  long long int y2;
  int z;
};

struct B {
  long long int x;
  struct A y[20];
  struct A *y1;
  struct A *y2;
  int z;
};

int bar(int *pointer, int selector) {
  int *ptr = 0;
  if (selector)
    ptr = pointer + 123;
  else
    ptr = pointer + 124;
  // CHECK: SingleObjectResolution.c:[[@LINE+1]]: memory error: out of bound pointer
  return *ptr;
}

int foo() {
  size_t x;
  int y;
  struct B b;

  // create a lot of memory objects
  int *ptrs[1024];
  for (int i = 0; i < 1024; i++) {
    ptrs[i] = malloc(23);
  }

  klee_make_symbolic(&x, sizeof(x), "x");
  klee_make_symbolic(&y, sizeof(y), "y");

  b.y1 = malloc(20 * sizeof(struct A));

  // dereference of a pointer within a struct
  int *tmp = &b.y1[x].z;

  int z = bar(tmp, y);
  // cleanup test for heap
  free(b.y1);

  tmp = &b.y[x].z; // this is to test the cleanup for stack vars
  z = bar(tmp, y);
  return z;
}

int main(int argc, char *argv[]) {
  // CHECK: KLEE: done: completed paths = 2
  // CHECK: KLEE: done: partially completed paths = 2
  // CHECK: KLEE: done: generated tests = 3
  return foo();
}
