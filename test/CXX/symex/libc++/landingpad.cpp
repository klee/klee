// Testcase for proper handling of
// c++ type, constructors and destructors.
// Based on: https://gcc.gnu.org/wiki/Dwarf2EHNewbiesHowto
// REQUIRES: uclibc
// REQUIRES: libcxx
// REQUIRES: eh-cxx
// RUN: %clangxx %s -emit-llvm %O0opt -std=c++11 -c %libcxx_includes -g -nostdinc++ -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --exit-on-error --libcxx --libc=uclibc  %t.bc | FileCheck %s
// Expect the following output:
// CHECK: Throwing 1...
// CHECK: A() 1
// CHECK: A(const A&) 2
// CHECK: Caught.
// CHECK: ~A() 2
// CHECK: ~A() 1
// CHECK: c == 2, d == 2

#include <stdio.h>

int c, d;

struct A {
  int i;
  A() {
    i = ++c;
    printf("A() %d\n", i);
  }
  A(const A &) {
    i = ++c;
    printf("A(const A&) %d\n", i);
  }
  ~A() {
    printf("~A() %d\n", i);
    ++d;
  }
};

void f() {
  printf("Throwing 1...\n");
  throw A();
}

int main() {
  c = 0;
  d = 0;
  try {
    f();
  } catch (A) {
    printf("Caught.\n");
  }
  printf("c == %d, d == %d\n", c, d);
  return c != d;
}
