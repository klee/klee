// RUN: %clang %s -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --solver-backend=dummy %t1.bc

#include "ExerciseSolver.c.inc"

// CHECK: KLEE: done: completed paths = 0
// CHECK: KLEE: done: partially completed paths = 1
