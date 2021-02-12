// RUN: %clang %s -emit-llvm -O0 -g -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --libc=klee --output-dir=%t.klee-out --exit-on-error %t1.bc > %t-output.txt 2>&1
// RUN: FileCheck -input-file=%t-output.txt %s
#include "klee/klee.h"
#include <stdio.h>

int main() {
  float x, y, z;
  klee_make_symbolic(&x, sizeof(float), "x");
  y = 1.0e12f;
  z = x * y;
  if (z == y) {
    printf("z == y\n");
  } else if (z < y) {
    printf("z < y\n");
  } else if (z > y) {
    printf("z > y\n");
  } else {
    // FIXME: Check that this actually is NaN
    printf("z is NaN/Inf+/Inf-\n");
  }
  return 0;
}
// CHECK-NOT: silently concretizing (reason: floating point)
// CHECK: KLEE: done: completed paths = 4
