// RUN: %clang -emit-llvm -c -g %s -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --entry-point=TestGen %t.bc
// RUN: test -f %t.klee-out/test000001.ktest
// RUN: not test -f %t.klee-out/test000002.ktest

// RUN: rm -rf %t.klee-out-2
// RUN: %klee --external-calls=all --exit-on-error --output-dir=%t.klee-out-2 --seed-file %t.klee-out/test000001.ktest %t.bc
// RUN: %klee-stats --print-columns 'SolverQueries' --table-format=csv %t.klee-out-2 | FileCheck --check-prefix=CHECK-STATS %s

#include "klee/klee.h"

#include <assert.h>
#include <stdlib.h>

void TestGen() {
  int x;
  klee_make_symbolic(&x, sizeof(x), "x");
  klee_assume(x == 12345678);
}

int main() {
  int x;
  klee_make_symbolic(&x, sizeof(x), "x");
  assert(abs(x) == 12345678);

  // CHECK-STATS: 1
}
