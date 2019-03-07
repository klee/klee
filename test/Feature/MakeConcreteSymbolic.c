// RUN: %clang %s -emit-llvm -g -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --optimize=false --output-dir=%t.klee-out --exit-on-error %t1.bc
// RUN: grep "done: total queries = 0" %t.klee-out/info

// RUN: rm -rf %t.klee-out
// RUN: %klee --optimize=false --output-dir=%t.klee-out --make-concrete-symbolic=1 --exit-on-error %t1.bc
// RUN: grep "done: total queries = 2" %t.klee-out/info


#include <assert.h>

#define N 2
int main() {
  int i;
  char a;
  
  a = 10;
  assert(a == 10);

  return 0;
}
