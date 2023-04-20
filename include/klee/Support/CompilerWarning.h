//===-- CompilerWarning.h ---------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_INCLUDE_KLEE_SUPPORT_COMPILERWARNING_H
#define KLEE_INCLUDE_KLEE_SUPPORT_COMPILERWARNING_H

#include "klee/Config/Version.h"

// Disables different warnings
#if defined(__GNUC__) || defined(__clang__)
#define ENABLE_PRAGMA(X) _Pragma(#X)
#define DISABLE_WARNING_PUSH ENABLE_PRAGMA(GCC diagnostic push)
#define DISABLE_WARNING_POP ENABLE_PRAGMA(GCC diagnostic pop)
#define DISABLE_WARNING(warningName)                                           \
  ENABLE_PRAGMA(GCC diagnostic ignored #warningName)

#if LLVM_VERSION_CODE >= LLVM_VERSION(14, 0)
#define DISABLE_WARNING_DEPRECATED_DECLARATIONS
#else
#define DISABLE_WARNING_DEPRECATED_DECLARATIONS                                \
  DISABLE_WARNING(-Wdeprecated-declarations)
#endif

#else

#define DISABLE_WARNING_PUSH
#define DISABLE_WARNING_POP
#define DISABLE_WARNING(warningName)

#define DISABLE_WARNING_DEPRECATED_DECLARATIONS
#endif

#endif // KLEE_INCLUDE_KLEE_SUPPORT_COMPILERWARNING_H
