// Test case based on #1668, generates array names as part of the solver builder that collide.
// This depends on the order of expression evaluation.
//
// RUN: %clang %s -g -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out -silent-klee-assume --use-branch-cache=false --use-cex-cache=false --use-independent-solver=false %t1.bc 2>&1 | FileCheck %s
#include "klee/klee.h"
#include <assert.h>

int main() {
  int p1 = klee_int("val");
  int p2 = klee_int("val");
  int p3 = klee_int("val");
  int p4 = klee_int("val");
  int p5 = klee_int("val");
  int p6 = klee_int("val");
  int p7 = klee_int("val");
  int p8 = klee_int("val");
  int p9 = klee_int("val");
  int p10 = klee_int("val");
  int p11 = klee_int("val");
  int p12 = klee_int("val");
  int p13 = klee_int("val");
  int p14 = klee_int("val");
  int p15 = klee_int("val");
  int cond = klee_int("val");
  klee_assume(p12 > p14);
  klee_assume(p6 > p3);
  //    klee_assume(p2 > 0);
  klee_assume(p7 != 0);
  klee_assume(p11 < p14 & p15 < p13);
  klee_assume(cond > p5);
  klee_assume(0 > p4);
  // CHECK: [[@LINE+1]]: ASSERTION FAIL
  assert(p2 > p11);
}