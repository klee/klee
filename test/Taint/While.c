// RUN: %llvmgcc %s -emit-llvm -g -O0 -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee -taint=direct --output-dir=%t.klee-out -exit-on-error %t.bc


/* taint propagation by indirect flow in the conditional (in the while)
 */ 

#include<klee/klee.h>
int 
main (int argc, char *argv[])
{
  int result=1;
  int param=0;
  
  klee_set_taint(1,&param,sizeof(param));
  while (param<=10) {
    param++;
    result = result*2;
  }
  result = result / 2;

  klee_assert(klee_get_taint(&result,sizeof(result))==0);

  return result;
}
