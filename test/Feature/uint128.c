// RUN: %clang %s -emit-llvm %O0opt -c -g -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out %t1.bc 2>&1 | FileCheck %s
// CHECK-NOT: failed external call
#include "klee-test-comp.c"
extern unsigned __int128 __VERIFIER_nondet_uint128();

int main() {
  __int128 x = __VERIFIER_nondet_uint128();
  return x > 0;
}
