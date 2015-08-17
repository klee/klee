// RUN: %llvmgcc %s -emit-llvm -g -O0 -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee -taint=direct --output-dir=%t.klee-out -exit-on-error %t.bc



#include<klee/klee.h>
int 
main (int argc, char *argv[])
{
  int a = 1;
  int c = 0;
  klee_set_taint (1, &a, sizeof (a));
  switch(a)
    {
    case 0: 
       c = 0;
       break;
    case 1: 
       c = 1;
       break;
    default:
       c=1000;
    }

  klee_assert (klee_get_taint (&c, sizeof (c)) == 0 );
}

