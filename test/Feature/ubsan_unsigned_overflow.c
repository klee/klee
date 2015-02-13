// RUN: %llvmgcc %s -fsanitize=unsigned-integer-overflow -emit-llvm -g -O0 -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out %t.bc 2>&1 | FileCheck %s

// llvm-gcc 2.9 does not support -fsanitize=unsigned-integer-overflow
// REQUIRES: not-llvm-2.9

#include "klee/klee.h"

int main()
{
  unsigned int x;
  unsigned int y;
  volatile unsigned int result;

  klee_make_symbolic(&x, sizeof(x), "x");
  klee_make_symbolic(&y, sizeof(y), "y");

  // CHECK: ubsan_unsigned_overflow.c:20: overflow on unsigned addition
  result = x + y;

  // CHECK: ubsan_unsigned_overflow.c:23: overflow on unsigned subtraction
  result = x - y;

  // CHECK: ubsan_unsigned_overflow.c:26: overflow on unsigned multiplication
  result = x * y;

  return 0;
}
