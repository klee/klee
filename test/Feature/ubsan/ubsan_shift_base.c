// RUN: %clang %s -fsanitize=shift-base -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --emit-all-errors --ubsan-runtime --check-overshift=false %t.bc 2>&1 | FileCheck %s
//
// There may be 2 or 3 test cases depending on the search heuristic, so we don't check the number of tests.
// For example, 2 test cases may be as follows:
// (1) b <= 31 without overflow, (2) a > 0 and b > 31 with overflow
// For example, 3 test cases may be as follows:
// (1) a = 0 and b > 31 without overflow, (2) b < 31 without overflow, (3) a > 0 and b > 31 with overflow
//
// RUN: ls %t.klee-out/ | grep .overflow.err | wc -l | grep 1

#include "klee/klee.h"

int lsh_overflow(signed int a, signed int b) {
  // CHECK: KLEE: ERROR: {{.*}}runtime/Sanitizer/ubsan/ubsan_handlers.cpp:{{[0-9]+}}: shift out of bounds
  return a << b;
}

int main() {
  signed int a;
  signed int b;
  volatile signed int result;

  klee_make_symbolic(&a, sizeof(a), "a");
  klee_make_symbolic(&b, sizeof(b), "b");

  result = lsh_overflow(a, b);

  return 0;
}
