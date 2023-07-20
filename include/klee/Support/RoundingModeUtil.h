//===-- RoundingModeUtil.h --------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#ifndef KLEE_ROUNDING_MODE_UTIL_H

#include "klee/Support/CompilerWarning.h"
DISABLE_WARNING_PUSH
DISABLE_WARNING_DEPRECATED_DECLARATIONS
#include "llvm/ADT/APFloat.h"
DISABLE_WARNING_POP

namespace klee {

int LLVMRoundingModeToCRoundingMode(llvm::APFloat::roundingMode rm);

const char *LLVMRoundingModeToString(llvm::APFloat::roundingMode rm);
} // namespace klee

#endif
