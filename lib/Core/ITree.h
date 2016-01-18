/*
 * ITree.h
 *
 *  Created on: Oct 15, 2015
 *      Author: felicia
 */

#ifndef ITREE_H_
#define ITREE_H_

#include <klee/Expr.h>
#include "klee/Config/Version.h"
#include "klee/ExecutionState.h"

#include "Dependency.h"

using namespace llvm;

namespace klee {
  class ExecutionState;

  class ShadowArray {
    static std::map<const Array *, const Array *> shadowArray;

  public:
    static void addShadowArrayMap(const Array *source, const Array *target);

    static ref<Expr> getShadowExpression(ref<Expr> expr);
  };

  class PathCondition {
    /// @brief KLEE expression
    ref<Expr> constraint;

    /// @brief KLEE expression with variables (arrays) replaced by their shadows
    ref<Expr> shadowConstraint;

    /// @brief If shadow consraint had been generated: We generate shadow
    /// constraint
    /// on demand only when the constraint is required in an interpolant
    bool shadowed;

    /// @brief The dependency information for the current
    /// interpolation tree node
    Dependency *dependency;

    /// @brief the condition value from which the
    /// constraint was generated
    VersionedValue *condition;

    /// @brief When true, indicates that the constraint should be included
    /// in the interpolant
    bool inInterpolant;

    /// @brief Previous path condition
    PathCondition *tail;

  public:
    PathCondition(ref<Expr> &constraint, Dependency *dependency,
                  llvm::Value *condition, PathCondition *prev);

    ~PathCondition();

    ref<Expr> car() const;

    PathCondition* cdr() const;

    void includeInInterpolant();

    bool carInInterpolant() const;

    std::vector< ref<Expr> > packInterpolant();

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

    std::map<llvm::Value *, ref<Expr> > singletonStore;

    std::vector<llvm::Value *> singletonStoreKeys;

    std::map<llvm::Value *, std::vector<ref<Expr> > > compositeStore;

    std::vector<llvm::Value *> compositeStoreKeys;

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

    void markPathCondition(ExecutionState &state, TimingSolver *solver);

    std::pair<ITreeNode *, ITreeNode *> split(ITreeNode *parent, ExecutionState *left, ExecutionState *right);

    void executeAbstractDependency(llvm::Instruction *instr, ref<Expr> value);

    void print(llvm::raw_ostream &stream);

    void dump();
  };

  class ITreeNode{
    friend class ITree;

    friend class ExecutionState;

    typedef ref<Expr> expression_type;

    typedef std::pair <expression_type, expression_type> pair_type;

    /// @brief The path condition
    PathCondition *pathCondition;

    /// @brief Abstract stack for value dependencies
    Dependency *dependency;

    ITreeNode *parent, *left, *right;

    unsigned int nodeId;

    bool isSubsumed;

  public:

    unsigned int getNodeId();

    std::vector< ref<Expr> > getInterpolant() const;

    void setNodeLocation(unsigned int programPoint);

    void addConstraint(ref<Expr> &constraint, llvm::Value *value);

    void split(ExecutionState *leftData, ExecutionState *rightData);

    void dump() const;

    void print(llvm::raw_ostream &stream) const;

    std::map< ref<Expr>, PathConditionMarker *> makeMarkerMap();

    bool introducesMarkedConstraint();

    void executeAbstractDependency(llvm::Instruction *instr, ref<Expr> value);

    void bindCallArguments(llvm::Instruction *site,
                           std::vector<ref<Expr> > &arguments);

    void popAbstractDependencyFrame(llvm::CallInst *site,
                                    llvm::Instruction *inst,
                                    ref<Expr> returnValue);

    std::map<llvm::Value *, ref<Expr> >
    getLatestCoreExpressions(bool interpolantValueOnly = false) const;

    std::map<llvm::Value *, std::vector<ref<Expr> > >
    getCompositeCoreExpressions(bool interpolantValueOnly = false) const;

  private:
    ITreeNode(ITreeNode *_parent);

    ~ITreeNode();

    void print(llvm::raw_ostream &stream, const unsigned int tab_num) const;

  };

}
#endif /* ITREE_H_ */
