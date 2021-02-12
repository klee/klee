/*===-- fenv.c ------------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===*/
#include "klee/klee.h"
#include "klee_fenv.h"

// Define the constants. Don't include `fenv.h` here to avoid
// polluting the Intrinsic module.
#if defined(__x86_64__) || defined(__i386__)
// These are the constants used by glibc and musl for x86_64 and i386
enum {
  FE_TONEAREST = 0,
  FE_DOWNWARD = 0x400,
  FE_UPWARD = 0x800,
  FE_TOWARDZERO = 0xc00,

  // Our own implementation defined values.
  // Do we want this? Although it's allowed by
  // the standard it won't be replayable on native
  // binaries.
};
#else
#error Architecture not supported
#endif


int klee_internal_fegetround(void) {
  enum KleeRoundingMode rm = klee_get_rounding_mode();
  switch(rm) {
    case KLEE_FP_RNE:
      return FE_TONEAREST;
// Don't allow setting this mode for now.
// It won't be reproducible on native hardware
// so there's probably no point in supporting it
// via this interface.
//
//    case KLEE_FP_RNA:
//      return FE_TONEAREST_TIES_TO_AWAY;
    case KLEE_FP_RU:
      return FE_UPWARD;
    case KLEE_FP_RD:
      return FE_DOWNWARD;
    case KLEE_FP_RZ:
      return FE_TOWARDZERO;
    default:
      // The rounding mode could not be determined.
      return -1;
  }
}

int klee_internal_fesetround(int rm) {
  switch (rm) {
    case FE_TONEAREST:
      klee_set_rounding_mode(KLEE_FP_RNE);
      break;
    // Don't allow setting this mode for now.
    // It won't be reproducible on native hardware
    // so there's probably no point in supporting it
    // via this interface.
    //
    //case FE_TONEAREST_TIES_TO_AWAY:
    //  klee_set_rounding_mode(KLEE_FP_RNA);
    //  break;
    case FE_UPWARD:
      klee_set_rounding_mode(KLEE_FP_RU);
      break;
    case FE_DOWNWARD:
      klee_set_rounding_mode(KLEE_FP_RD);
      break;
    case FE_TOWARDZERO:
      klee_set_rounding_mode(KLEE_FP_RZ);
      break;
    default:
      // Can't set
      return -1;
  }
  return 0;
}
