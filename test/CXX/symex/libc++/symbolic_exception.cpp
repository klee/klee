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
  try {
    throw klee_int("x");
  } catch (int ex) {
    if (ex > 7) {
      puts("ex > 7");
      // CHECK-DAG: ex > 7
    } else if (ex < 2) {
      puts("ex < 2");
      // CHECK-DAG: ex < 2
    } else {
      puts("2 <= ex <= 7");
      // CHECK-DAG: 2 <= ex <= 7
    }
  }
  return 0;
}

// CHECK-DAG: KLEE: done: completed paths = 3
