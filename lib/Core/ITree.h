/*
 * ITree.h
 *
 *  Created on: Oct 15, 2015
 *      Author: felicia
 */

#ifndef ITREE_H_
#define ITREE_H_

#include <klee/Expr.h>
#include "klee/ExecutionState.h"
#include "klee/Config/Version.h"

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

enum Status { NoInterpolant, HalfInterpolant, FullInterpolant};
enum Operation { Add, Sub, Mul, UDiv, SDiv, URem, SRem, And, Or, Xor, Shl, LShr, AShr};
enum Comparison {Eq, Ne, Ult, Ule, Ugt, Uge, Slt, Sle, Sgt, Sge, Neg, Not};

namespace klee {
  class ExecutionState;

  class InstructionList {
    /// LLVM instruction
    Instruction& inst;

    /// Previous basic block
    InstructionList *tail;

  public:
    InstructionList(Instruction& inst);

    InstructionList(Instruction& inst, InstructionList *prev);

    ~InstructionList();

    Instruction& car() const;

    InstructionList* cdr() const;

    void dump();

    void print(llvm::raw_ostream& stream);
  };


  class UpdateRelation{
    ref<Expr> base;
    ref<Expr> baseLoc; //load location
    ref<Expr> value;
    ref<Expr> valueLoc;
    Operation operationName;
  public:
    UpdateRelation(const ref<Expr>& baseLoc, const ref<Expr>& value, const Operation& operationName);

    ~UpdateRelation();

    ref<Expr> makeExpr(ref<Expr> locToCompare, ref<Expr>& lhs) const;

    void setBase(const ref<Expr>& base);

    void setValueLoc(const ref<Expr>& valueLoc);

    ref<Expr> getBaseLoc() const;

    bool isBase(ref<Expr> expr) const;

    void dump() const;

    void print(llvm::raw_ostream &stream) const;
  };

  struct BranchCondition{
    ref<Expr> base;
    ref<Expr> value;
    Comparison compareName;
  };

  class SubsumptionTableEntry {
    unsigned int programPoint;

    ref<Expr> interpolant;

    std::pair< ref<Expr> , ref<Expr> > interpolantLoc;

  public:
    SubsumptionTableEntry(ITreeNode *node);

    ~SubsumptionTableEntry();

    bool subsumed(ExecutionState& state);

    void dump() const;

    void print(llvm::raw_ostream &stream) const;
  };

  class ITree{
    typedef std::vector< ref<Expr> > ExprList;
    typedef ExprList::iterator iterator;
    typedef ExprList::const_iterator const_iterator;

    ITreeNode *currentINode;
    std::vector<SubsumptionTableEntry> subsumptionTable;

  public:
    ITreeNode *root;

    ITree(ExecutionState* _root);

    ~ITree();

    std::pair<ITreeNode*, ITreeNode*> split(ITreeNode *node,
                                            ExecutionState* leftData,
                                            ExecutionState* rightData);

    std::vector<SubsumptionTableEntry> getStore();

    void store(SubsumptionTableEntry subItem);

    bool isCurrentNodeSubsumed();

    void setCurrentINode(ITreeNode *node);

    void checkCurrentNodeSubsumption();
  };

  class ITreeNode{
    friend class ITree;
    typedef ref<Expr> expression_type;
    typedef std::pair <expression_type, expression_type> pair_type;
    std::vector< UpdateRelation > newUpdateRelationsList;
    std::vector< UpdateRelation > updateRelationsList;
    ref<Expr> interpolant;
    std::pair< ref<Expr>, ref<Expr> > interpolantLoc;
    Status interpolantStatus;
    InstructionList *instructionList;

  public:
    unsigned int programPoint;
    ITreeNode *parent, *left, *right;
    ExecutionState *data;
    std::vector< ref<Expr> > dependenciesLoc;
    bool isSubsumed;
    std::vector <pair_type> variablesTracking;
    BranchCondition latestBranchCond;

    void addUpdateRelations(std::vector<UpdateRelation> addedUpdateRelations);

    void addUpdateRelations(ITreeNode *otherNode);

    void addNewUpdateRelation(UpdateRelation& updateRelation);

    void addStoredNewUpdateRelationsTo(std::vector<UpdateRelation>& relationsList);

    ref<Expr> buildUpdateExpression(ref<Expr>& lhs, ref<Expr> rhs);

    ref<Expr> buildNewUpdateExpression(ref<Expr>& lhs, ref<Expr> rhs);

    ref<Expr> getInterpolantBaseLocation(ref<Expr>& interpolant);

    void setInterpolantStatus(Status interpolantStatus);

    void setInterpolant(ref<Expr> interpolant);

    void setInterpolant(ref<Expr> interpolant, Status interpolantStatus);

    void setInterpolant(ref<Expr> interpolant, std::pair< ref<Expr>, ref<Expr> > interpolantLoc,
                            Status interpolantStatus);

    ref<Expr> &getInterpolant();

    std::pair< ref<Expr>, ref<Expr> > getInterpolantLoc();

    Status getInterpolantStatus();

    void correctNodeLocation(Instruction& inst, unsigned int programPoint);

    void dump() const;

    void print(llvm::raw_ostream &stream) const;

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

  ref<Expr> buildUpdateExpression(std::vector<UpdateRelation> updateRelationsList, ref<Expr>& lhs, ref<Expr> rhs);
}
#endif /* ITREE_H_ */
