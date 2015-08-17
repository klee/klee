// RUN: %llvmgcc %s -emit-llvm -g -O0 -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee -taint=direct --output-dir=%t.klee-out -exit-on-error %t.bc

#include<klee/klee.h>

/**
 * Constants should not contribute any taint
 */
int 
main (int argc, char *argv[])
{
  int a = 10;
  int b = 10;
  klee_set_taint (1, &a, sizeof (a));
  klee_set_taint (1, &b, sizeof (b));

  int c ;

  c = b+1;

  klee_assert(klee_get_taint (&c, sizeof (c))==1);
}
