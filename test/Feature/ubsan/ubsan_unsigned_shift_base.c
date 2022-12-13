// REQUIRES: geq-llvm-12.0

// RUN: %clang %s -fsanitize=unsigned-shift-base -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out %t.bc 2>&1 | FileCheck %s

#include "klee/klee.h"

int lsh_overflow(unsigned int a, unsigned int b) {
  // CHECK: ubsan_unsigned_shift_base.c:[[@LINE+1]]: shifted value is invalid
  return a << b;
}

int main() {
  unsigned int a;
  unsigned int b;
  volatile unsigned int result;

  klee_make_symbolic(&a, sizeof(a), "a");
  klee_make_symbolic(&b, sizeof(b), "b");

  result = lsh_overflow(a, b);

  return 0;
}
