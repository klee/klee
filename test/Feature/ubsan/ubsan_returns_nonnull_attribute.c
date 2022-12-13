// RUN: %clang %s -fsanitize=returns-nonnull-attribute -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out %t.bc 2>&1 | FileCheck %s

#include "klee/klee.h"

__attribute__((returns_nonnull)) char *foo(char *a) {
  // CHECK: ubsan_returns_nonnull_attribute.c:[[@LINE+1]]: null pointer returned from function declared to never return null
  return a;
}

int main() {
  _Bool null;

  klee_make_symbolic(&null, sizeof null, "null");

  char local = 0;
  char *arg = null ? 0x0 : &local;
  volatile char *result = foo(arg);
  return 0;
}
