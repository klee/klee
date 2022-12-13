// RUN: %clang %s -emit-llvm -O0 -g -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --libc=klee --fp-runtime --output-dir=%t.klee-out --exit-on-error %t1.bc > %t-output.txt 2>&1
// RUN: FileCheck -input-file=%t-output.txt %s
#include "klee/klee.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

int main() {
  double x;
  klee_make_symbolic(&x, sizeof(double), "x");
  if (isnan(x)) {
    assert(x != x);
    assert(klee_is_nan_double(x));
  } else {
    assert(x == x);
    assert(!klee_is_nan_double(x));
  }
}
// CHECK-NOT: silently concretizing (reason: floating point)
// CHECK: KLEE: done: completed paths = 2
