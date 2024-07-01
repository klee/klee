// RUN: %clang %s -emit-llvm %O0opt -c -g -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out %t.bc 2>&1 | FileCheck %s

#include "klee/klee.h"

int main() {
  int n, m;
  klee_make_symbolic(&n, sizeof(n), "n");
  klee_make_symbolic(&m, sizeof(m), "m");

  int z = (n ? 1 : 0) / (m ? 1 : 0);
  (void)z;
}
// CHECK: KLEE: done
