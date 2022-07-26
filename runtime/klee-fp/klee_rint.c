/*===-- klee_rint.c -------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===*/

#include "klee_rint.h"
#include "klee/klee.h"

float klee_internal_rintf(float arg) { return klee_rintf(arg); }

double klee_internal_rint(double arg) { return klee_rint(arg); }

long double klee_internal_rintl(long double arg) { return klee_rintl(arg); }

float nearbyintf(float arg) { return klee_rintf(arg); }

double nearbyint(double arg) { return klee_rint(arg); }

long double nearbyintl(long double arg) { return klee_rintl(arg); }
