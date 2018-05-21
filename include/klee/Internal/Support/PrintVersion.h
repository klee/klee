//===-- Version.h -----------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_PRINT_VERSION_H
#define KLEE_PRINT_VERSION_H

#include "llvm/Support/raw_ostream.h"

#include "klee/Config/Version.h"

namespace klee {
#if LLVM_VERSION_CODE >= LLVM_VERSION(6, 0)
  void printVersion(llvm::raw_ostream &OS);
#else
  void printVersion();
#endif
}

#endif
