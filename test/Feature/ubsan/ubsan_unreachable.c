// RUN: %clang %s -fsanitize=unreachable -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out %t.bc 2>&1 | FileCheck %s %c_prefixes_6

#include "klee/klee.h"

void _Noreturn f() {
  // CHECK-6: ubsan_unreachable.c:13: execution reached an unreachable program point
  // CHECK: ubsan_unreachable.c:13: reached "unreachable" instruction
}

int main() {
  f();

  return 0;
}
