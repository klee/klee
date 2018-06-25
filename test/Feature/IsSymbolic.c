// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee -exit-on-error --output-dir=%t.klee-out %t1.bc  &> %t1.log

#include <assert.h>
#include "klee/klee.h"

int main() {
  int x, y, z = 0;
  klee_make_symbolic(&x, sizeof x, "x");
  klee_make_symbolic(&y, sizeof y, "y");
  klee_assume(y >= 0 & y <= 4);

  int array[5] = {1,1,x,1,1};
  if (x == 1) {
    assert(klee_is_symbolic(y));
    assert(!klee_is_symbolic(array[y]));
  } else {
    assert(!klee_is_symbolic(z));
    assert(klee_is_symbolic(array[y]));
  }


  return 0;
}
