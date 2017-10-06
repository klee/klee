// RUN: %llvmgcc %s -fsanitize=signed-integer-overflow -emit-llvm -g -O0 -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out %t.bc 2>&1 | FileCheck %s

#include "klee/klee.h"

int main()
{
  signed int x;
  signed int y;
  volatile signed int result;

  klee_make_symbolic(&x, sizeof(x), "x");
  klee_make_symbolic(&y, sizeof(y), "y");

  // CHECK: ubsan_signed_overflow.c:17: overflow on addition
  result = x + y;

  // CHECK: ubsan_signed_overflow.c:20: overflow on subtraction
  result = x - y;

  // CHECK: ubsan_signed_overflow.c:23: overflow on multiplication
  result = x * y;

  return 0;
}
