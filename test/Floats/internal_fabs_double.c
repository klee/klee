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
#include <stdint.h>

int main() {
  // -ve
  double x = -100.0;
  double result = klee_abs_double(x);
  assert(result == 100.0);

  // +ve
  x = 100.0;
  assert(!signbit(x));
  result = klee_abs_double(x);
  assert(result == 100.0);

  // -ve zero
  x = -0.0;
  assert(signbit(x));
  result = klee_abs_double(x);
  assert(result == 0.0);
  assert(signbit(x));

  // +ve zero
  x = 0.0;
  assert(!signbit(x));
  result = klee_abs_double(x);
  assert(result == 0.0);
  assert(!signbit(x));

  // +NaN
  uint64_t posNanBits = 0x7ff8000000000000;
  double posNan = 0.0f;
  assert(sizeof(posNanBits) == sizeof(posNan));
  memcpy(&posNan, &posNanBits, sizeof(double));
  assert(isnan(posNan));
  assert(!signbit(posNan));
  result = klee_abs_double(posNan);
  assert(isnan(result));
  assert(!signbit(result)); // is +ve NaN.

  // -NaN
  uint64_t negNanBits = 0xfff8000000000000;
  double negNan = 0.0f;
  assert(sizeof(negNanBits) == sizeof(negNan));
  memcpy(&negNan, &negNanBits, sizeof(double));
  assert(isnan(negNan));
  assert(signbit(negNan));
  result = klee_abs_double(negNan);
  assert(isnan(result));
  assert(!signbit(result)); // is +ve NaN.

  // +Infinity
  x = INFINITY;
  assert(!signbit(x));
  result = klee_abs_double(x);
  assert(isinf(x) == 1);

  // -Infinity
  x = - INFINITY;
  assert(signbit(x));
  result = klee_abs_double(x);
  assert(isinf(x) == -1);

  return 0;
}
// CHECK: KLEE: done: completed paths = 1
