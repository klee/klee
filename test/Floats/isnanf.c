// RUN: %clang %s -emit-llvm -O0 -g -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --libc=klee --fp-runtime --output-dir=%t.klee-out --exit-on-error %t1.bc > %t-output.txt 2>&1
// RUN: FileCheck -input-file=%t-output.txt %s
#include "klee/klee.h"
#include <stdio.h>
#include <math.h>
#include <assert.h>

int main() {
  float x;
  klee_make_symbolic(&x, sizeof(float), "x");
  if (isnan(x)) {
    assert(x != x);
    assert(klee_is_nan_float(x));
  }
  else {
    assert(x == x);
    assert(!klee_is_nan_float(x));
  }
}
// CHECK-NOT: silently concretizing (reason: floating point)
// CHECK: KLEE: done: completed paths = 2
