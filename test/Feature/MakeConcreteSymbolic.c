// RUN: %llvmgcc %s -emit-llvm -g -c -o %t1.bc
// RUN: %klee --exit-on-error %t1.bc
// RUN: grep "done: total queries = 0" klee-last/info
// RUN: %klee --make-concrete-symbolic=1 --exit-on-error %t1.bc
// RUN: grep "done: total queries = 2" klee-last/info


#include <assert.h>

#define N 2
int main() {
  int i;
  char a;
  
  a = 10;
  assert(a == 10);

  return 0;
}
