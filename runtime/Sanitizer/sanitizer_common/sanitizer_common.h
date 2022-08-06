//===-- sanitizer_common.h --------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file is shared between run-time libraries of sanitizers.
//
// It declares common functions and classes that are used in both runtimes.
// Implementation of some functions are provided in sanitizer_common, while
// others must be defined by run-time library itself.
//===----------------------------------------------------------------------===//

// NOTE: Needs to be kept in sync with
// compiler-rt/lib/sanitizer_common/sanitizer_common.h from LLVM project.
// But in fact, nothing was used form that header, so leave it as a wrapper for
// used internal headers.

#ifndef SANITIZER_COMMON_H
#define SANITIZER_COMMON_H

#include "sanitizer_internal_defs.h"

#endif // SANITIZER_COMMON_H
