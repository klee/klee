/*===-- ceil.c ------------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===*/

#include "float.h"
#include "klee_floor.h"
#include "klee_rint.h"

float ceilf(float f) {
  if (f == klee_internal_rintf(f)) {
    return f;
  }
  return ((f < 0.0f) ? -1 : 1) + klee_floorf(f);
}

double ceil(double f) {
  if (f == klee_internal_rint(f)) {
    return f;
  }
  return ((f < 0.0f) ? -1 : 1) + klee_floor(f);
}

long double ceill(long double f) {
  if (f == klee_internal_rintl(f)) {
    return f;
  }
  return ((f < 0.0f) ? -1 : 1) + klee_floorl(f);
}
