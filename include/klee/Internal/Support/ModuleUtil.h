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

#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LLVMContext.h"

#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 5)
#include "llvm/IR/CallSite.h"
#else
#include "llvm/Support/CallSite.h"
#endif

#include <string>

namespace klee {

/// Links all the modules together into one and returns it.
///
/// @param modules List of modules to link together
/// @param resolveOnly true, use all the modules, to solve unresolved methods
/// from the first module,
///                    false: merges all modules together
/// @return final module or null in this case errorMsg is set
llvm::Module *linkModules(std::vector<llvm::Module *> &modules,
                          bool resolveOnly, std::string &errorMsg);

/// Return the Function* target of a Call or Invoke instruction, or
/// null if it cannot be determined (should be only for indirect
/// calls, although complicated constant expressions might be
/// another possibility).
///
/// If `moduleIsFullyLinked` is set to true it will be assumed that the
///  module containing the `llvm::CallSite` is fully linked. This assumption
///  allows resolution of functions that are marked as overridable.
llvm::Function *getDirectCallTarget(llvm::CallSite, bool moduleIsFullyLinked);

/// Return true iff the given Function value is used in something
/// other than a direct call (or a constant expression that
/// terminates in a direct call).
bool functionEscapes(const llvm::Function *f);

/// Loads the file libraryName and reads all possible modules out of it.
///
/// Different file types are possible:
/// * .bc binary file
/// * .ll IR file
/// * .a archive containing .bc and .ll files
///
/// @param libraryName library to read
/// @param modules contains extracted modules
/// @param errorMsg contains the error description in case the file could not be
/// loaded
/// @return true if successful otherwise false
bool loadFile(const std::string &libraryName, llvm::LLVMContext &context,
              std::vector<llvm::Module *> &modules, std::string &errorMsg);

void checkModule(llvm::Module *m);
}

#endif
