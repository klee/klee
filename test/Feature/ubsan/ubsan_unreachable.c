// RUN: %clang %s -fsanitize=unreachable -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out %t.bc 2>&1 | FileCheck %s

#include "klee/klee.h"

void _Noreturn f() {
  // CHECK: ubsan_unreachable.c:12: execution reached an unreachable program point
}

int main() {
  f();

  return 0;
}
