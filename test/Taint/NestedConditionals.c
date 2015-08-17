// RUN: %llvmgcc %s -emit-llvm -g -O0 -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee -taint=direct --output-dir=%t.klee-out -exit-on-error %t.bc


/* taint propagation by indirect flow in nested conditionals
 */
#include<klee/klee.h>
int 
main (int argc, char *argv[])
{
  int result;
  int param=0;
  int inner_param=param+1;

  klee_set_taint(1, &param, sizeof(param));

  if (param==1) 
    {

      if (inner_param==2)
	result=1;
      else
	result=2;

      result = result*2;
    }
  else
    result = 0;

  result = result / 2;

  klee_assert(klee_get_taint(&result,sizeof(result))==0);

  return result;

}

