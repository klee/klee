//===-- ubsan_value.h -------------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Representation of data which is passed from the compiler-generated calls into
// the ubsan runtime.
//
//===----------------------------------------------------------------------===//

// NOTE: Needs to be kept in sync with compiler-rt/lib/ubsan/ubsan_value.h
// from LLVM project.
// But in fact, only subset of TypeDescriptor was used and SourceLocation left
// for later work on specifying location in ubsan_handlers.cpp, so how it is.

#ifndef UBSAN_VALUE_H
#define UBSAN_VALUE_H

#include "../sanitizer_common/sanitizer_common.h"

namespace __ubsan {

/// \brief A description of a source location. This corresponds to Clang's
/// \c PresumedLoc type.
class SourceLocation {
  __attribute__((unused)) const char *Filename;
  __attribute__((unused)) u32 Line;
  __attribute__((unused)) u32 Column;

public:
  SourceLocation() : Filename(), Line(), Column() {}
  SourceLocation(const char *Filename, unsigned Line, unsigned Column)
      : Filename(Filename), Line(Line), Column(Column) {}
};

/// \brief A description of a type.
class TypeDescriptor {
  /// A value from the \c Kind enumeration, specifying what flavor of type we
  /// have.
  u16 TypeKind;

  /// A \c Type-specific value providing information which allows us to
  /// interpret the meaning of a ValueHandle of this type.
  u16 TypeInfo;

  /// The name of the type follows, in a format suitable for including in
  /// diagnostics.
  char TypeName[1];

public:
  enum Kind {
    /// An integer type. Lowest bit is 1 for a signed value, 0 for an unsigned
    /// value. Remaining bits are log_2(bit width). The value representation is
    /// the integer itself if it fits into a ValueHandle, and a pointer to the
    /// integer otherwise.
    TK_Integer = 0x0000,
    /// A floating-point type. Low 16 bits are bit width. The value
    /// representation is that of bitcasting the floating-point value to an
    /// integer type.
    TK_Float = 0x0001,
    /// Any other type. The value representation is unspecified.
    TK_Unknown = 0xffff
  };

  const char *getTypeName() const { return TypeName; }

  Kind getKind() const { return static_cast<Kind>(TypeKind); }

  bool isIntegerTy() const { return getKind() == TK_Integer; }
  bool isSignedIntegerTy() const { return isIntegerTy() && (TypeInfo & 1); }
  bool isUnsignedIntegerTy() const { return isIntegerTy() && !(TypeInfo & 1); }
};

/// \brief An opaque handle to a value.
typedef uptr ValueHandle;

} // namespace __ubsan

#endif // UBSAN_VALUE_H
