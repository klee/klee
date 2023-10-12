// RUN: %clang %s -g -emit-llvm %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --single-object-resolution %t.bc > %t.log 2>&1
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

int foo(int *pointer) {
  //printf("pointer is called\n");
  int *ptr = pointer + 123;
  return *ptr;
}

int main(int argc, char *argv[]) {

  int x;
  struct B b;

  // create a lot of memory objects
  int *ptrs[1024];
  for (int i = 0; i < 1024; i++) {
    ptrs[i] = malloc(23);
  }

  klee_make_symbolic(&x, sizeof(x), "x");

  b.y1 = malloc(20 * sizeof(struct A));

  // dereference of a pointer within a struct
  int *tmp = &b.y1[x].z;

  // CHECK: SingleObjectResolution.c:26: memory error: out of bound pointer
  // CHECK: KLEE: done: completed paths = 1
  // CHECK: KLEE: done: partially completed paths = 1
  // CHECK: KLEE: done: generated tests = 2
  return foo(tmp);
}
