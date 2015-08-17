// RUN: %llvmgcc %s -emit-llvm -g -O0 -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee -taint=direct --output-dir=%t.klee-out -exit-on-error %t.bc


/*
  This shall propagate the taints from both the value and from
  the pointer used to set the result 
*/
#include<klee/klee.h>
int 
main (int argc, char *argv[])
{
  int value = 0x12345678;
  int *value_p = &value;
  int result;

  klee_set_taint(0,&result,sizeof(result));
  klee_set_taint(1,&value,sizeof(value));
  klee_set_taint(2,&value_p,sizeof(value_p));

  result = *value_p;

  //printf ("Result taint: 0x%08x\n", klee_get_taint(&result,sizeof(result)));
  klee_assert(klee_get_taint(&result,sizeof(result))==3);

  return result;
}


