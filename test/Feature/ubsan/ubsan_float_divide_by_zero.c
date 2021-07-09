// RUN: %clang %s -fsanitize=float-divide-by-zero -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out %t.bc 2>&1 | FileCheck %s
// REQUIRES: floating-point
#include "klee/klee.h"

int main() {
  float DIVIDEND;
  klee_make_symbolic(&DIVIDEND, sizeof DIVIDEND, "DIVIDEND");
  klee_assume(DIVIDEND != 0.0);
  // CHECK: ubsan_float_divide_by_zero.c:[[@LINE+1]]: overflow on division or remainder
  volatile float result = DIVIDEND / 0;
}
