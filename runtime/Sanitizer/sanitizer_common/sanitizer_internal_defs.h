//===-- sanitizer_internal_defs.h -------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file is shared between AddressSanitizer and ThreadSanitizer.
// It contains macro used in run-time libraries code.
//===----------------------------------------------------------------------===//

// NOTE: Needs to be kept in sync with
// compiler-rt/lib/sanitizer_common/sanitizer_internal_defs.h from LLVM project.
// But in fact, only typedefs for basic types were used, this is how it is.

#ifndef SANITIZER_DEFS_H
#define SANITIZER_DEFS_H

#include "sanitizer_platform.h"

// For portability reasons we do not include stddef.h, stdint.h or any other
// system header, but we do need some basic types that are not defined
// in a portable way by the language itself.
namespace __sanitizer {

#if defined(_WIN64)
// 64-bit Windows uses LLP64 data model.
typedef unsigned long long uptr;
typedef signed long long sptr;
#else
#if (SANITIZER_WORDSIZE == 64) || defined(__APPLE__) || defined(_WIN32)
typedef unsigned long uptr;
typedef signed long sptr;
#else
typedef unsigned int uptr;
typedef signed int sptr;
#endif
#endif // defined(_WIN64)

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;
typedef signed char s8;
typedef signed short s16;
typedef signed int s32;
typedef signed long long s64;

} // namespace __sanitizer

namespace __ubsan {
using namespace __sanitizer;
}

#endif // SANITIZER_DEFS_H
