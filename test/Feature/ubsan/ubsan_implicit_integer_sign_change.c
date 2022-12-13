// REQUIRES: geq-llvm-8.0
// RUN: %clang %s -fsanitize=implicit-integer-sign-change -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out %t.bc 2>&1 | FileCheck %s

#include "klee/klee.h"

signed int convert_unsigned_int_to_signed_int(unsigned int x) {
  // CHECK: ubsan_implicit_integer_sign_change.c:[[@LINE+1]]: invalid implicit conversion
  return x;
}

int main() {
  unsigned int x;
  volatile signed int result;

  klee_make_symbolic(&x, sizeof x, "x");

  result = convert_unsigned_int_to_signed_int(x);
  return 0;
}
