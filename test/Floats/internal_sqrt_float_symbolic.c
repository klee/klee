// RUN: %clang %s -emit-llvm -O0 -g -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --libc=klee --fp-runtime --output-dir=%t.klee-out --exit-on-error %t1.bc > %t-output.txt 2>&1
// RUN: FileCheck -input-file=%t-output.txt %s
// REQUIRES: fp-runtime
#include "klee/klee.h"
#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdio.h>

int main() {
  float x = 0.0f;
  klee_make_symbolic(&x, sizeof(float), "x");
  float result = klee_sqrt_float(x);

  if (isnan(x)) {
    assert(isnan(result));
    return 0;
  }

  if (x < 0.0f) {
    assert(isnan(result));
    return 0;
  }

  if (isinf(x)) {
    assert(isinf(result) == 1);
    return 0;
  }

  if (x == 0.0f) {
    assert(result == 0.0f);
    // check sign bit sqrt(-0.0) == -0.0
    if (signbit(x)) {
      assert(signbit(result));
    }
    return 0;
  }

  assert(x > 0.0f);
  return 0;
}
// CHECK: KLEE: done: completed paths = 6
