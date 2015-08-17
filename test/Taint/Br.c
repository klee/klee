// RUN: %llvmgcc %s -emit-llvm -g -O0 -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee -taint=direct --output-dir=%t.klee-out -exit-on-error %t.bc



#include<klee/klee.h>
int 
main (int argc, char *argv[])
{
  int a = 1;
  int b = 2;
  int c = 3;
  klee_set_taint (1, &a, sizeof (a));
  klee_set_taint (2, &b, sizeof (b));
  klee_set_taint (4, &c, sizeof (c));
  if (a==1)
    a=b;
  else
    a=c;

  klee_assert (klee_get_taint (&a, sizeof (a)) == 2);
  klee_assert (klee_get_taint (&b, sizeof (b)) == 2);
  klee_assert (klee_get_taint (&c, sizeof (c)) == 4);
}

