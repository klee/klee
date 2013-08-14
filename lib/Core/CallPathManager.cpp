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
#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 3)
#include "llvm/IR/Function.h"
#else
#include "llvm/Function.h"
#endif

#include "llvm/Support/raw_ostream.h"

using namespace llvm;
using namespace klee;

///

CallPathNode::CallPathNode(CallPathNode *_parent, 
                           Instruction *_callSite,
                           Function *_function)
  : parent(_parent),
    callSite(_callSite),
    function(_function),
    count(0) {
}

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

CallPathManager::CallPathManager() : root(0, 0, 0) {
}

CallPathManager::~CallPathManager() {
  for (std::vector<CallPathNode*>::iterator it = paths.begin(),
         ie = paths.end(); it != ie; ++it)
    delete *it;
}

void CallPathManager::getSummaryStatistics(CallSiteSummaryTable &results) {
  results.clear();

  for (std::vector<CallPathNode*>::iterator it = paths.begin(),
         ie = paths.end(); it != ie; ++it)
    (*it)->summaryStatistics = (*it)->statistics;

  // compute summary bottom up, while building result table
  for (std::vector<CallPathNode*>::reverse_iterator it = paths.rbegin(),
         ie = paths.rend(); it != ie; ++it) {
    CallPathNode *cp = *it;
    cp->parent->summaryStatistics += cp->summaryStatistics;

    CallSiteInfo &csi = results[cp->callSite][cp->function];
    csi.count += cp->count;
    csi.statistics += cp->summaryStatistics;
  }
}


CallPathNode *CallPathManager::computeCallPath(CallPathNode *parent, 
                                               Instruction *cs,
                                               Function *f) {
  for (CallPathNode *p=parent; p; p=p->parent)
    if (cs==p->callSite && f==p->function)
      return p;
  
  CallPathNode *cp = new CallPathNode(parent, cs, f);
  paths.push_back(cp);
  return cp;
}

CallPathNode *CallPathManager::getCallPath(CallPathNode *parent, 
                                           Instruction *cs,
                                           Function *f) {
  std::pair<Instruction*,Function*> key(cs, f);
  if (!parent)
    parent = &root;
  
  CallPathNode::children_ty::iterator it = parent->children.find(key);
  if (it==parent->children.end()) {
    CallPathNode *cp = computeCallPath(parent, cs, f);
    parent->children.insert(std::make_pair(key, cp));
    return cp;
  } else {
    return it->second;
  }
}

