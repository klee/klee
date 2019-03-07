// RUN: %clang %s -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out %t1.bc

#include <assert.h>

int main(void) {
  char c[2];

  klee_make_symbolic(&c, sizeof(c), "c");

  if (c[0] > 10) {
    int x;

    c[1] = 1; // copy object state

    assert(c[0] > 10);
  }

  return 0;
}
