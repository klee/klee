// RUN: %llvmgcc %s -emit-llvm -g -O0 -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee -taint=direct --output-dir=%t.klee-out -exit-on-error %t.bc

/* assigning a constant to a variable should erase its taintset*/
#include<klee/klee.h>
int 
main (int argc, char *argv[])
{
  int value = 0x12345678;
  klee_set_taint(1, &value, sizeof(value));
  value = 10;
  klee_assert(klee_get_taint(&value,sizeof(value)) == 0 );
}
