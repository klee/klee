// REQUIRES: z3
// RUN: %clang %s -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --search=bfs --solver-backend=z3-tree --max-solvers-approx-tree-inc=4 --debug-crosscheck-core-solver=z3 --debug-z3-validate-models --debug-assignment-validating-solver --use-cex-cache=false %t1.bc 2>&1 | FileCheck %s
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --search=dfs --solver-backend=z3-tree --max-solvers-approx-tree-inc=64 --debug-crosscheck-core-solver=z3 --debug-z3-validate-models --debug-assignment-validating-solver --use-cex-cache=false %t1.bc 2>&1 | FileCheck %s

#include "ExerciseSolver.c.inc"

// CHECK: KLEE: done: completed paths = 10
// CHECK: KLEE: done: partially completed paths = 4
