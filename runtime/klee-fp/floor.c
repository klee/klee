/*===-- floor.c
------------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===*/

#include "floor.h"
#include "klee/klee.h"
#include "math.h"

float floorf(float x) {
  int sign = signbit(x);
  x = klee_abs_float(x);
  if (klee_rintf(x) > x) {
    return sign * (klee_rintf(x) - 1);
  } else {
    return sign * klee_rintf(x);
  }
}

double floor(double x) {
  int sign = signbit(x);
  x = klee_abs_double(x);
  if (klee_rint(x) > x) {
    return sign * (klee_rint(x) - 1);
  } else {
    return sign * klee_rint(x);
  }
}

long double floorl(long double x) {
  int sign = signbit(x);
  x = klee_abs_long_double(x);
  if (klee_rintl(x) > x) {
    return sign * (klee_rintl(x) - 1);
  } else {
    return sign * klee_rintl(x);
  }
}
