// RUN: %llvmgcc %s -fsanitize=signed-integer-overflow -emit-llvm -g -O0 -c -o %t.bc
// RUN: %klee %t.bc 2> %t.log
// RUN: grep -c "overflow on unsigned subtraction" %t.log
// RUN: grep -c "ubsan_ssub_overflow.c:16: overflow" %t.log

#include "klee/klee.h"

int main()
{
  signed int x;
  signed int y;
  volatile signed int result;

  klee_make_symbolic(&x, sizeof(x), "signed sub 1");
  klee_make_symbolic(&y, sizeof(y), "signed sub 2");
  result = x - y;

  return 0;
}
