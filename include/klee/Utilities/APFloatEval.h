//===-- APFloatEval.h -------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_APFLOAT_EVAL_H
#define KLEE_APFLOAT_EVAL_H

#include "klee/Support/CompilerWarning.h"
DISABLE_WARNING_PUSH
DISABLE_WARNING_DEPRECATED_DECLARATIONS
#include "llvm/ADT/APFloat.h"
#include "llvm/Support/ErrorHandling.h"
DISABLE_WARNING_POP

namespace klee {

llvm::APFloat evalSqrt(const llvm::APFloat &v, llvm::APFloat::roundingMode rm);

#if defined(__x86_64__) || defined(__i386__)
long double GetNativeX87FP80FromLLVMAPInt(const llvm::APInt &apint);
llvm::APInt GetAPIntFromLongDouble(long double ld);
#endif
} // namespace klee
#endif
