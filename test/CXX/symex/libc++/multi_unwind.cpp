// REQUIRES: not-msan
// Disabling msan because it times out on CI
// REQUIRES: libcxx
// REQUIRES: eh-cxx
// RUN: %clangxx %s -emit-llvm %O0opt -c -std=c++11 %libcxx_includes -g -nostdinc++ -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --libc=uclibc --libcxx %t1.bc 2>&1 | FileCheck %s

#include "klee/klee.h"
#include <cassert>
#include <cstdio>

struct CustomStruct {
  int a;
};

int thrower(int x) {
  if (x == 0) {
    CustomStruct p;
    throw p;
  } else if (x == 1) {
    throw 7;
  } else {
    throw new CustomStruct();
  }
}
int rethrower(int x) {
  thrower(x);
  return 7;
}

int failed_catch(int x) {
  try {
    rethrower(x);
  } catch (char *ex) {
    // Don't catch wrong type
    assert(0);
  }
  // rethrower always throws, cannot reach this instruction
  assert(0);
  return 0;
}

int catcher(int x) {
  try {
    failed_catch(x);
  } catch (int ex) {
    printf("Caught int\n");
    // CHECK-DAG: Caught int
    return 1;
  } catch (CustomStruct ex) {
    printf("Caught normal CustomStruct\n");
    // CHECK-DAG: Caught normal CustomStruct
    return 2;
  } catch (CustomStruct *ex) {
    printf("Caught pointer CustomStruct\n");
    // CHECK-DAG: Caught pointer CustomStruct
    return 3;
  }
  // Unreachable instruction
  assert(0);
  return 0;
}

int main(int argc, char **args) {
  int x = klee_int("x");
  int res = catcher(x);
  return res;
}

// CHECK-DAG: KLEE: done: completed paths = 3
