/* This test checks the case where the seed needs to be patched on re-run */

// RUN: %clang -emit-llvm -c %O0opt -g %s -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --entry-point=TestGen %t.bc
// RUN: test -f %t.klee-out/test000001.ktest
// RUN: not test -f %t.klee-out/test000002.ktest

// RUN: rm -rf %t.klee-out-2
// RUN: %klee --exit-on-error --output-dir=%t.klee-out-2 --seed-file %t.klee-out/test000001.ktest --allow-seed-extension %t.bc 2>&1 | FileCheck %s
// RUN: %klee-stats --print-columns 'SolverQueries' --table-format=csv %t.klee-out-2 | FileCheck --check-prefix=CHECK-STATS %s

#include "klee/klee.h"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

void TestGen() {
  uint16_t x;
  klee_make_symbolic(&x, sizeof(x), "x");
  klee_assume(x == 1234);
}

int main() {
  uint32_t i;
  klee_make_symbolic(&i, sizeof(i), "i");

  if (i < 5000) {
    double d = i;
    // CHECK: concretizing (reason: floating point)
    assert((unsigned) d < 5001);
  }

  // CHECK-STATS: 3
  // These is similar to SeedConcretizeFP.c (1 query) plus the extra queries due to an incomplete seed
}
