// RUN: %clang %s -emit-llvm -O0 -g -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --libc=klee --fp-runtime --output-dir=%t.klee-out --exit-on-error %t1.bc > %t-output.txt 2>&1
// RUN: FileCheck -input-file=%t-output.txt %s
// REQUIRES: x86_64
// REQUIRES: fp-runtime
#include "klee/klee.h"
#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

long double make_long_double(uint16_t highBits, uint64_t lowBits) {
  long double v = 0.0l;
  memcpy(&v, &lowBits, sizeof(lowBits));       // 64 bits
  memcpy(((uint8_t *)(&v)) + 8, &highBits, 2); // 16 bits
  return v;
}

int main() {
  // -ve
  long double x = -100.0;
  long double result = klee_abs_long_double(x);
  assert(result == 100.0l);

  // +ve
  x = 100.0l;
  assert(!signbit(x));
  result = klee_abs_long_double(x);
  assert(result == 100.0l);

  // -ve zero
  x = -0.0l;
  assert(signbit(x));
  result = klee_abs_long_double(x);
  assert(result == 0.0l);
  assert(signbit(x));

  // +ve zero
  x = 0.0l;
  assert(!signbit(x));
  result = klee_abs_long_double(x);
  assert(result == 0.0l);
  assert(!signbit(x));

  // +NaN
  long double posNan = make_long_double(0x7fff, 0xc000000000000000);
  assert(isnan(posNan));
  assert(!signbit(posNan));
  result = klee_abs_long_double(posNan);
  assert(isnan(result));
  assert(!signbit(result)); // is +ve NaN.

  // -NaN
  long double negNan = make_long_double(0xffff, 0xc000000000000000);
  assert(isnan(negNan));
  assert(signbit(negNan));
  result = klee_abs_long_double(negNan);
  assert(isnan(result));
  assert(!signbit(result)); // is +ve NaN.

  // +Infinity
  x = INFINITY;
  assert(!signbit(x));
  result = klee_abs_long_double(x);
  assert(isinf(x) == 1);

  // -Infinity
  x = -INFINITY;
  assert(signbit(x));
  result = klee_abs_long_double(x);
  assert(isinf(x) == -1);

  return 0;
}
// CHECK: KLEE: done: completed paths = 1
