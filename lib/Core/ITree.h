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

/// Global variable denoting whether interpolation is enabled or otherwise
struct InterpolationOption {
  static bool interpolation;
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

  PathCondition *cdr() const;

  void includeInInterpolant(AllocationGraph *g);

  bool carInInterpolant() const;

  ref<Expr> packInterpolant(std::vector<const Array *> &replacements);

  void dump();

  void print(llvm::raw_ostream &stream);
};

class PathConditionMarker {
  bool mayBeInInterpolant;

  PathCondition *pathCondition;

public:
  PathConditionMarker(PathCondition *pathCondition);

  ~PathConditionMarker();

  void includeInInterpolant(AllocationGraph *g);

  void mayIncludeInInterpolant();
};

class SubsumptionTableEntry {
  uintptr_t nodeId;

  ref<Expr> interpolant;

  std::map<llvm::Value *, ref<Expr> > singletonStore;

  std::vector<llvm::Value *> singletonStoreKeys;

  std::map<llvm::Value *, std::vector<ref<Expr> > > compositeStore;

  std::vector<llvm::Value *> compositeStoreKeys;

  std::vector<const Array *> existentials;

  static bool hasExistentials(std::vector<const Array *> &existentials,
                              ref<Expr> expr);

  static ref<Expr> createBinaryOfSameKind(ref<Expr> originalExpr,
                                          ref<Expr> newLhs, ref<Expr> newRhs);

  static bool containShadowExpr(ref<Expr> expr, ref<Expr> shadowExpr);

  static ref<Expr> replaceExpr(ref<Expr> originalExpr, ref<Expr> replacedExpr,
                               ref<Expr> withExpr);

  static ref<Expr>
  simplifyInterpolantExpr(std::vector<ref<Expr> > &interpolantPack,
                          ref<Expr> expr);

  static ref<Expr> simplifyEqualityExpr(std::vector<ref<Expr> > &equalityPack,
                                        ref<Expr> expr);

  static ref<Expr> simplifyExistsExpr(ref<Expr> existsExpr);

  static ref<Expr> simplifyArithmeticBody(ref<Expr> existsExpr);

  bool empty() {
    return !interpolant.get() && singletonStoreKeys.empty() &&
           compositeStoreKeys.empty();
  }

public:
  SubsumptionTableEntry(ITreeNode *node);

  ~SubsumptionTableEntry();

  bool subsumed(TimingSolver *solver, ExecutionState &state, double timeout);

  void dump() const;

  void print(llvm::raw_ostream &stream) const;
};

class ITree {
  typedef std::vector<ref<Expr> > ExprList;
  typedef ExprList::iterator iterator;
  typedef ExprList::const_iterator const_iterator;

  ITreeNode *currentINode;

  std::vector<SubsumptionTableEntry> subsumptionTable;

  void printNode(llvm::raw_ostream &stream, ITreeNode *n, std::string edges);

public:
  ITreeNode *root;

  ITree(ExecutionState *_root);

  ~ITree();

  std::vector<SubsumptionTableEntry> getStore();

  void store(SubsumptionTableEntry subItem);

  void setCurrentINode(ITreeNode *node);

  void remove(ITreeNode *node);

  bool checkCurrentStateSubsumption(TimingSolver *solver, ExecutionState &state,
                                    double timeout);

  void markPathCondition(ExecutionState &state, TimingSolver *solver);

  std::pair<ITreeNode *, ITreeNode *>
  split(ITreeNode *parent, ExecutionState *left, ExecutionState *right);

  void executeAbstractBinaryDependency(llvm::Instruction *i,
                                       ref<Expr> valueExpr, ref<Expr> tExpr,
                                       ref<Expr> fExpr);

  void executeAbstractMemoryDependency(llvm::Instruction *instr,
                                       ref<Expr> value, ref<Expr> address);

  void executeAbstractDependency(llvm::Instruction *instr, ref<Expr> value);

  void print(llvm::raw_ostream &stream);

  void dump();
};

class ITreeNode {
  friend class ITree;

  friend class ExecutionState;

  typedef ref<Expr> expression_type;

  typedef std::pair<expression_type, expression_type> pair_type;

  /// @brief The path condition
  PathCondition *pathCondition;

  /// @brief Abstract stack for value dependencies
  Dependency *dependency;

  ITreeNode *parent, *left, *right;

  uintptr_t nodeId;

  bool isSubsumed;

public:
  uintptr_t getNodeId();

  ref<Expr> getInterpolant(std::vector<const Array *> &replacements) const;

  void setNodeLocation(uintptr_t programPoint);

  void addConstraint(ref<Expr> &constraint, llvm::Value *value);

  void split(ExecutionState *leftData, ExecutionState *rightData);

  void dump() const;

  void print(llvm::raw_ostream &stream) const;

  std::map<ref<Expr>, PathConditionMarker *> makeMarkerMap() const;

  static void
  deleteMarkerMap(std::map<ref<Expr>, PathConditionMarker *> &markerMap);

  void executeBinaryDependency(llvm::Instruction *i, ref<Expr> valueExpr,
                               ref<Expr> tExpr, ref<Expr> fExpr);

  void executeAbstractMemoryDependency(llvm::Instruction *instr,
                                       ref<Expr> value, ref<Expr> address);

  void executeAbstractDependency(llvm::Instruction *instr, ref<Expr> value);

  void bindCallArguments(llvm::Instruction *site,
                         std::vector<ref<Expr> > &arguments);

  void popAbstractDependencyFrame(llvm::CallInst *site, llvm::Instruction *inst,
                                  ref<Expr> returnValue);

  std::map<llvm::Value *, ref<Expr> > getLatestCoreExpressions() const;

  std::map<llvm::Value *, std::vector<ref<Expr> > >
  getCompositeCoreExpressions() const;

  std::map<llvm::Value *, ref<Expr> > getLatestInterpolantCoreExpressions(
      std::vector<const Array *> &replacements) const;

  std::map<llvm::Value *, std::vector<ref<Expr> > >
  getCompositeInterpolantCoreExpressions(
      std::vector<const Array *> &replacements) const;

  void computeInterpolantAllocations(AllocationGraph *g);

private:
  ITreeNode(ITreeNode *_parent);

  ~ITreeNode();

  void print(llvm::raw_ostream &stream, const unsigned tabNum) const;
};
}
#endif /* ITREE_H_ */
