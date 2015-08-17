// RUN: %llvmgcc %s -emit-llvm -g -O0 -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee -taint=direct --output-dir=%t.klee-out -exit-on-error %t.bc


/* Basic tainting an int*/
#include<klee/klee.h>
int 
main (int argc, char *argv[])
{
  int i;
  int value = 0x41414141;
  klee_set_taint(1, &value, sizeof(value));
  for(i=0; i < sizeof value; i++){
      klee_assert(klee_get_taint((char*)&value + i, 1) == 1);
  }
  klee_assert(klee_get_taint(&value, sizeof value) == 1);
}
