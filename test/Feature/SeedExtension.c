/* This test checks the case where the seed needs to be patched on re-run */

// RUN: %clang -emit-llvm -c %O0opt -g %s -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --entry-point=TestGen %t.bc
// RUN: test -f %t.klee-out/test000001.ktest
// RUN: not test -f %t.klee-out/test000002.ktest

// RUN: rm -rf %t.klee-out-2
// RUN: %klee --exit-on-error --output-dir=%t.klee-out-2 --seed-file %t.klee-out/test000001.ktest --allow-seed-extension %t.bc 2>&1 | FileCheck %s

#include "klee/klee.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

void TestGen() {
  int x;
  klee_make_symbolic(&x, sizeof(x), "x");
  klee_assume(x == 12345678);
}

int main() {
  int i;
  klee_make_symbolic(&i, sizeof(i), "i");
  double d = i;
  // CHECK: concretizing (reason: floating point)
  assert((unsigned)d == 12345678);

  int j;
  klee_make_symbolic(&j, sizeof(j), "j");
  if (j)
    printf("Yes\n");
  // CHECK-DAG: Yes
  else
    printf("No\n");
  // CHECK-DAG: No
}
