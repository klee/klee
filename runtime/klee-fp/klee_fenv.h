/*===-- klee_fenv.h -------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===*/

#ifndef KLEE_FENV_H
#define KLEE_FENV_H
#include "klee/klee.h"

int klee_internal_fegetround(void);
int klee_internal_fesetround(int rm);

#endif //KLEE_FENV_H
