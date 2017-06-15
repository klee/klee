//===-- InstructionInfoTable.h ----------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 7)
#include "llvm/IR/LegacyPassManager.h"
#else
#include "llvm/PassManager.h"
#endif

namespace klee {
#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 7)
typedef llvm::legacy::PassManager LegacyLLVMPassManagerTy;
#else
typedef llvm::PassManager LegacyLLVMPassManagerTy;
#endif
}
