//===-- RoundingModeUtil.h --------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#ifndef KLEE_ROUNDING_MODE_UTIL_H
#include "llvm/ADT/APFloat.h"

namespace klee {

int LLVMRoundingModeToCRoundingMode(llvm::APFloat::roundingMode rm);

const char *LLVMRoundingModeToString(llvm::APFloat::roundingMode rm);
} // namespace klee

#endif
