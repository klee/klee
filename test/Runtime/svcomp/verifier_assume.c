// RUN: %llvmgcc %s -emit-llvm -std=c99 -g -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --svcomp-runtime %t1.bc
// RUN: not test -f %t.klee-out/test*.err
// RUN: test -f %t.klee-out/test000001.ktest
// RUN: test -f %t.klee-out/test000002.ktest
// RUN: not test -f %t.klee-out/test000003.ktest
#include "klee/klee.h"
#include "klee/klee_svcomp.h"
#include "stdio.h"

int main() {
  // FIXME: Use FileCheck's relative line feature
  // CHECK: verifier_assert.c:12: svcomp assert failed
  int x;
  klee_make_symbolic(&x, sizeof(int), "x");
  __VERIFIER_assume( x > 10 );
  if (x == 10) {
    // should be unreachable
    printf("Is 10\n");
    __VERIFIER_error();
  } else if (x == 11) {
    printf("Is 11\n");
  } else {
    printf("Is something else\n");
  }
  return 0;
}
