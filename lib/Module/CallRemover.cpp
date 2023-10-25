//===-- CallRemover.cpp----------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "Passes.h"
#include <unordered_set>

namespace klee {

using namespace llvm;

char CallRemover::ID;

bool CallRemover::runOnModule(llvm::Module &M) {
  std::vector<std::string> badFuncs = {"llvm.dbg.declare", "llvm.dbg.label"};

  for (const auto &f : badFuncs) {
    auto Declare = M.getFunction(f);
    if (!Declare)
      continue;
    while (!Declare->use_empty()) {
      auto CI = cast<CallInst>(Declare->user_back());
      assert(CI->use_empty() && "deleted function must have void result");
      CI->eraseFromParent();
    }
    Declare->eraseFromParent();
  }

  return true;
}
} // namespace klee
