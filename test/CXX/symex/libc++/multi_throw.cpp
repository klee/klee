// REQUIRES: not-msan
// Disabling msan because it times out on CI
// REQUIRES: libcxx
// REQUIRES: eh-cxx
// RUN: %clangxx %s -emit-llvm %O0opt -c -std=c++11 %libcxx_includes -g -nostdinc++ -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --libc=uclibc --libcxx %t1.bc 2>&1 | FileCheck %s

#include "klee/klee.h"
#include <cstdio>

static void x(int a) {
  if (a == 7) {
    throw static_cast<float>(a);
  }
}

struct ThrowingDestructor {
  int p = 0;
  ~ThrowingDestructor() {
    try {
      x(p);
    } catch (float ex) {
      printf("Caught float in ThrowingDestructor()\n");
      // CHECK-DAG: Caught float in ThrowingDestructor()
    }
  }
};

static void y(int a) {
  ThrowingDestructor p;
  p.p = klee_int("struct_int");
  try {
    if (a == 7) {
      throw a;
    }
  } catch (int ex) {
    printf("Caught int in y()\n");
    // CHECK-DAG: Caught int in y()
  }
}

int main(int argc, char **args) {
  y(klee_int("fun_int"));
  return 0;
}

// CHECK-DAG: KLEE: done: completed paths = 4
