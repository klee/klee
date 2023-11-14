/*===-- rint.c ------------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===*/

#include "rint.h"
#include "klee/klee.h"

float rintf(float arg) { return klee_rintf(arg); }

double rint(double arg) { return klee_rint(arg); }

long double rintl(long double arg) { return klee_rintl(arg); }

float nearbyintf(float arg) { return klee_rintf(arg); }

double nearbyint(double arg) { return klee_rint(arg); }

long double nearbyintl(long double arg) { return klee_rintl(arg); }
