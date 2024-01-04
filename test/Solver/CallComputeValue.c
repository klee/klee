// REQUIRES: z3
// RUN: %clang %s -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --search=bfs --solver-backend=z3-tree --max-solvers-approx-tree-inc=4 --debug-crosscheck-core-solver=z3 --debug-z3-validate-models --debug-assignment-validating-solver --use-cex-cache=false --use-lazy-initialization=only %t1.bc 2>&1 | FileCheck %s

#include "klee/klee.h"

int main() {
  int y;
  klee_make_symbolic(&y, sizeof(y), "y");
  int *x;
  klee_make_symbolic(&x, sizeof(x), "x");
  if (!x)
    return 0;
  if (*x == y)
    return 1;
  return 2;
}

// CHECK: KLEE: done: completed paths = {{3|5}}
// CHECK: KLEE: done: partially completed paths = 0
