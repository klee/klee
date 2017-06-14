//===-- ModuleUtil.h --------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_MODULE_UTIL_H
#define KLEE_MODULE_UTIL_H

#include "klee/Config/Version.h"

#if LLVM_VERSION_CODE > LLVM_VERSION(3, 2)
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LLVMContext.h"
#else
#include "llvm/Module.h"
#include "llvm/Function.h"
#include "llvm/LLVMContext.h"
#endif

#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 5)
#include "llvm/IR/CallSite.h"
#else
#include "llvm/Support/CallSite.h"
#endif

#include <string>

namespace klee {
  /// Load llvm module from a bitcode archive file.
  llvm::Module *loadModule(llvm::LLVMContext &ctx,
                           const std::string &path,
                           std::string &errorMsg);

  /// Link a module with a specified bitcode archive.
  llvm::Module *linkWithLibrary(llvm::Module *module,
                                const std::string &libraryName);

  /// Return the Function* target of a Call or Invoke instruction, or
  /// null if it cannot be determined (should be only for indirect
  /// calls, although complicated constant expressions might be
  /// another possibility).
  ///
  /// If `moduleIsFullyLinked` is set to true it will be assumed that the
  //  module containing the `llvm::CallSite` is fully linked. This assumption
  //  allows resolution of functions that are marked as overridable.
  llvm::Function *getDirectCallTarget(llvm::CallSite, bool moduleIsFullyLinked);

  /// Return true iff the given Function value is used in something
  /// other than a direct call (or a constant expression that
  /// terminates in a direct call).
  bool functionEscapes(const llvm::Function *f);

}

#endif
