// RUN: %clang %s -fsanitize=float-cast-overflow -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out %t.bc 2>&1 | FileCheck %s
// REQUIRES: floating-point
#include "klee/klee.h"

int float_int_overflow(float f) {
  // CHECK: ubsan_float_cast_overflow.c:[[@LINE+1]]: floating point value is outside the range of representable values
  return f;
}

int long_double_int_overflow(long double ld) {
  // CHECK: ubsan_float_cast_overflow.c:[[@LINE+1]]: floating point value is outside the range of representable values
  return ld;
}

int main() {
  float f;

  klee_make_symbolic(&f, sizeof f, "f");
  float_int_overflow(f);

  long double ld;
  klee_make_symbolic(&ld, sizeof ld, "ld");
  long_double_int_overflow(ld);

  return 0;
}
