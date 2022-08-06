//===-- ubsan_handlers.h ----------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Entry points to the runtime library for Clang's undefined behavior sanitizer.
//
//===----------------------------------------------------------------------===//

// NOTE: Needs to be kept in sync with compiler-rt/lib/ubsan/ubsan_handlers.h
// from LLVM project.

#ifndef UBSAN_HANDLERS_H
#define UBSAN_HANDLERS_H

#include "ubsan_value.h"

#include "klee/Config/Version.h"

using namespace __ubsan;

struct TypeMismatchData {
  SourceLocation Loc;
  const TypeDescriptor &Type;
  unsigned char LogAlignment;
  unsigned char TypeCheckKind;
};

struct AlignmentAssumptionData {
  SourceLocation Loc;
  SourceLocation AssumptionLoc;
  const TypeDescriptor &Type;
};

struct OverflowData {
  SourceLocation Loc;
  const TypeDescriptor &Type;
};

struct ShiftOutOfBoundsData {
  SourceLocation Loc;
  const TypeDescriptor &LHSType;
  const TypeDescriptor &RHSType;
};

struct OutOfBoundsData {
  SourceLocation Loc;
  const TypeDescriptor &ArrayType;
  const TypeDescriptor &IndexType;
};

struct UnreachableData {
  SourceLocation Loc;
};

struct VLABoundData {
  SourceLocation Loc;
  const TypeDescriptor &Type;
};

struct InvalidValueData {
  SourceLocation Loc;
  const TypeDescriptor &Type;
};

/// Known implicit conversion check kinds.
/// Keep in sync with the enum of the same name in CGExprScalar.cpp
enum ImplicitConversionCheckKind : unsigned char {
  ICCK_IntegerTruncation = 0, // Legacy, was only used by clang 7.
  ICCK_UnsignedIntegerTruncation = 1,
  ICCK_SignedIntegerTruncation = 2,
  ICCK_IntegerSignChange = 3,
  ICCK_SignedIntegerTruncationOrSignChange = 4,
};

struct ImplicitConversionData {
  SourceLocation Loc;
  const TypeDescriptor &FromType;
  const TypeDescriptor &ToType;
  /* ImplicitConversionCheckKind */ unsigned char Kind;
};

struct InvalidBuiltinData {
  SourceLocation Loc;
  unsigned char Kind;
};

struct NonNullReturnData {
  SourceLocation AttrLoc;
};

struct NonNullArgData {
  SourceLocation Loc;
  SourceLocation AttrLoc;
  int ArgIndex;
};

struct PointerOverflowData {
  SourceLocation Loc;
};

#endif // UBSAN_HANDLERS_H
