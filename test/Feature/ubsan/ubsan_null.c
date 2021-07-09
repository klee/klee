// RUN: %clang %s -fsanitize=null -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out %t.bc 2>&1 | FileCheck %s

#include "klee/klee.h"

int main() {
  _Bool null;

  klee_make_symbolic(&null, sizeof null, "null");

  int local = 0;
  int *arg = null ? 0x0 : &local;

  // CHECK: ubsan_null.c:[[@LINE+1]]: invalid usage of null pointer
  volatile int result = *arg;
  return 0;
}
