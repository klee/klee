// REQUIRES: uclibc
// RUN: %clangxx %s -emit-llvm %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --libc=uclibc %t.bc 2>&1 | FileCheck %s

// CHECK: KLEE: done: completed paths = 5

#include "klee/klee.h"

class Test {
  int x;

public:
  Test(int _x) : x(_x) {}

  int test() {
    if (x % 2 == 0)
      return 0;
    if (x % 3 == 0)
      return 1;
    if (x % 5 == 0)
      return 2;
    if (x % 7 == 0)
      return 3;
    return 4;
  }
};

int main(int argc, char **args) {
  Test a(klee_int("Test"));

  return a.test();
}
