// REQUIRES: uclibc
// RUN: %clangxx %s -emit-llvm %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --libc=uclibc %t.bc 2>&1 | FileCheck %s

#include <cstdio>

struct A {
  int a;
  A() {
    printf("A constructor\n");
  }
};

struct B : virtual A {
  int b;
  B() {
    printf("B constructor\n");
  }
};

struct C : virtual A {
  int c;
  C() {
    printf("C constructor\n");
  }
};

struct D : B, C {
  int d;
};

int main(int argc, char **args) {
  D x;
  // CHECK: A constructor
  // CHECK: B constructor
  // CHECK: C constructor

  x.a = 7;
  B *y = &x;
  printf("B::a = %d\n", y->a);
  // CHECK: B::a = 7

  return 0;
}
