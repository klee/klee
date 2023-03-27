//===-- PrintVersion.h ------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_PRINTVERSION_H
#define KLEE_PRINTVERSION_H

#include "klee/Support/CompilerWarning.h"
DISABLE_WARNING_PUSH
DISABLE_WARNING_DEPRECATED_DECLARATIONS
#include "llvm/Support/raw_ostream.h"
DISABLE_WARNING_POP

#include "klee/Config/Version.h"

namespace klee {
  void printVersion(llvm::raw_ostream &OS);
}

#endif /* KLEE_PRINTVERSION_H */
