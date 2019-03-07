// RUN: %clang %s -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out %t1.bc

#include <assert.h>

int main() {
  int d;
  
  klee_make_symbolic(&d, sizeof(d), "d");

  int l = d - 1;
  unsigned long long m = ((unsigned long long) l << 32) / d;
  if (d==2) {
    assert(m == 2147483648u);
  }

  klee_silent_exit(0);

  return 0;
}
