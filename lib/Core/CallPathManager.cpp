//===-- CallPathManager.cpp -----------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "CallPathManager.h"

#include "klee/Statistics.h"

#include <map>
#include <vector>
#include "llvm/IR/Function.h"

#include "llvm/Support/raw_ostream.h"

using namespace klee;

///

CallPathNode::CallPathNode(CallPathNode *_parent,
                           const llvm::Instruction *_callSite,
                           const llvm::Function *_function)
    : parent(_parent), callSite(_callSite), function(_function), count(0) {}

void CallPathNode::print() {
  llvm::errs() << "  (Function: " << this->function->getName() << ", "
               << "Callsite: " << callSite << ", "
               << "Count: " << this->count << ")";
  if (parent && parent->callSite) {
    llvm::errs() << ";\n";
    parent->print();
  }
  else llvm::errs() << "\n";
}

///

CallPathManager::CallPathManager() : root(nullptr, nullptr, nullptr) {}

void CallPathManager::getSummaryStatistics(CallSiteSummaryTable &results) {
  results.clear();

  for (auto &path : paths)
    path->summaryStatistics = path->statistics;

  // compute summary bottom up, while building result table
  for (auto it = paths.rbegin(), ie = paths.rend(); it != ie; ++it) {
    const auto &cp = (*it);
    cp->parent->summaryStatistics += cp->summaryStatistics;

    CallSiteInfo &csi = results[cp->callSite][cp->function];
    csi.count += cp->count;
    csi.statistics += cp->summaryStatistics;
  }
}

CallPathNode *CallPathManager::computeCallPath(CallPathNode *parent,
                                               const llvm::Instruction *cs,
                                               const llvm::Function *f) {
  for (CallPathNode *p=parent; p; p=p->parent)
    if (cs==p->callSite && f==p->function)
      return p;

  auto cp = std::unique_ptr<CallPathNode>(new CallPathNode(parent, cs, f));
  auto newCP = cp.get();
  paths.emplace_back(std::move(cp));
  return newCP;
}

CallPathNode *CallPathManager::getCallPath(CallPathNode *parent,
                                           const llvm::Instruction *cs,
                                           const llvm::Function *f) {
  std::pair<const llvm::Instruction *, const llvm::Function *> key(cs, f);
  if (!parent)
    parent = &root;

  auto it = parent->children.find(key);
  if (it==parent->children.end()) {
    auto cp = computeCallPath(parent, cs, f);
    parent->children.insert(std::make_pair(key, cp));
    return cp;
  } else {
    return it->second;
  }
}

