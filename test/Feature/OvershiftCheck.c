// RUN: %clang %s -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out -check-overshift %t.bc 2> %t.log
// RUN: FileCheck --input-file %t.log %s

/* This test checks that two consecutive potential overshifts
 * are reported as errors.
 */
#include "klee/klee.h"
int main() {
  unsigned int x = 15;
  unsigned int y;
  unsigned int z;
  volatile unsigned int result;

  /* Overshift if y>= sizeof(x) */
  klee_make_symbolic(&y, sizeof(y), "shift_amount1");
  // CHECK: OvershiftCheck.c:[[@LINE+1]]: overshift error
  result = x << y;

  /* Overshift is z>= sizeof(x) */
  klee_make_symbolic(&z, sizeof(z), "shift_amount2");
  // CHECK: OvershiftCheck.c:[[@LINE+1]]: overshift error
  result = x >> z;

  return 0;
}
