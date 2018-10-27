//===-- FunctionAliasPass.cpp -----------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "Passes.h"
#include "klee/Internal/Support/ErrorHandling.h"
#include "klee/OptionCategories.h"

#include "llvm/IR/GlobalAlias.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Regex.h"

using namespace llvm;

namespace {

cl::list<std::string>
    FunctionAlias("function-alias",
                  cl::desc("Replace functions that match name or regular "
                           "expression pattern with an alias to the function "
                           "whose name is given by replacement"),
                  cl::value_desc("name|pattern>:<replacement"),
                  cl::cat(klee::ModuleCat));

} // namespace

namespace klee {

bool FunctionAliasPass::runOnModule(Module &M) {
  bool modified = false;

#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 9)
  assert((M.ifunc_size() == 0) && "Unexpected ifunc");
#endif

  for (const auto &pair : FunctionAlias) {
    bool matchFound = false;

    // regex pattern can contain colons, e.g. in character classes,
    // but replacement function name cannot
    std::size_t lastColon = pair.rfind(':');
    if (lastColon == std::string::npos) {
      klee_error("function-alias: no replacement given");
    }
    std::string pattern = pair.substr(0, lastColon);
    std::string replacement = pair.substr(lastColon + 1);

    if (pattern.empty())
      klee_error("function-alias: name or pattern cannot be empty");
    if (replacement.empty())
      klee_error("function-alias: replacement cannot be empty");

    if (pattern == replacement) {
      klee_error("function-alias: @%s cannot replace itself", pattern.c_str());
    }

    // check if replacement function exists
    GlobalValue *replacementValue = M.getNamedValue(replacement);
    if (!isFunctionOrGlobalFunctionAlias(replacementValue))
      klee_error("function-alias: replacement function @%s could not be found",
                 replacement.c_str());

    // directly replace if pattern is not a regex
    GlobalValue *match = M.getNamedValue(pattern);
    if (isFunctionOrGlobalFunctionAlias(match)) {
      if (tryToReplace(match, replacementValue)) {
        modified = true;
        klee_message("function-alias: replaced @%s with @%s", pattern.c_str(),
                     replacement.c_str());
        continue;
      }
    }
    if (match != nullptr) {
      // pattern is not a regex, but no replacement was found
      klee_error("function-alias: no (replacable) match for '%s' found",
                 pattern.c_str());
    }

    Regex regex(pattern);
    std::string error;
    if (!regex.isValid(error)) {
      klee_error("function-alias: '%s' is not a valid regex: %s",
                 pattern.c_str(), error.c_str());
    }

    std::vector<GlobalValue *> matches;

    // find matches for regex
#if LLVM_VERSION_CODE >= LLVM_VERSION(5, 0)
    for (GlobalValue &global : M.global_values()) {
#else
    // chain iterators of alias list and function list
    auto firstIt = M.getAliasList().begin();
    auto firstIe = M.getAliasList().end();
    auto secondIt = M.getFunctionList().begin();
    auto secondIe = M.getFunctionList().end();
    for (bool firstList = true;;
         (firstList && (++firstIt != firstIe)) || (++secondIt != secondIe)) {
      GlobalValue *gv = nullptr;
      if (firstIt == firstIe)
        firstList = false;
      if (firstList) {
        gv = cast<GlobalValue>(&*firstIt);
      } else {
        if (secondIt == secondIe)
          break;
        gv = cast<GlobalValue>(&*secondIt);
      }
      GlobalValue &global = *gv;
#endif
      if (!global.hasName())
        continue;

      if (!isFunctionOrGlobalFunctionAlias(&global))
        continue;

      SmallVector<StringRef, 8> matchVec;
      bool match = regex.match(global.getName(), &matchVec);
      if (match && matchVec[0].str() == global.getName().str()) {
        if (global.getName().str() == replacement) {
          klee_warning("function-alias: do not replace @%s (matching '%s') "
                       "with @%s: cannot replace itself",
                       global.getName().str().c_str(), pattern.c_str(),
                       replacement.c_str());
          continue;
        }
        matches.push_back(&global);
      }
    }

    // replace all found regex matches
    for (GlobalValue *global : matches) {
      // name will be invalidated by tryToReplace
      std::string name = global->getName().str();
      if (tryToReplace(global, replacementValue)) {
        modified = true;
        matchFound = true;
        klee_message("function-alias: replaced @%s (matching '%s') with @%s",
                     name.c_str(), pattern.c_str(), replacement.c_str());
      }
    }

    if (!matchFound) {
      klee_error("function-alias: no (replacable) match for '%s' found",
                 pattern.c_str());
    }
  }

  return modified;
}

const FunctionType *FunctionAliasPass::getFunctionType(const GlobalValue *gv) {
  const Type *type = gv->getType();
  while (type->isPointerTy()) {
    const PointerType *ptr = cast<PointerType>(type);
    type = ptr->getElementType();
  }
  return cast<FunctionType>(type);
}

bool FunctionAliasPass::checkType(const GlobalValue *match,
                                  const GlobalValue *replacement) {
  const FunctionType *MFT = getFunctionType(match);
  const FunctionType *RFT = getFunctionType(replacement);
  assert(MFT != nullptr && RFT != nullptr);

  if (MFT->getReturnType() != RFT->getReturnType()) {
    klee_warning("function-alias: @%s could not be replaced with @%s: "
                 "return type differs",
                 match->getName().str().c_str(),
                 replacement->getName().str().c_str());
    return false;
  }

  if (MFT->isVarArg() != RFT->isVarArg()) {
    klee_warning("function-alias: @%s could not be replaced with @%s: "
                 "one has varargs while the other does not",
                 match->getName().str().c_str(),
                 replacement->getName().str().c_str());
    return false;
  }

  if (MFT->getNumParams() != RFT->getNumParams()) {
    klee_warning("function-alias: @%s could not be replaced with @%s: "
                 "number of parameters differs",
                 match->getName().str().c_str(),
                 replacement->getName().str().c_str());
    return false;
  }

  std::size_t numParams = MFT->getNumParams();
  for (std::size_t i = 0; i < numParams; ++i) {
    if (MFT->getParamType(i) != RFT->getParamType(i)) {
      klee_warning("function-alias: @%s could not be replaced with @%s: "
                   "parameter types differ",
                   match->getName().str().c_str(),
                   replacement->getName().str().c_str());
      return false;
    }
  }
  return true;
}

bool FunctionAliasPass::tryToReplace(GlobalValue *match,
                                     GlobalValue *replacement) {
  if (!checkType(match, replacement))
    return false;

  GlobalAlias *alias = GlobalAlias::create("", replacement);
  match->replaceAllUsesWith(alias);
  alias->takeName(match);
  match->eraseFromParent();

  return true;
}

bool FunctionAliasPass::isFunctionOrGlobalFunctionAlias(const GlobalValue *gv) {
  if (gv == nullptr)
    return false;

  if (isa<Function>(gv))
    return true;

  if (const auto *ga = dyn_cast<GlobalAlias>(gv)) {
    const auto *aliasee = dyn_cast<GlobalValue>(ga->getAliasee());
    if (!aliasee) {
      // check if GlobalAlias is alias bitcast
      const auto *cexpr = dyn_cast<ConstantExpr>(ga->getAliasee());
      if (!cexpr || !cexpr->isCast())
        return false;
      aliasee = dyn_cast<GlobalValue>(cexpr->getOperand(0));
    }
    return isFunctionOrGlobalFunctionAlias(aliasee);
  }

  return false;
}

char FunctionAliasPass::ID = 0;

} // namespace klee
