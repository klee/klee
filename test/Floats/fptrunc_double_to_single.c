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
  // TODO: Set the rounding mode, assume RNE for now
  klee_make_symbolic(&y, sizeof(double), "y");
  klee_assume(y > 0.0); // This also implies y isn't a NaN
  x = (float)y;
  if (x > 0.0f) {
    printf("no truncation around 0.0\n");
  } else {
    // TODO: If we use RU instead of RNE I don't think this branch is feasible
    assert(x == 0.0f);
    printf("truncation around 0.0\n");
  }
  return 0;
}
// CHECK-NOT: silently concretizing (reason: floating point)
// CHECK: KLEE: done: completed paths = 2
