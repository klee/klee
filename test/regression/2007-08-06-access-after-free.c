// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t1.bc
// RUN: %klee %t1.bc

#include <assert.h>

int main() {
  int a;
  unsigned char *p = malloc(4);

  klee_make_symbolic(&a, sizeof a);
  klee_make_symbolic(p, sizeof p);

  p[0] |= 16;

  if (a) {
    free(p);

    // this should give an error instead of
    // pulling the state from the parent, where
    // it is not free
    assert(p[0] > 10);
   
    return 0;
  }
  
  assert(p[0] > 10);

  return 0;
}
