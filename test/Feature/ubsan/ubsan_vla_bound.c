// RUN: %clang %s -fsanitize=vla-bound -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --emit-all-errors --ubsan-runtime %t.bc 2>&1 | FileCheck %s
// RUN: ls %t.klee-out/ | grep .ktest | wc -l | grep 3
// RUN: ls %t.klee-out/ | grep .ptr.err | wc -l | grep 1
// RUN: ls %t.klee-out/ | grep .model.err | wc -l | grep 1

#include "klee/klee.h"

int main() {
  int x;
  volatile int result;

  x = klee_range(-10, 10, "x");

  // CHECK: KLEE: ERROR: {{.*}}runtime/Sanitizer/ubsan/ubsan_handlers.cpp:{{[0-9]+}}: non-positive-vla-index
  int arr[x];
  result = arr[0];
  return 0;
}
