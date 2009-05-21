// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t1.bc
// RUN: %klee --exit-on-error --optimize --libc=uclibc %t1.bc

#include <string.h>
#include <assert.h>

int main() {
  int a;

  klee_make_symbolic(&a, sizeof a);

  memset(&a, 0, sizeof a);

  if (a) {
    assert(0);
  }
  
  return 0;
}
