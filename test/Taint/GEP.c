// RUN: %llvmgcc %s -emit-llvm -g -O0 -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee -taint=direct --output-dir=%t.klee-out -exit-on-error %t.bc


#include<klee/klee.h>
void test_gep()
{
  char a[256];
  char i=2;
  klee_set_taint (1, &i, sizeof (i));
  int r = *(a+i);
  klee_assert (klee_get_taint (&r, sizeof(r)) == 1 );
}


void test_idx()
{
  char a[256];
  int i=2;
  klee_set_taint (1, &i, sizeof (i));
  a[i]=1;
  klee_assert (klee_get_taint (&a[i], sizeof (a[i])) == 1 );
}

int 
main (int argc, char *argv[])
{
test_gep();
test_idx();
}

