// RUN: %llvmgcc %s -emit-llvm -g -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --stop-after-n-instructions=500 --search=bfs %t1.bc 2>%t2.log
// RUN: FileCheck -input-file=%t2.log %s
#include "klee/klee.h"
unsigned int nd() {
  unsigned int r;
  klee_make_symbolic(&r, sizeof(r), "r");
  return r;
}

int main() {
  unsigned int x = 1;
  unsigned int y = nd();
  klee_assume(y > 0);
  while (x < y) {
    if (x < y / x) {
      x *= x;
    } else {
      x++;
    }
  }
  // CHECK: ASSERTION FAIL
  klee_assert(x != y);
  return x;
}
