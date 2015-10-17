// RUN: %llvmgcc %s -emit-llvm -g -std=c99 -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --svcomp-runtime %t.bc > %t.kleeoutput.log 2>&1
// RUN: not test -f %t.klee-out/test*.err

// Check that the atomic functions are callable.
#include "klee/klee_svcomp.h"
int main() {
  __VERIFIER_atomic_begin();
  __VERIFIER_atomic_end();
  return 0;
}
