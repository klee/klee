// RUN: %clang %s -fsanitize=returns-nonnull-attribute -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --emit-all-errors --ubsan-runtime %t.bc 2>&1 | FileCheck %s
// RUN: ls %t.klee-out/ | grep .ktest | wc -l | grep 2
// RUN: ls %t.klee-out/ | grep .nullable_attribute.err | wc -l | grep 1

#include "klee/klee.h"

__attribute__((returns_nonnull)) char *foo(char *a) {
  // CHECK: KLEE: ERROR: {{.*}}runtime/Sanitizer/ubsan/ubsan_handlers.cpp:{{[0-9]+}}: invalid-null-return
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