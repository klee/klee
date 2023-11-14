// REQUIRES: z3
// REQUIRES: bitwuzla
// RUN: %clang %s -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --solver-backend=z3 --use-forked-solver=false --debug-crosscheck-core-solver=bitwuzla --use-guided-search=none %t1.bc 2>&1 | FileCheck %s

#include "ExerciseSolver.c.inc"

// CHECK: KLEE: done: completed paths = 18
// CHECK: KLEE: done: partially completed paths = 0
