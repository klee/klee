// RUN: %clang %s -fsanitize=unreachable -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --emit-all-errors --ubsan-runtime %t.bc 2>&1 | FileCheck %s
// RUN: ls %t.klee-out/ | grep .ktest | wc -l | grep 1
// RUN: ls %t.klee-out/ | grep .unreachable_call.err | wc -l | grep 1

#include "klee/klee.h"

void _Noreturn f() {
  // CHECK: KLEE: ERROR: {{.*}}runtime/Sanitizer/ubsan/ubsan_handlers.cpp:{{[0-9]+}}: unreachable-call
  __builtin_unreachable();
}

int main() {
  f();

  return 0;
}
