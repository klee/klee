// RUN: %clang %s -fsanitize=builtin -w -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --emit-all-errors --ubsan-runtime %t.bc 2>&1 | FileCheck %s

#include "klee/klee.h"

int main() {
  signed int x;

  klee_make_symbolic(&x, sizeof(x), "x");

  // CHECK: runtime/Sanitizer/ubsan/ubsan_handlers.cpp:35: invalid-builtin-use
  __builtin_ctz(x);
  return 0;
}
