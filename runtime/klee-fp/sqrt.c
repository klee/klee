/*===-- sqrt.c ------------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===*/
#include "klee/klee.h"

double klee_internal_sqrt(double d) { return klee_sqrt_double(d); }

float klee_internal_sqrtf(float f) { return klee_sqrt_float(f); }

#if defined(__x86_64__) || defined(__i386__)
long double klee_internal_sqrtl(long double f) {
  return klee_sqrt_long_double(f);
}
#endif
