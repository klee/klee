//===-- Version.h -----------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_VERSION_H
#define KLEE_VERSION_H

#include "klee/Config/config.h"

#define LLVM_VERSION(major, minor) (((major) << 8) | (minor))
#define LLVM_VERSION_CODE LLVM_VERSION(LLVM_VERSION_MAJOR, LLVM_VERSION_MINOR)

#endif /* KLEE_VERSION_H */
