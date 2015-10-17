// RUN: %llvmgcc %s -emit-llvm -g -std=c99 -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --svcomp-runtime %t1.bc 2>&1 | FileCheck %s
// RUN: test -f %t.klee-out/test000001.svcomp.err

#include "klee/klee_svcomp.h"

int main() {
  // FIXME: Use FileCheck's relative line feature
  // CHECK: verifier_error.c:11: svcomp error
  __VERIFIER_error();
  return 0;
}
