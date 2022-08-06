//===-- ubsan_diag.h --------------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Diagnostics emission for Clang's undefined behavior sanitizer.
//
//===----------------------------------------------------------------------===//

// NOTE: Needs to be kept in sync with compiler-rt/lib/ubsan/ubsan_diag.h
// from LLVM project.
// But in fact, only TypeName in TypeDescriptor and ErrorType commonly were used,
// so how it is.

#ifndef UBSAN_DIAG_H
#define UBSAN_DIAG_H

#include "ubsan_value.h"

namespace __ubsan {

/// \brief A C++ type name. Really just a strong typedef for 'const char*'.
class TypeName {
  const char *Name;

public:
  TypeName(const char *Name) : Name(Name) {}
  const char *getName() const { return Name; }
};

enum class ErrorType {
#define UBSAN_CHECK(Name, SummaryKind, FSanitizeFlagName) Name,
#include "ubsan_checks.inc"
#undef UBSAN_CHECK
};

} // namespace __ubsan

#endif // UBSAN_DIAG_H
