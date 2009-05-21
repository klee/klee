// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t1.bc
// RUN: %klee %t1.bc

#include <assert.h>

int main() {
  int d;
  
  klee_make_symbolic( &d, sizeof(d) );

  int l = d - 1;
  unsigned long long m = ((unsigned long long) l << 32) / d;
  if (d==2) {
    assert(m == 2147483648u);
  }

  klee_silent_exit(0);

  return 0;
}
