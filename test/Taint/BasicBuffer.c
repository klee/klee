// RUN: %llvmgcc %s -emit-llvm -g -O0 -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee -taint=direct --output-dir=%t.klee-out -exit-on-error %t.bc


/* Basic buffer tainting */
#include<klee/klee.h>
int 
main (int argc, char *argv[])
{
  int i;
  char value[8] = {0,1,2,3,4,5,6,7};
  for(i=0; i < sizeof value; i++){
      klee_set_taint(1<<i, &value[i], 1);
  }
  for(i=0; i < sizeof value; i++){
      klee_assert(klee_get_taint(&value[i], 1) == 1<<i );
  }
  klee_assert(klee_get_taint(&value, sizeof value) == 0xff);
}
