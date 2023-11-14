/*===-- ceil.c ------------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===*/

#include "float.h"
#include "floor.h"
#include "rint.h"

float ceilf(float f) {
  if (f == rintf(f)) {
    return f;
  }
  return ((f < 0.0f) ? -1 : 1) + floorf(f);
}

double ceil(double f) {
  if (f == rint(f)) {
    return f;
  }
  return ((f < 0.0f) ? -1 : 1) + floor(f);
}

long double ceill(long double f) {
  if (f == rintl(f)) {
    return f;
  }
  return ((f < 0.0f) ? -1 : 1) + floorl(f);
}
