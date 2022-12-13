/*===-- fabs.c ------------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===*/
#include "klee/klee.h"

double klee_internal_fabs(double d) { return klee_abs_double(d); }

float klee_internal_fabsf(float f) { return klee_abs_float(f); }

#if defined(__x86_64__) || defined(__i386__)
long double klee_internal_fabsl(long double f) {
  return klee_abs_long_double(f);
}
#endif
