// RUN: %clang %s -fsanitize=nonnull-attribute -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --emit-all-errors --ubsan-runtime %t.bc 2>&1 | FileCheck %s

#include "klee/klee.h"

__attribute__((nonnull)) int func(int *nonnull) { return *nonnull; }

int main() {
  _Bool null;
  volatile int result;

  klee_make_symbolic(&null, sizeof(null), "null");

  int local = 0;
  int *arg = null ? 0x0 : &local;

  // CHECK: runtime/Sanitizer/ubsan/ubsan_handlers.cpp:35: invalid-null-argument
  result = func(arg);
  return 0;
}
