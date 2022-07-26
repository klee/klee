/*===-- log.c -------------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===*/

#include "fenv.h"
#include "klee_fenv.h"
#include "math.h"

#define LOG_CORNER_CASE(suffix, type, isnan_function)                          \
  int log_corner_case_##suffix(type *x) {                                      \
    if (isinf(*x) || isnan_function(*x)) {                                     \
      return 1;                                                                \
    }                                                                          \
    if (*x == 0.0) {                                                           \
      feraiseexcept(FE_DIVBYZERO);                                             \
      *x = -INFINITY;                                                          \
      return 1;                                                                \
    }                                                                          \
    if (*x == 1.0) {                                                           \
      *x = +0.0;                                                               \
      return 1;                                                                \
    }                                                                          \
    if (signbit(*x) == -1) {                                                   \
      feraiseexcept(FE_INVALID);                                               \
      *x = NAN;                                                                \
      return 1;                                                                \
    }                                                                          \
    return 0;                                                                  \
  }

LOG_CORNER_CASE(f, float, isnanf)
LOG_CORNER_CASE(d, double, isnan)
LOG_CORNER_CASE(ld, long double, isnan)

float logf(float x) {
  if (log_corner_case_f(&x)) {
    return x;
  }
  float res = 0;
  float xpow = x - 1;
  int k;
  for (k = 1; k < 6; k++) {
    int sign = (k % 2) ? -1 : 1;
    res += sign * xpow / k;
    xpow *= (x - 1);
  }
  return -res;
}

double log(double x) {
  if (log_corner_case_d(&x)) {
    return x;
  }
  double res = 0;
  double xpow = x - 1;
  int k;
  for (k = 1; k < 6; k++) {
    int sign = (k % 2) ? -1 : 1;
    res += sign * xpow / k;
    xpow *= (x - 1);
  }
  return -res;
}

long double logl(long double x) {
  if (log_corner_case_ld(&x)) {
    return x;
  }
  long double res = 0;
  long double xpow = x - 1;
  int k;
  for (k = 1; k < 6; k++) {
    int sign = (k % 2) ? -1 : 1;
    res += sign * xpow / k;
    xpow *= (x - 1);
  }
  return -res;
}

#define LN2 0.69314718055994530941
#define LN10 2.30258509299404568401

float log10f(float x) { return logf(x) / LN10; }

double log10(double x) { return log(x) / LN10; }

long double log10l(long double x) { return logl(x) / LN10; }
float log2f(float x) { return logf(x) / LN2; }

double log2(double x) { return log(x) / LN2; }

long double log2l(long double x) { return logl(x) / LN2; }
