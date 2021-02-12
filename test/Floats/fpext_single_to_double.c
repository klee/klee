// RUN: %clang %s -emit-llvm -O0 -g -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --libc=klee --output-dir=%t.klee-out --exit-on-error %t1.bc > %t-output.txt 2>&1
// RUN: FileCheck -input-file=%t-output.txt %s
#include "klee/klee.h"
#include <assert.h>
#include <stdio.h>

int main() {
  float x;
  double y;
  klee_make_symbolic(&x, sizeof(float), "x");
  klee_assume(x > 0.0f); // This also implies x isn't a NaN
  y = (double) x;
  assert(y > 0.0);
  return 0;
}
// CHECK-NOT: silently concretizing (reason: floating point)
// CHECK: KLEE: done: completed paths = 1
