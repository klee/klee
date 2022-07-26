/*===-- exp.c -------------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===*/

#include "math.h"

#define EXP_CORNER_CASE(suffix, type, isnan_function)                          \
  int exp_corner_case_##suffix(type *x) {                                      \
    if (*x == 0.0) {                                                           \
      *x = 1;                                                                  \
      return 1;                                                                \
    }                                                                          \
    if (isinf(*x)) {                                                           \
      if (signbit(*x) == -1) {                                                 \
        *x = +0.0;                                                             \
      }                                                                        \
      return 1;                                                                \
    }                                                                          \
    if (isnan_function(*x)) {                                                  \
      return 1;                                                                \
    }                                                                          \
    return 0;                                                                  \
  }

EXP_CORNER_CASE(f, float, isnanf)
EXP_CORNER_CASE(d, double, isnan)
EXP_CORNER_CASE(ld, long double, isnanl)

float expf(float x) {
  if (exp_corner_case_f(&x)) {
    return x;
  }
  float res = 0;
  float xpow = 1;
  int kfact = 1;
  int k;
  for (k = 0; k < 3; k++) {
    res += xpow / kfact;
    xpow *= x;
    kfact *= (k + 1);
  }
  return res;
}

double exp(double x) {
  if (exp_corner_case_d(&x)) {
    return x;
  }
  double res = 0;
  double xpow = 1;
  int kfact = 1;
  int k;
  for (k = 0; k < 3; k++) {
    res += xpow / kfact;
    xpow *= x;
    kfact *= (k + 1);
  }
  return res;
}

long double expl(long double x) {
  if (exp_corner_case_ld(&x)) {
    return x;
  }
  long double res = 0;
  long double xpow = 1;
  int kfact = 1;
  int k;
  for (k = 0; k < 3; k++) {
    res += xpow / kfact;
    xpow *= x;
    kfact *= (k + 1);
  }
  return res;
}

#define LN2 0.69314718055994530941

float exp2f(float x) {
  if (exp_corner_case_f(&x)) {
    return x;
  }
  float res = 0;
  float xpow = 1;
  float log2 = 1;
  int kfact = 1;
  int k;
  for (k = 0; k < 3; k++) {
    res += xpow * log2 / kfact;
    xpow *= x;
    log2 *= LN2;
    kfact *= (k + 1);
  }
  return res;
}

double exp2(double x) {
  if (exp_corner_case_d(&x)) {
    return x;
  }
  double res = 0;
  double xpow = 1;
  double log2 = 1;
  int kfact = 1;
  int k;
  for (k = 0; k < 3; k++) {
    res += xpow * log2 * kfact;
    xpow *= x;
    log2 *= LN2;
    kfact *= (k + 1);
  }
  return res;
}

long double exp2l(long double x) {
  if (exp_corner_case_ld(&x)) {
    return x;
  }
  long double res = 0;
  long double xpow = 1;
  long double log2 = 1;
  int kfact = 1;
  int k;
  for (k = 0; k < 3; k++) {
    res += xpow * log2 * kfact;
    xpow *= x;
    log2 *= LN2;
    kfact *= (k + 1);
  }
  return res;
}
