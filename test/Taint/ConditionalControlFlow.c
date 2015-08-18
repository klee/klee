// RUN: %llvmgcc %s -emit-llvm -g -O0 -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee -taint=controlflow --output-dir=%t.klee-out -exit-on-error %t.bc


/* taint propagation by indirect flow in the conditional
 */ 
#include<klee/klee.h>
int 
main (int argc, char *argv[])
{

  int result;
  int param=1;
  klee_set_taint(1,&param,sizeof(param));
  if (param == 1)
    result = 1;
  else
    result = 2;
  
  klee_assert(klee_get_taint(&result,sizeof(result))==1);
  return result;
}
