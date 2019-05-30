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

#if LLVM_VERSION_CODE >= LLVM_VERSION(4, 0)
#  define KLEE_LLVM_CL_VAL_END
#else
#  define KLEE_LLVM_CL_VAL_END , clEnumValEnd
#endif

#if LLVM_VERSION_CODE >= LLVM_VERSION(5, 0)
#  define KLEE_LLVM_GOIF_TERMINATOR
#else
#  define KLEE_LLVM_GOIF_TERMINATOR , NULL
#endif

#endif /* KLEE_VERSION_H */
