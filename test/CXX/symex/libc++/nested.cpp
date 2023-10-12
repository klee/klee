// REQUIRES: not-msan
// Disabling msan because it times out on CI
// REQUIRES: libcxx
// REQUIRES: eh-cxx
// RUN: %clangxx %s -emit-llvm %O0opt -c -std=c++11 %libcxx_includes -g -nostdinc++ -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --exit-on-error --libc=uclibc --libcxx %t1.bc 2>&1 | FileCheck %s

#include <cstdio>

int main(int argc, char **args) {
  try {
    try {
      char *p = new char[8];
      throw p;
    } catch (char *ex) {
      int i = 7;
      throw i;
    }
  } catch (int ex) {
    printf("Reached outer catch with i = %d\n", ex);
    // CHECK-DAG: Reached outer catch with i = 7
  }
  return 0;
}

// CHECK-DAG: KLEE: done: completed paths = 1
