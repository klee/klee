// REQUIRES: not-msan
// Disabling msan because it times out on CI
// REQUIRES: libcxx
// REQUIRES: uclibc
// RUN: %clangxx %s -emit-llvm %O0opt -c -std=c++11 %libcxx_includes -g -nostdinc++ -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --libc=uclibc --libcxx %t1.bc 2>&1 | FileCheck %s

#include "klee/klee.h"
#include <iostream>

int main(int argc, char **args) {
  int x = klee_int("x");
  if (x > 0) {
    std::cout << "greater: " << x << std::endl;
    // CHECK-DAG: greater
  } else {
    std::cout << "lower: " << x << std::endl;
    // CHECK-DAG: lower
  }
  return 0;
}
