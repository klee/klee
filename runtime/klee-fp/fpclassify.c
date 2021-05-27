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
int klee_internal_isnanf(float f) {
  return klee_is_nan_float(f);
}

// __isnan
int klee_internal_isnan(double d) {
  return klee_is_nan_double(d);
}

// __isnanl
int klee_internal_isnanl(long double d) {
  return klee_is_nan_long_double(d);
}

// __fpclassifyf
int klee_internal_fpclassifyf(float f) {
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

// __fpclassify
int klee_internal_fpclassify(double f) {
  int b = klee_is_infinite_double(f);
  int c = (f == 0.0f);
  int d = klee_is_subnormal_double(f);
  int x = klee_is_normal_double(f);
  return ((x << 2) | ((c | d) << 1) | (b | d));
}

// __fpclassifyl
#if defined(__x86_64__) || defined(__i386__)
int klee_internal_fpclassifyl(long double f) {
  int b = klee_is_infinite_long_double(f);
  int c = (f == 0.0f);
  int d = klee_is_subnormal_long_double(f);
  int x = klee_is_normal_long_double(f);
  return ((x << 2) | ((c | d) << 1) | (b | d));
}
#endif

// __finitef
int klee_internal_finitef(float f) {
  return (!klee_is_nan_float(f)) & (!klee_is_infinite_float(f));
}

// __finite
int klee_internal_finite(double f) {
  return (!klee_is_nan_double(f)) & (!klee_is_infinite_double(f));
}

// __finitel
int klee_internal_finitel(long double f) {
  return (!klee_is_nan_long_double(f)) & (!klee_is_infinite_long_double(f));
}
