// RUN: %clang %s -fsanitize=builtin -w -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --emit-all-errors --ubsan-runtime %t.bc 2>&1 | FileCheck %s
// RUN: ls %t.klee-out/ | grep .ktest | wc -l | grep 2
// RUN: ls %t.klee-out/ | grep .invalid_builtin_use.err | wc -l | grep 1

#include "klee/klee.h"

int main() {
  signed int x;

  klee_make_symbolic(&x, sizeof(x), "x");

  // CHECK: KLEE: ERROR: {{.*}}runtime/Sanitizer/ubsan/ubsan_handlers.cpp:{{[0-9]+}}: invalid-builtin-use
  __builtin_ctz(x);
  return 0;
}
