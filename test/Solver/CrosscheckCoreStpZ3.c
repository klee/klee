// REQUIRES: stp
// REQUIRES: z3
// RUN: %clang %s -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --solver-backend=stp --use-forked-solver=false --debug-crosscheck-core-solver=z3 %t1.bc

#include "ExerciseSolver.c.inc"

// CHECK: KLEE: done: completed paths = 15
// CHECK: KLEE: done: partially completed paths = 0
