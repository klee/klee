//===-- RoundingModeUtil.cpp ------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#include "klee/Support/RoundingModeUtil.h"

#include "klee/Support/CompilerWarning.h"
DISABLE_WARNING_PUSH
DISABLE_WARNING_DEPRECATED_DECLARATIONS
#include "llvm/ADT/APFloat.h"
#include "llvm/Support/ErrorHandling.h"
DISABLE_WARNING_POP

#include <cfenv>

namespace klee {

int LLVMRoundingModeToCRoundingMode(llvm::APFloat::roundingMode rm) {
  // FIXME: This function really ought to take a target so we aren't
  // tied to the host machine.
  switch (rm) {
  case llvm::APFloat::rmNearestTiesToEven:
    return FE_TONEAREST;
  case llvm::APFloat::rmNearestTiesToAway:
    return -1; // Not support on x86_64
  case llvm::APFloat::rmTowardPositive:
    return FE_UPWARD;
  case llvm::APFloat::rmTowardNegative:
    return FE_DOWNWARD;
  case llvm::APFloat::rmTowardZero:
    return FE_TOWARDZERO;
  default:
    llvm_unreachable("Invalid LLVM rounding mode");
  }
}

const char *LLVMRoundingModeToString(llvm::APFloat::roundingMode rm) {
  switch (rm) {
  case llvm::APFloat::rmNearestTiesToEven:
    return "RNE";
  case llvm::APFloat::rmNearestTiesToAway:
    return "RNA";
  case llvm::APFloat::rmTowardPositive:
    return "RU";
  case llvm::APFloat::rmTowardNegative:
    return "RD";
  case llvm::APFloat::rmTowardZero:
    return "RZ";
  default:
    llvm_unreachable("Invalid LLVM rounding mode");
  }
}
} // namespace klee
