//===-- WorkaroundLLVMPR39177.cpp -------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

// This pass provides a workaround for LLVM bug PR39177 within the KLEE repo.
// For more information on this, please refer to the comments in
// cmake/workaround_llvm_pr39177.cmake

#include "Passes.h"
#include "klee/Internal/Support/ErrorHandling.h"

#include "llvm/Transforms/Utils/Cloning.h"

using namespace llvm;

namespace klee {

bool WorkaroundLLVMPR39177Pass::runOnModule(Module &M) {
  bool modified = false;

  const char *libfunctions[] = {
    "strlen",
    "strchr",
    "strncmp",
    "strcpy",
    "strncpy",
    "__memcpy_chk",
    "memchr",
    "memcmp",
    "putchar",
    "puts",
    "fputc",
    "fputc_unlocked",
    "fputs",
    "fputs_unlocked",
    "fwrite",
    "malloc",
    "calloc",
    "fwrite_unlocked",
    "fgetc_unlocked",
    "fgets_unlocked",
    "fread_unlocked",
    "memset_pattern16",
    "fopen"
  };

  for (auto *funcname : libfunctions) {
    if (M.getFunction(funcname) != nullptr)
      continue;

    GlobalValue *gv = M.getNamedValue(funcname);
    auto *alias = dyn_cast_or_null<GlobalAlias>(gv);
    if (alias == nullptr)
      continue;

    // get aliasee function if exists
    while (auto *ga = dyn_cast<GlobalAlias>(alias->getAliasee())) {
      assert(ga != alias && "alias pointing to itself");
      alias = ga;
    }
    Function *f = dyn_cast<Function>(alias->getAliasee());
    if (f == nullptr)
      continue;

    std::string aliasName = alias->getName().str();

    // clone function
    ValueToValueMapTy VMap;
    Function *g = CloneFunction(f, VMap);

    // replace alias with cloned function
    alias->replaceAllUsesWith(g);
    g->takeName(alias);
    alias->eraseFromParent();

    klee_message(
        "WorkaroundLLVMPR39177: replaced alias @%s with clone of function @%s",
        aliasName.c_str(), f->getName().str().c_str());
    modified = true;
  }

  return modified;
}

char WorkaroundLLVMPR39177Pass::ID = 0;

} // namespace klee
