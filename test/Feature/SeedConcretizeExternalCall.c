// The test checks that when an external call is made with a symbolic
// argument, the concretized value is chosen to be the seed value

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
  int a, b;
  klee_make_symbolic(&a, sizeof(a), "a");
  klee_make_symbolic(&b, sizeof(b), "b");
  klee_assume(a == 12345678);
  klee_assume(b == 123456);
}

int main() {
  int a, b;
  klee_make_symbolic(&a, sizeof(a), "a");
  klee_make_symbolic(&b, sizeof(b), "b");
  div_t r = div(a, b);
  assert(r.quot == 100);
  assert(r.rem == 78);

  // CHECK-STATS: 2
}
