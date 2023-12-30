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

#if defined(__x86_64__) || defined(__i386__)
#include <fenv.h>
#elif defined(__arm64__) || defined(__arm__)
enum {
  FE_TONEAREST = 0x0,
  FE_UPWARD = 0x1,
  FE_DOWNWARD = 0x2,
  FE_TOWARDZERO = 0x3,

  // Our own implementation defined values.
  // Do we want this? Although it's allowed by
  // the standard it won't be replayable on native
  // binaries.
};
enum {
  FE_INVALID = 0x01,
  FE_DIVBYZERO = 0x02,
  FE_OVERFLOW = 0x04,
  FE_UNDERFLOW = 0x08,
  FE_INEXACT = 0x10,
  FE_DENORMAL = 0x80,
  FE_ALL_EXCEPT = FE_DIVBYZERO | FE_INEXACT | FE_INVALID | FE_OVERFLOW |
                  FE_UNDERFLOW | FE_DENORMAL
};
#else
#error Architecture not supported
#endif

int fegetround(void);
int fesetround(int rm);

#endif // KLEE_FENV_H
