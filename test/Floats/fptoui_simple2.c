// RUN: %clang %s -emit-llvm -O0 -g -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --libc=klee --output-dir=%t.klee-out --exit-on-error %t1.bc > %t-output.txt 2>&1
// RUN: FileCheck -input-file=%t-output.txt %s
#include "klee/klee.h"
#include <assert.h>
#include <stdio.h>

int main() {
  float x;
  unsigned int y;
  klee_make_symbolic(&x, sizeof(float), "x");
  klee_assume(x > 0.95); // This also implies x isn't a NaN
  klee_assume(x < 128.0f);
  // The C11 spec (6.3.1.4) says this conversion should
  // truncate the fractional part of ``x`` (i.e.
  // this is like rounding toward 0).
  y = (unsigned int) x;
  if (y == 0) {
    printf("y is zero\n");
  } else {
    printf("y non zero\n");
  }
  assert(y < 128);
  return 0;
}
// CHECK-NOT: silently concretizing (reason: floating point)
// CHECK: KLEE: done: completed paths = 2
