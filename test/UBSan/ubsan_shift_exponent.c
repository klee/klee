// RUN: %clang %s -fsanitize=shift-exponent -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --emit-all-errors --ubsan-runtime --check-overshift=false %t.bc 2>&1 | FileCheck %s
// RUN: ls %t.klee-out/ | grep .ktest | wc -l | grep 2
// RUN: ls %t.klee-out/ | grep .overflow.err | wc -l | grep 1

#include "klee/klee.h"

int rsh_inbounds(signed int a, signed int b) {
  // CHECK: KLEE: ERROR: {{.*}}runtime/Sanitizer/ubsan/ubsan_handlers.cpp:{{[0-9]+}}: shift out of bounds
  return a >> b;
}

int main() {
  signed int a;
  signed int b;
  volatile signed int result;

  klee_make_symbolic(&a, sizeof(a), "a");
  klee_make_symbolic(&b, sizeof(b), "b");

  result = rsh_inbounds(a, b);

  return 0;
}
