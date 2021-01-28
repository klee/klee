// RUN: %clang %s -fsanitize=returns-nonnull-attribute -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --emit-all-errors --ubsan-runtime %t.bc 2>&1 | FileCheck %s

#include "klee/klee.h"

__attribute__((returns_nonnull)) char *foo(char *a) {
  // CHECK: runtime/Sanitizer/ubsan/ubsan_handlers.cpp:35: invalid-null-return
  return a;
}

int main() {
  _Bool null;
  volatile char *result;

  klee_make_symbolic(&null, sizeof(null), "null");

  char local = 0;
  char *arg = null ? 0x0 : &local;
  result = foo(arg);
  return 0;
}