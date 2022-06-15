// RUN: %clang %s -emit-llvm %O0opt -c -g -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --external-calls=all --exit-on-error --output-dir=%t.klee-out %t.bc > %t.output.log 2>&1

#include <assert.h>
#include <stdlib.h>

int main() {

  int x;
  klee_make_symbolic(&x, sizeof(x), "x");

  if (x >= 0) {
    int y = abs(x);
    assert(x == y);
  }

  return 0;
}
