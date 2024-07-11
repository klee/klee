/*===-- fpclassify.c ------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===*/
#include "klee/klee.h"

// These are implementations of internal functions found in libm for classifying
// floating point numbers. They have different names to avoid name collisions
// during linking.

// __isnanf
int __isnanf(float f) { return klee_is_nan_float(f); }
int isnanf(float f) { return __isnanf(f); }

// __isnan
int __isnan(double d) { return klee_is_nan_double(d); }
int isnan(double d) { return __isnan(d); }

// __isnanl
int __isnanl(long double d) { return klee_is_nan_long_double(d); }
int isnanl(long double d) { return __isnanl(d); }

// __fpclassifyf
int __fpclassifyf(float f) {
  /*
   * This version acts like a switch case which returns correct
   * float type from the enum, but itself does not fork
   */
  int b = klee_is_infinite_float(f);
  int c = (f == 0.0f);
  int d = klee_is_subnormal_float(f);
  int x = klee_is_normal_float(f);
  return ((x << 2) | ((c | d) << 1) | (b | d));
}
int fpclassifyf(float f) { return __fpclassifyf(f); }

// __fpclassify
int __fpclassify(double f) {
  int b = klee_is_infinite_double(f);
  int c = (f == 0.0f);
  int d = klee_is_subnormal_double(f);
  int x = klee_is_normal_double(f);
  return ((x << 2) | ((c | d) << 1) | (b | d));
}
int fpclassify(double f) { return __fpclassify(f); }

// __fpclassifyl
#if defined(__x86_64__) || defined(__i386__)
int __fpclassifyl(long double f) {
  int b = klee_is_infinite_long_double(f);
  int c = (f == 0.0f);
  int d = klee_is_subnormal_long_double(f);
  int x = klee_is_normal_long_double(f);
  return ((x << 2) | ((c | d) << 1) | (b | d));
}
int fpclassifyl(long double f) { return __fpclassifyl(f); }
#endif

// __finitef
int __finitef(float f) {
  return (!klee_is_nan_float(f)) && (!klee_is_infinite_float(f));
}
int finitef(float f) { return __finitef(f); }

// __finite
int __finite(double f) {
  return (!klee_is_nan_double(f)) && (!klee_is_infinite_double(f));
}
int finite(double f) { return __finite(f); }

// __finitel
int __finitel(long double f) {
  return (!klee_is_nan_long_double(f)) && (!klee_is_infinite_long_double(f));
}
int finitel(long double f) { return __finitel(f); }
