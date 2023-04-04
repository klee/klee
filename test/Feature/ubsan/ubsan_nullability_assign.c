// RUN: %clang %s -fsanitize=nullability-assign -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --emit-all-errors --ubsan-runtime %t.bc 2>&1 | FileCheck %s
// RUN: ls %t.klee-out/ | grep .ktest | wc -l | grep 2
// RUN: ls %t.klee-out/ | grep .ptr.err | wc -l | grep 1

#include "klee/klee.h"

void nonnull_assign(int *p) {
  volatile int *_Nonnull local;
  // CHECK: KLEE: ERROR: {{.*}}runtime/Sanitizer/ubsan/ubsan_handlers.cpp:{{[0-9]+}}: null-pointer-use
  local = p;
}

int main() {
  _Bool null;

  klee_make_symbolic(&null, sizeof(null), "null");

  int local = 0;
  int *arg = null ? 0x0 : &local;

  nonnull_assign(arg);
  return 0;
}
