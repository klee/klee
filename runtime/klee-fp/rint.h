/*===-- rint.h ------------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===*/

#ifndef KLEE_RINT_H
#define KLEE_RINT_H

float rintf(float arg);
double rint(double arg);
long double rintl(long double arg);
float nearbyintf(float arg);
double nearbyint(double arg);
long double nearbyintl(long double arg);

#endif // KLEE_RINT_H
