//===-- Z3HashConfig.h -----------------------------------------*- C++ -*-====//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_Z3HASHCONFIG_H
#define KLEE_Z3HASHCONFIG_H

#include "klee/Support/CompilerWarning.h"
DISABLE_WARNING_PUSH
DISABLE_WARNING_DEPRECATED_DECLARATIONS
#include "llvm/Support/CommandLine.h"
DISABLE_WARNING_POP

#include <atomic>

namespace Z3HashConfig {
extern llvm::cl::opt<bool> UseConstructHashZ3;
extern std::atomic<bool> Z3InteractionLogOpen;
} // namespace Z3HashConfig
#endif // KLEE_Z3HASHCONFIG_H
