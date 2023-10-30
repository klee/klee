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
    try {
      char *p = new char[8];
      throw p;
    } catch (int ex) {
      printf("Landed in wrong inner catch\n");
      // CHECK-NOT: Landed in wrong inner catch
    }
  } catch (int ex) {
    printf("Landed in wrong outer catch\n");
    // CHECK-NOT: Landed in wrong outer catch
  }
  return 0;
}
// CHECK: terminating {{.*}} uncaught exception of type char*
