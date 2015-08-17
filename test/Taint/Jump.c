// RUN: %llvmgcc %s -emit-llvm -g -O0 -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee -taint=direct --output-dir=%t.klee-out -exit-on-error %t.bc


/* Tests taint propagation in parameter passing to function called by pointer */
#include<klee/klee.h>
void jmpfun(int *result, int param){
  *result=param;
}

int 
main (int argc, char *argv[])
{
  void (*ptrFun)();
  int result;
  int param;
  klee_set_taint(1, &param, sizeof(param));

  ptrFun = &jmpfun;
  ptrFun(&result, param);

  klee_assert(klee_get_taint(&result,sizeof(result))==1);
}
