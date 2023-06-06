// REQUIRES: z3
// RUN: %clang %s -emit-llvm %O0opt -c -g -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --solver-backend=z3 --out-of-mem-allocs=true --skip-not-symbolic-objects --use-merged-pointer-dereference=true %t.bc 2>&1 | FileCheck %s

#include "klee/klee.h"

int main() {
  int x = klee_int("x");
  int f[x];
  // CHECK-DAG: SymbolicArrayOnStackWithSkipSymbolics.c:[[@LINE+2]]: memory error: out of bound pointer
  // CHECK-DAG: SymbolicArrayOnStackWithSkipSymbolics.c:[[@LINE+1]]: memory error: null pointer exception
  f[0] = 10;
}

// CHECK: KLEE: done: completed paths = 1
// CHECK: KLEE: done: partially completed paths = 2
// CHECK: KLEE: done: generated tests = 3
