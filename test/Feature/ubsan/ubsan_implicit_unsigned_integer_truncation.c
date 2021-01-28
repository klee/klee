// REQUIRES: geq-llvm-8.0

// RUN: %clang %s -fsanitize=implicit-unsigned-integer-truncation -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --emit-all-errors --ubsan-runtime %t.bc 2>&1 | FileCheck %s

#include "klee/klee.h"

unsigned char convert_unsigned_int_to_unsigned_char(unsigned int x) {
  // CHECK: runtime/Sanitizer/ubsan/ubsan_handlers.cpp:35: implicit-unsigned-integer-truncation
  return x;
}

int main() {
  unsigned int x;
  volatile unsigned char result;

  klee_make_symbolic(&x, sizeof(x), "x");

  result = convert_unsigned_int_to_unsigned_char(x);
  return 0;
}