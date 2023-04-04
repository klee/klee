//===-- sanitizer_platform.h ------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Common platform macros.
//===----------------------------------------------------------------------===//

// NOTE: Needs to be kept in sync with
// compiler-rt/lib/sanitizer_common/sanitizer_platform.h from LLVM project.
// But in fact, only SANITIZER_WORDSIZE macro was used, so how it is.

#ifndef SANITIZER_PLATFORM_H
#define SANITIZER_PLATFORM_H

#if __LP64__ || defined(_WIN64)
#define SANITIZER_WORDSIZE 64
#else
#define SANITIZER_WORDSIZE 32
#endif

#endif // SANITIZER_PLATFORM_H
