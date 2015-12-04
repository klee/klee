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

enum Status { NoInterpolant, HalfInterpolant, FullInterpolant};
enum Operation { Add, Sub, Mul, UDiv, SDiv, URem, SRem, And, Or, Xor, Shl, LShr, AShr};
enum Comparison {Eq, Ne, Ult, Ule, Ugt, Uge, Slt, Sle, Sgt, Sge, Neg, Not};

namespace klee {
  class ExecutionState;

  class ConstraintList {
    /// KLEE expression
    ref<Expr> constraint;

    /// Previous basic block
    ConstraintList *tail;

  public:
    ConstraintList(ref<Expr>& constraint);

    ConstraintList(ref<Expr>& constraint, ConstraintList *prev);

    ~ConstraintList();

    ref<Expr> car() const;

    ConstraintList* cdr() const;

    std::vector< ref<Expr> > pack() const;

    void dump();

    void print(llvm::raw_ostream& stream);
  };

  struct BranchCondition{
    ref<Expr> base;
    ref<Expr> value;
    Comparison compareName;
  };

  class SubsumptionTableEntry {
    unsigned int programPoint;

    std::vector< ref<Expr> > interpolant;

  public:
    SubsumptionTableEntry(ITreeNode *node);

    ~SubsumptionTableEntry();

    bool subsumed(ITreeNode *state);

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
    ConstraintList *pathCondition;
    ITreeNode *parent, *left, *right;

  public:
    unsigned int programPoint;
    ExecutionState *data;
    bool isSubsumed;
    BranchCondition latestBranchCond;

    std::vector< ref<Expr> > getInterpolant() const;

    void correctNodeLocation(unsigned int programPoint);

    void split(ExecutionState *leftData, ExecutionState *rightData);

    ITreeNode *getParent();

    ITreeNode *getLeft();

    ITreeNode *getRight();

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

}
#endif /* ITREE_H_ */
