/*===-- trigonometry.c ----------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===*/

double const eps = 1e-8;

double sin(double x) {
  double result = 0.0;
  double xpow = x;
  int kfact = 1;
  int k;
  for (k = 0; k < 3; k++) {
    int sign = (k % 2) ? -1 : 1;
    result += sign * xpow / kfact;
    xpow *= x * x;
    kfact *= (2 * k + 2) * (2 * k + 3);
  }
  return result;
}

double cos(double x) {
  double result = 0.0;
  double xpow = 1;
  int kfact = 1;
  int k;
  for (k = 0; k < 3; k++) {
    int sign = (k % 2) ? -1 : 1;
    result += sign * xpow / kfact;
    xpow *= x * x;
    kfact *= (2 * k + 1) * (2 * k + 2);
  }
  return result;
}
