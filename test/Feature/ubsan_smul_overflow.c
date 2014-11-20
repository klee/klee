// RUN: %llvmgcc %s -fsanitize=signed-integer-overflow -emit-llvm -g -O0 -c -o %t.bc
// RUN: %klee %t.bc 2> %t.log
// RUN: grep -c "overflow on unsigned multiplication" %t.log
// RUN: grep -c "ubsan_smul_overflow.c:16: overflow" %t.log

#include "klee/klee.h"

int main()
{
  int x;
  int y;
  volatile int result;

  klee_make_symbolic(&x, sizeof(x), "signed mul 1");
  klee_make_symbolic(&y, sizeof(y), "signed mul 2");
  result = x * y;

  return 0;
}
