// RUN: %clang %s -fsanitize=nullability-return -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --emit-all-errors --ubsan-runtime %t.bc 2>&1 | FileCheck %s

#include "klee/klee.h"

int *_Nonnull nonnull_retval(int *p) {
  // CHECK: runtime/Sanitizer/ubsan/ubsan_handlers.cpp:35: invalid-null-return
  return p;
}

int main() {
  _Bool null;
  volatile int *result;

  klee_make_symbolic(&null, sizeof(null), "null");

  int local = 0;
  int *arg = null ? 0x0 : &local;

  result = nonnull_retval(arg);
  return 0;
}
