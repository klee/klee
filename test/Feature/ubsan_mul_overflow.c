// RUN: %llvmgcc %s -fsanitize=unsigned-integer-overflow -emit-llvm -g -O0 -c -o %t.bc
// RUN: %klee %t.bc 2> %t.log
// RUN: grep -c "overflow on unsigned multiplication" %t.log
// RUN: grep -c "ubsan_mul_overflow.c:18: overflow" %t.log
// RUN: grep -c "ubsan_mul_overflow.c:19: overflow" %t.log

#include "klee/klee.h"

int main()
{
  unsigned int x;
  unsigned int y;
  uint32_t x2=3147483648, y2=3727483648;
  volatile unsigned int result;

  klee_make_symbolic(&x, sizeof(x), "unsigned add 1");
  klee_make_symbolic(&y, sizeof(y), "unsigned add 2");
  result = x * y;
  result = x2 * y2;

  return 0;
}
