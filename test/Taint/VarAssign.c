// RUN: %llvmgcc %s -emit-llvm -g -O0 -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee -taint=direct --output-dir=%t.klee-out -exit-on-error %t.bc

/* assigning to a variable should overwrite its taintset*/
#include<klee/klee.h>
int 
main (int argc, char *argv[])
{
  int valuea = 0x41414141;
  int valueb = 0x42424242;
  klee_set_taint(1, &valuea, sizeof(valuea));
  klee_set_taint(2, &valueb, sizeof(valueb));
  valuea = valueb;

  klee_assert (klee_get_taint(&valuea, sizeof(valuea)) == 2 );
}
