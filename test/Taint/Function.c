// RUN: %llvmgcc %s -emit-llvm -g -O0 -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee -taint=direct --output-dir=%t.klee-out -exit-on-error %t.bc

#include<klee/klee.h>

int inner_fun(int parama, int paramb){
    return parama+paramb;
}

int fun(int parama, int paramb){
  int result;
  result = inner_fun(parama, paramb);
  return result;
}

/* Tests taint is mantained under internal function calls
 */
int 
main (int argc, char *argv[])
{
  int result;
  int parama=100;
  int paramb=200;

  klee_set_taint(1, &parama, sizeof(parama));
  klee_set_taint(2, &paramb, sizeof(paramb));

  result = fun(parama, paramb);

  result = result / 2;

  klee_assert(klee_get_taint(&result, sizeof(result))==3);

  return result;
}


