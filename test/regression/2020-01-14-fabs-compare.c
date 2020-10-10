// RUN: %clang %s -emit-llvm %O0opt -g -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --exit-on-error --output-dir=%t.klee-out %t.bc

#include <math.h>
#include <assert.h>

int main(void) {
  long double a = 0.25;
  long double b = -a;

  long double b_abs = fabsl(b);

  assert(a == b_abs);
  return 0;
}
