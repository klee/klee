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

namespace klee {
  class ExecutionState;

  class PathConditionMarker {
    bool inInterpolant;

    PathCondition *pathCondition;

  public:
    PathConditionMarker(PathCondition *pathCondition);

    ~PathConditionMarker();

    void includeInInterpolant();
  };

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

    std::vector< ref<Expr> > packInterpolant() const;

    void dump();

    void print(llvm::raw_ostream& stream);
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
  };

  class ITreeNode{
    friend class ITree;
    typedef ref<Expr> expression_type;
    typedef std::pair <expression_type, expression_type> pair_type;
    PathCondition *pathCondition;
    ITreeNode *parent, *left, *right;
    unsigned int nodeId;

  public:
    ExecutionState *data;

    unsigned int getNodeId();

    std::vector< ref<Expr> > getInterpolant() const;

    void setNodeLocation(unsigned int programPoint);

    void split(ExecutionState *leftData, ExecutionState *rightData);

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
