// RUN: %clang %s -fsanitize=signed-integer-overflow -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --emit-all-errors --ubsan-runtime %t.bc 2>&1 | FileCheck %s

#include "klee/klee.h"

int main() {
  signed int x;
  signed int y;
  volatile signed int result;

  klee_make_symbolic(&x, sizeof(x), "x");
  klee_make_symbolic(&y, sizeof(y), "y");

  // CHECK: runtime/Sanitizer/ubsan/ubsan_handlers.cpp:35: signed-integer-overflow
  result = x + y;

  // CHECK: runtime/Sanitizer/ubsan/ubsan_handlers.cpp:35: signed-integer-overflow
  result = x - y;

  // CHECK: runtime/Sanitizer/ubsan/ubsan_handlers.cpp:35: signed-integer-overflow
  result = x * y;

  // CHECK: runtime/Sanitizer/ubsan/ubsan_handlers.cpp:35: signed-integer-overflow
  result = -x;

  return 0;
}
