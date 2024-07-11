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

#include "klee/Core/Interpreter.h"
#include "llvm/IR/InstrTypes.h"
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
/// @return false in this case errorMsg is set
bool linkModules(llvm::Module *composite,
                 std::vector<std::unique_ptr<llvm::Module>> &modules,
                 const unsigned Flags, std::string &errorMsg);

void replaceOrRenameFunction(llvm::Module *module, const char *old_name,
                             const char *new_name);

#if defined(__x86_64__) || defined(__i386__)
#define addIntrinsicReplacement(from, to)                                      \
  {"llvm." #from ".f32", #to "f"}, {"llvm." #from ".f64", #to}, {              \
    "llvm." #from ".f80", #to "l"                                              \
  }

#else
#define addIntrinsicReplacement(from, to)                                      \
  {"llvm." #from ".f32", #to "f"}, { "llvm." #from ".f64", #to }

#endif

/// Map for replacing std float computations to klee internals (avoid setting
/// concretes during calls below) There are some functions with provided
/// implementations in runtime/klee-fp, but not explicitly replaced here. Should
/// we rename them and complete the list?
const std::vector<std::pair<std::string, std::string>> floatReplacements = {
    addIntrinsicReplacement(sqrt, sqrt),
    addIntrinsicReplacement(rint, rint),
    addIntrinsicReplacement(round, rint),
    addIntrinsicReplacement(nearbyint, nearbyint),
    addIntrinsicReplacement(copysign, copysign),
    addIntrinsicReplacement(floor, floor),
    addIntrinsicReplacement(ceil, ceil)};
#undef addIntrinsicReplacement

/// Return the Function* target of a Call or Invoke instruction, or
/// null if it cannot be determined (should be only for indirect
/// calls, although complicated constant expressions might be
/// another possibility).
///
/// If `moduleIsFullyLinked` is set to true it will be assumed that the module
/// containing the `llvm::CallBase` is fully linked. This assumption allows
/// resolution of functions that are marked as overridable.
llvm::Function *getDirectCallTarget(const llvm::CallBase &cb,
                                    bool moduleIsFullyLinked);

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
/// @param context module context
/// @param modules contains extracted modules
/// @param errorMsg contains the error description in case the file could not be
/// loaded
/// @return true if successful otherwise false
bool loadFile(const std::string &libraryName, llvm::LLVMContext &context,
              std::vector<std::unique_ptr<llvm::Module>> &modules,
              std::string &errorMsg);

/// Loads the file libraryName and reads all modules into one.
///
/// Different file types are possible:
/// * .bc binary file
/// * .ll IR file
/// * .a archive containing .bc and .ll files
///
/// @param libraryName library to read
/// @param context module context
/// @param modules contains extracted modules
/// @param errorMsg contains the error description in case the file could not be
/// loaded
/// @return true if successful otherwise false
bool loadFileAsOneModule(const std::string &libraryName,
                         llvm::LLVMContext &context,
                         std::vector<std::unique_ptr<llvm::Module>> &modules,
                         std::string &errorMsg);

bool loadArchiveAsOneModule(const std::string &libraryName,
                            llvm::LLVMContext &context,
                            std::vector<std::unique_ptr<llvm::Module>> &modules,
                            Interpreter::FunctionsByModule &functionsByModule,
                            std::string &errorMsg);
} // namespace klee

#endif /* KLEE_MODULEUTIL_H */
