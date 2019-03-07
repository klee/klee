// RUN: %clang %s -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --exit-on-error %t1.bc

#include <assert.h>

int main() {
  double a1 = -1.1;
  double a2 = 1.2;
  
  int b1 = (int) a1;
  assert(b1 == -1);
  
  int b2 = (int) a2;
  assert(b2 == 1);

  a1 = (double) b1;
  assert(a1 == -1);

  a2 = (double) b2;
  assert(a2 == 1);

  return 0;
}
