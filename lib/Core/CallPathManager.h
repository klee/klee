//===-- CallPathManager.h ---------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_CALLPATHMANAGER_H
#define KLEE_CALLPATHMANAGER_H

#include "klee/Statistics.h"

#include <map>
#include <memory>
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

  typedef std::map<const llvm::Instruction *,
                   std::map<const llvm::Function *, CallSiteInfo>>
      CallSiteSummaryTable;

  class CallPathNode {
    friend class CallPathManager;

  public:
    typedef std::map<
        std::pair<const llvm::Instruction *, const llvm::Function *>,
        CallPathNode *>
        children_ty;

    // form list of (callSite,function) path
    CallPathNode *parent;
    const llvm::Instruction *callSite;
    const llvm::Function *function;
    children_ty children;

    StatisticRecord statistics;
    StatisticRecord summaryStatistics;
    unsigned count;

  public:
    CallPathNode(CallPathNode *parent, const llvm::Instruction *callSite,
                 const llvm::Function *function);

    void print();
  };

  class CallPathManager {
    CallPathNode root;
    std::vector<std::unique_ptr<CallPathNode>> paths;

  private:
    CallPathNode *computeCallPath(CallPathNode *parent,
                                  const llvm::Instruction *callSite,
                                  const llvm::Function *f);

  public:
    CallPathManager();
    ~CallPathManager() = default;

    void getSummaryStatistics(CallSiteSummaryTable &result);

    CallPathNode *getCallPath(CallPathNode *parent,
                              const llvm::Instruction *callSite,
                              const llvm::Function *f);
  };
}

#endif /* KLEE_CALLPATHMANAGER_H */
