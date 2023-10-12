// REQUIRES: not-msan
// Disabling msan because it times out on CI
// REQUIRES: libcxx
// REQUIRES: eh-cxx
// RUN: %clangxx %s -emit-llvm %O0opt -c -std=c++11 %libcxx_includes -g -nostdinc++ -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --libc=uclibc --libcxx %t1.bc 2>&1 | FileCheck %s

#include "klee/klee.h"
#include <cstdio>

int main(int argc, char **args) {
  int x = klee_int("Test");

  try {
    if (x % 2 == 0) {
      printf("Normal return\n");
      // CHECK-DAG: Normal return
      return 0;
    }
    throw x;
  } catch (int b) {
    if (b % 3 == 0) {
      printf("First exceptional return\n");
      // CHECK-DAG: First exceptional return
      return 2;
    }
    printf("Second exceptional return\n");
    // CHECK-DAG: Second exceptional return
    return 3;
  }
}

// CHECK-DAG: KLEE: done: completed paths = 3
