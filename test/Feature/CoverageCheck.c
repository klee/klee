// RUN: %clang %s -emit-llvm %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --use-cov-check=instruction-based --output-dir=%t.klee-out %t.bc > %t.log
#include "klee/klee.h"

#define a (2)
int main() {
  int i, n = 0, sn = 0;
  klee_make_symbolic(&n, sizeof(n), "n");
  klee_assume(n > 0);
  for (i = 1; i <= n; i++) {
    sn = sn + a;
  }
  assert(sn >= 0);
}
