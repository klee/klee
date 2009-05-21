//===-- CallPathManager.h ---------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef __UTIL_CALLPATHMANAGER_H__
#define __UTIL_CALLPATHMANAGER_H__

#include "klee/Statistics.h"

#include <map>
#include <vector>

namespace llvm {
  class Instruction;
  class Function;
}

namespace klee {
  class StatisticRecord;

  struct CallSiteInfo {
    unsigned count;
    StatisticRecord statistics;

  public:
    CallSiteInfo() : count(0) {}
  };

  typedef std::map<llvm::Instruction*,
                   std::map<llvm::Function*, CallSiteInfo> > CallSiteSummaryTable;    
  
  class CallPathNode {
    friend class CallPathManager;

  public:
    typedef std::map<std::pair<llvm::Instruction*, 
                               llvm::Function*>, CallPathNode*> children_ty;

    // form list of (callSite,function) path
    CallPathNode *parent;
    llvm::Instruction *callSite;
    llvm::Function *function;
    children_ty children;

    StatisticRecord statistics;
    StatisticRecord summaryStatistics;
    unsigned count;

  public:
    CallPathNode(CallPathNode *parent, 
                 llvm::Instruction *callSite,
                 llvm::Function *function);

    void print();
  };

  class CallPathManager {
    CallPathNode root;
    std::vector<CallPathNode*> paths;

  private:
    CallPathNode *computeCallPath(CallPathNode *parent, 
                                  llvm::Instruction *callSite,
                                  llvm::Function *f);
    
  public:
    CallPathManager();
    ~CallPathManager();

    void getSummaryStatistics(CallSiteSummaryTable &result);
    
    CallPathNode *getCallPath(CallPathNode *parent, 
                              llvm::Instruction *callSite,
                              llvm::Function *f);
  };
}

#endif
