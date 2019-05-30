//===-- ModuleUtil.h --------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_MODULEUTIL_H
#define KLEE_MODULEUTIL_H

#include "klee/Config/Version.h"

#include "llvm/IR/CallSite.h"
#include "llvm/IR/Module.h"

#include <memory>
#include <string>
#include <vector>

namespace klee {

/// Links all the modules together into one and returns it.
///
/// All the modules which are used for resolving entities are freed,
/// all the remaining ones are preserved.
///
/// @param modules List of modules to link together: if resolveOnly true,
/// everything is linked against the first entry.
/// @param entryFunction if set, missing functions of the module containing the
/// entry function will be solved.
/// @return final module or null in this case errorMsg is set
std::unique_ptr<llvm::Module>
linkModules(std::vector<std::unique_ptr<llvm::Module>> &modules,
            llvm::StringRef entryFunction, std::string &errorMsg);

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
              std::vector<std::unique_ptr<llvm::Module>> &modules,
              std::string &errorMsg);
}

#endif /* KLEE_MODULEUTIL_H */
