/*===-- copysign.h --------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===*/

#ifndef KLEE_COPYSIGN_H
#define KLEE_COPYSIGN_H

float copysignf(float x, float y);
double copysign(double x, double y);
long double copysignl(long double x, long double y);

#endif // KLEE_COPYSIGN_H
