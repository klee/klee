// RUN: %clang %s -fsanitize=unreachable -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --emit-all-errors --ubsan-runtime %t.bc 2>&1 | FileCheck %s

#include "klee/klee.h"

void _Noreturn f() {
  // CHECK: runtime/Sanitizer/ubsan/ubsan_handlers.cpp:35: unreachable-call
  __builtin_unreachable();
}

int main() {
  f();

  return 0;
}
