// RUN: %llvmgcc %s -fsanitize=signed-integer-overflow -emit-llvm -g -O0 -c -o %t.bc
// RUN: %klee %t.bc 2> %t.log
// RUN: grep -c "overflow on unsigned addition" %t.log
// RUN: grep -c "ubsan_sadd_overflow.c:16: overflow" %t.log

#include "klee/klee.h"

int main()
{
  signed int x;
  signed int y;
  volatile signed int result;

  klee_make_symbolic(&x, sizeof(x), "signed add 1");
  klee_make_symbolic(&y, sizeof(y), "signed add 2");
  result = x + y;

  return 0;
}
