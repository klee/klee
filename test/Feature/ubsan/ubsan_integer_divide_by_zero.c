// RUN: %clang %s -fsanitize=integer-divide-by-zero -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --emit-all-errors --ubsan-runtime --check-div-zero=false %t.bc 2>&1 | FileCheck %s
// RUN: ls %t.klee-out/ | grep .ktest | wc -l | grep 1
// RUN: ls %t.klee-out/ | grep .overflow.err | wc -l | grep 1

#include "klee/klee.h"

int main() {
  int x;
  volatile int result;

  klee_make_symbolic(&x, sizeof(x), "x");

  // CHECK: KLEE: ERROR: {{.*}}runtime/Sanitizer/ubsan/ubsan_handlers.cpp:{{[0-9]+}}: integer division overflow
  result = x / 0;
  return 0;
}
