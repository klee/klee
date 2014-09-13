//===-- Debug.h -------------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_INTERNAL_SUPPORT_DEBUG_H
#define KLEE_INTERNAL_SUPPORT_DEBUG_H

#include <klee/Config/config.h>
#include <llvm/Support/Debug.h>

// We define wrappers around the LLVM DEBUG macros that are conditionalized on
// whether the LLVM we are building against has the symbols needed by these
// checks.

#ifdef ENABLE_KLEE_DEBUG
#define KLEE_DEBUG_WITH_TYPE(TYPE, X) DEBUG_WITH_TYPE(TYPE, X)
#else
#define KLEE_DEBUG_WITH_TYPE(TYPE, X) do { } while (0)
#endif
#define KLEE_DEBUG(X) KLEE_DEBUG_WITH_TYPE(DEBUG_TYPE, X)

#endif
