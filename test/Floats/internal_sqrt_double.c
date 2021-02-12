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
  double x = 100.0;
  double result = klee_sqrt_double(x);
  printf("sqrt(%f) = %f\n", x, result);
  assert(result == 10.0);

  float y = 100.0f;
  result = klee_sqrt_double(y);
  printf("sqrt(%f) = %f\n", y, result);
  assert(result == 10.0);

  printf("Test sqrt negative zero\n");
  x = -0.0;
  assert(signbit(x));
  result = klee_sqrt_double(x);
  printf("sqrt(%f) = %f\n", x, result);
  assert(result == -0.0);
  assert(signbit(result));

  printf("Test sqrt negative\n");
  x = -DBL_MIN;
  assert(signbit(x));
  assert(x < 0.0);
  result = klee_sqrt_double(x);
  printf("sqrt(%f) = %f\n", x, result);
  assert(isnan(result));

  return 0;
}
// CHECK: KLEE: done: completed paths = 1
