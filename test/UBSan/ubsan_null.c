// RUN: %clang %s -fsanitize=null -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --emit-all-errors --ubsan-runtime %t.bc 2>&1 | FileCheck %s
// RUN: ls %t.klee-out/ | grep .ktest | wc -l | grep 2
// RUN: ls %t.klee-out/ | grep .ptr.err | wc -l | grep 1

#include "klee/klee.h"

int main() {
  _Bool null;
  volatile int result;

  klee_make_symbolic(&null, sizeof(null), "null");

  int local = 0;
  int *arg = null ? 0x0 : &local;

  // CHECK: KLEE: ERROR: {{.*}}runtime/Sanitizer/ubsan/ubsan_handlers.cpp:{{[0-9]+}}: null-pointer-use
  result = *arg;
  return 0;
}
