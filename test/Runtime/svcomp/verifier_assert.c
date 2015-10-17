// RUN: %llvmgcc %s -emit-llvm -std=c99 -g -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --svcomp-runtime %t1.bc 2>&1 | FileCheck %s
// RUN: test -f %t.klee-out/test000001.svcomp.assertfail
// RUN: test -f %t.klee-out/test000001.ktest
// RUN: test -f %t.klee-out/test000002.ktest
#include "klee/klee.h"
#include "klee/klee_svcomp.h"

int main() {
  // FIXME: Use FileCheck's relative line feature
  // CHECK: verifier_assert.c:15: svcomp assert failed
  int truth;
  klee_make_symbolic(&truth, sizeof(int), "truth");
  __VERIFIER_assert(truth);
  return 0;
}
