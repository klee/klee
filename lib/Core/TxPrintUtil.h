//===--- TxPrintUtil.h - Memory location dependency -------------*- C++ -*-===//
//
//               The Tracer-X KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Functions to help display object contents.
///
//===----------------------------------------------------------------------===//

#ifndef KLEE_TXPRINTUTIL_H
#define KLEE_TXPRINTUTIL_H

#include "klee/Config/Version.h"

#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 3)
#include <llvm/IR/Value.h>
#else
#include <llvm/Value.h>
#endif

#include <string>

namespace klee {

/// \brief Output function name to the output stream
extern bool outputFunctionName(llvm::Value *value, llvm::raw_ostream &stream);

/// \brief Create 8 times padding amount of spaces
extern std::string makeTabs(const unsigned paddingAmount);

/// \brief Create 1 padding amount of spaces
extern std::string appendTab(const std::string &prefix);
}

#endif
