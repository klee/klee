// RUN: %clang %s -emit-llvm -O0 -g -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --libc=klee --fp-runtime --output-dir=%t.klee-out --exit-on-error %t1.bc > %t-output.txt 2>&1
// RUN: FileCheck -input-file=%t-output.txt %s
// REQUIRES: fp-runtime
#include "klee/klee.h"
#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>

int main() {
  // -ve
  float x = -100.0f;
  float result = klee_abs_float(x);
  assert(result == 100.0f);

  // +ve
  x = 100.0f;
  assert(!signbit(x));
  result = klee_abs_float(x);
  assert(result == 100.0f);

  // -ve zero
  x = -0.0f;
  assert(signbit(x));
  result = klee_abs_float(x);
  assert(result == 0.0f);
  assert(signbit(x));

  // +ve zero
  x = 0.0f;
  assert(!signbit(x));
  result = klee_abs_float(x);
  assert(result == 0.0f);
  assert(!signbit(x));

  // +NaN
  uint32_t posNanBits = 0x7fc00000;
  float posNan = 0.0f;
  assert(sizeof(posNanBits) == sizeof(posNan));
  memcpy(&posNan, &posNanBits, sizeof(float));
  assert(isnan(posNan));
  assert(!signbit(posNan));
  result = klee_abs_float(posNan);
  assert(isnan(result));
  assert(!signbit(result)); // is +ve NaN.

  // -NaN
  uint32_t negNanBits = 0xffc00000;
  float negNan = 0.0f;
  assert(sizeof(negNanBits) == sizeof(negNan));
  memcpy(&negNan, &negNanBits, sizeof(float));
  assert(isnan(negNan));
  assert(signbit(negNan));
  result = klee_abs_float(negNan);
  assert(isnan(result));
  assert(!signbit(result)); // is +ve NaN.

  // +Infinity
  x = INFINITY;
  assert(!signbit(x));
  result = klee_abs_float(x);
  assert(isinf(x) == 1);

  // -Infinity
  x = -INFINITY;
  assert(signbit(x));
  result = klee_abs_float(x);
  assert(isinf(x) == -1);

  return 0;
}
// CHECK: KLEE: done: completed paths = 1
