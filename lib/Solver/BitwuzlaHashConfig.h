//===-- BitwuzlaHashConfig.h -----------------------------------------*- C++
//-*-====//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_BITWUZLAHASHCONFIG_H
#define KLEE_BITWUZLAHASHCONFIG_H

#include "klee/Support/CompilerWarning.h"
DISABLE_WARNING_PUSH
DISABLE_WARNING_DEPRECATED_DECLARATIONS
#include "llvm/Support/CommandLine.h"
DISABLE_WARNING_POP

#include <atomic>

namespace BitwuzlaHashConfig {
extern llvm::cl::opt<bool> UseConstructHashBitwuzla;
} // namespace BitwuzlaHashConfig
#endif // KLEE_BITWUZLAHASHCONFIG_H
