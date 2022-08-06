// RUN: %clang %s -fsanitize=bool -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --emit-all-errors --ubsan-runtime %t.bc 2>&1 | FileCheck %s
// RUN: ls %t.klee-out/ | grep .ktest | wc -l | grep 2
// RUN: ls %t.klee-out/ | grep .invalid_load.err | wc -l | grep 1

#include "klee/klee.h"

int main() {
  unsigned char x;
  volatile _Bool result;

  klee_make_symbolic(&x, sizeof(x), "x");

  // CHECK: KLEE: ERROR: {{.*}}runtime/Sanitizer/ubsan/ubsan_handlers.cpp:{{[0-9]+}}: load invalid value
  result = *(_Bool *)&x;

  return 0;
}
