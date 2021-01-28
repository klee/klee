// RUN: %clang %s -fsanitize=vla-bound -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --emit-all-errors --ubsan-runtime %t.bc 2>&1 | FileCheck %s

#include "klee/klee.h"

int main() {
  int x;
  volatile int result;

  x = klee_range(-10, 10, "x");

  // CHECK: runtime/Sanitizer/ubsan/ubsan_handlers.cpp:35: non-positive-vla-index
  int arr[x];
  result = arr[0];
  return 0;
}
