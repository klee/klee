/*===-- round.c -----------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===*/

#include "klee/klee.h"

float roundf(float x) { return klee_rintf(x); }

double round(double x) { return klee_rint(x); }

long double roundl(long double x) { return klee_rintl(x); }

long lroundf(float x) { return klee_rintf(x); }

long lround(double x) { return klee_rint(x); }

long lroundl(long double x) { return klee_rintl(x); }

long long llroundf(float x) { return klee_rintf(x); }

long long llround(double x) { return klee_rint(x); }

long long llroundl(long double x) { return klee_rintl(x); }
