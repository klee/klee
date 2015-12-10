/*
 * ITree.h
 *
 *  Created on: Oct 15, 2015
 *      Author: felicia
 */

#ifndef ITREE_H_
#define ITREE_H_

#include "BlockTable.h"

#include <klee/Expr.h>
#include "klee/Config/Version.h"
#include "klee/ExecutionState.h"

#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 3)
#include "llvm/IR/Function.h"
#include "llvm/IR/Attributes.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/TypeBuilder.h"
#else
#include "llvm/Attributes.h"
#include "llvm/BasicBlock.h"
#include "llvm/Constants.h"
#include "llvm/Function.h"
#include "llvm/Instructions.h"
#include "llvm/IntrinsicInst.h"
#include "llvm/LLVMContext.h"
#include "llvm/Module.h"

#if LLVM_VERSION_CODE <= LLVM_VERSION(3, 1)
#include "llvm/Target/TargetData.h"
#else
#include "llvm/DataLayout.h"
#include "llvm/TypeBuilder.h"
#endif
#endif
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/Process.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Analysis/MemoryDependenceAnalysis.h"

#if LLVM_VERSION_CODE < LLVM_VERSION(3, 5)
#include "llvm/Support/CallSite.h"
#else
#include "llvm/IR/CallSite.h"
#endif

using namespace llvm;

namespace klee {
  class ExecutionState;

  class PathCondition {
    /// KLEE expression
    ref<Expr> constraint;

    /// Should this be included in an interpolant?
    bool inInterpolant;

    /// Previous basic block
    PathCondition *tail;

  public:
    PathCondition(ref<Expr>& constraint);

    PathCondition(ref<Expr>& constraint, PathCondition *prev);

    ~PathCondition();

    ref<Expr> car() const;

    PathCondition* cdr() const;

    void includeInInterpolant();

    bool carInInterpolant();

    std::vector< ref<Expr> > packInterpolant() const;

    void dump();

    void print(llvm::raw_ostream& stream);
  };

  class PathConditionMarker {
    bool mayBeInInterpolant;

    PathCondition *pathCondition;

  public:
    PathConditionMarker(PathCondition *pathCondition);

    ~PathConditionMarker();

    void includeInInterpolant();

    void mayIncludeInInterpolant();
  };

  class SubsumptionTableEntry {
    unsigned int nodeId;

    std::vector< ref<Expr> > interpolant;

  public:
    SubsumptionTableEntry(ITreeNode *node);

    ~SubsumptionTableEntry();

    bool subsumed(TimingSolver *solver, ExecutionState& state, double timeout);

    void dump() const;

    void print(llvm::raw_ostream &stream) const;
  };

  class ITree{
    typedef std::vector< ref<Expr> > ExprList;
    typedef ExprList::iterator iterator;
    typedef ExprList::const_iterator const_iterator;

    ITreeNode *currentINode;

    std::vector<SubsumptionTableEntry> subsumptionTable;

    BlockTable blockTable;

    void printNode(llvm::raw_ostream& stream, ITreeNode *n, std::string edges);

  public:
    ITreeNode *root;

    ITree(ExecutionState* _root);

    ~ITree();

    std::vector<SubsumptionTableEntry> getStore();

    void store(SubsumptionTableEntry subItem);

    void setCurrentINode(ITreeNode *node);

    void remove(ITreeNode *node);

    bool checkCurrentStateSubsumption(TimingSolver* solver, ExecutionState& state, double timeout);

    void markPathCondition(std::vector< ref<Expr> > unsat_core);

    void print(llvm::raw_ostream &stream);

    void dump();

    void recordBlock(Instruction *inst);

    void dumpBlock();
  };

  class ITreeNode{
    friend class ITree;
    typedef ref<Expr> expression_type;
    typedef std::pair <expression_type, expression_type> pair_type;
    PathCondition *pathCondition;
    ITreeNode *parent, *left, *right;
    unsigned int nodeId;
    bool isSubsumed;

  public:
    ExecutionState *data;

    unsigned int getNodeId();

    std::vector< ref<Expr> > getInterpolant() const;

    void setNodeLocation(unsigned int programPoint);

    void split(ExecutionState *leftData, ExecutionState *rightData);

    void dump() const;

    void print(llvm::raw_ostream &stream) const;

    std::map< ref<Expr>, PathConditionMarker *> makeMarkerMap();

    bool introducesMarkedConstraint();

  private:
    ITreeNode(ITreeNode *_parent, ExecutionState *_data);

    ~ITreeNode();

    void print(llvm::raw_ostream &stream, const unsigned int tab_num) const;

    std::string make_tabs(const unsigned int tab_num) const {
      std::string tabs_string;
      for (unsigned int i = 0; i < tab_num; i++) {
	  tabs_string += "\t";
      }
      return tabs_string;
    }

  };

}
#endif /* ITREE_H_ */
