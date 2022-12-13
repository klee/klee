// REQUIRES: geq-llvm-8.0
// RUN: %clang %s -fsanitize=implicit-signed-integer-truncation -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out %t.bc 2>&1 | FileCheck %s

#include "klee/klee.h"

unsigned char convert_signed_int_to_unsigned_char(signed int x) {
  // CHECK: ubsan_implicit_signed_integer_truncation.c:[[@LINE+1]]: invalid implicit conversion
  return x;
}

int main() {
  signed int x;
  volatile unsigned char result;

  klee_make_symbolic(&x, sizeof x, "x");

  result = convert_signed_int_to_unsigned_char(x);
  return 0;
}
