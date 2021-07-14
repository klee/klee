/*===-- klee_set_rounding_mode.c ------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===*/
#include "klee/klee.h"

void klee_set_rounding_mode_internal(enum KleeRoundingMode rm);

// This indirection is used here so we can easily support a symbolic rounding
// mode from clients but in the Executor we only need to worry about a concrete
// rounding mode.
void klee_set_rounding_mode(enum KleeRoundingMode rm) {
  // We have to be careful here to make sure we pass a constant
  // to klee_set_rounding_mode_internal().
  switch (rm) {
    case KLEE_FP_RNE:
      klee_set_rounding_mode_internal(KLEE_FP_RNE); break;
    case KLEE_FP_RNA:
      klee_set_rounding_mode_internal(KLEE_FP_RNA); break;
    case KLEE_FP_RU:
      klee_set_rounding_mode_internal(KLEE_FP_RU); break;
    case KLEE_FP_RD:
      klee_set_rounding_mode_internal(KLEE_FP_RD); break;
    case KLEE_FP_RZ:
      klee_set_rounding_mode_internal(KLEE_FP_RZ); break;
    default:
      klee_report_error(__FILE__, __LINE__, "Invalid rounding mode", "");
  }
}
