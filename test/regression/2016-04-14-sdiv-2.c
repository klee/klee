// RUN: %clang %s -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out -exit-on-error -solver-optimize-divides=true %t.bc >%t1.log
// RUN: grep "m is 2" %t1.log
#include <assert.h>
#include <stdio.h>

#include "klee/klee.h"

int main(void)
{
  int n, m;
  klee_make_symbolic(&n, sizeof n, "n");
  klee_assume(n < -1);

  if (n/2 > 0)
    assert(0);

  klee_make_symbolic(&m, sizeof m, "m");
  klee_assume(m > 0);

  if (m/2 == 2)
    printf("m is 2\n");
  
  return 0;
} 
