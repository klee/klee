/*===-- klee_rint.h -------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===*/

#ifndef KLEE_RINT_H
#define KLEE_RINT_H

float klee_internal_rintf(float arg);
double klee_internal_rint(double arg);
long double klee_internal_rintl(long double arg);
float nearbyintf(float arg);
double nearbyint(double arg);
long double nearbyintl(long double arg);

#endif // KLEE_RINT_H
