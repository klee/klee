//===-- ITree.h - Interpolation tree ----------------------------*- C++ -*-===//
//
//               The Tracer-X KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains declarations of the classes that implements the
/// interpolation and subsumption checks for search-space reduction.
///
//===----------------------------------------------------------------------===//

#ifndef ITREE_H_
#define ITREE_H_

#include <klee/Expr.h>
#include "klee/CommandLine.h"
#include "klee/Config/Version.h"
#include "klee/ExecutionState.h"
#include "klee/util/ExprVisitor.h"

#include "Dependency.h"

#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace klee {
class ExecutionState;

class PathCondition;

class SubsumptionTableEntry;

/// Time records for method running time statistics
class StatTimer {
  double amount;
  double lastRecorded;

public:
  StatTimer() : amount(0.0), lastRecorded(0.0) {}

  ~StatTimer() {}

  void start() {
    if (lastRecorded == 0.0)
      lastRecorded = clock();
  }

  double stop() {
    double elapsed = clock() - lastRecorded;
    amount += elapsed;
    lastRecorded = 0.0;
    return elapsed;
  }

  double get() { return (amount / (double)CLOCKS_PER_SEC); }
};


/// Storage of search tree for displaying
class SearchTree {

  /// @brief counter for the next visited node id
  static unsigned long nextNodeId;

  /// @brief Global search tree instance
  static SearchTree *instance;

  /// Encapsulates functionality of expression builder
  class PrettyExpressionBuilder {

    std::string bvOne() {
      return "1";
    };
    std::string bvZero() {
      return "0";
    };
    std::string bvMinusOne() { return "-1"; }
    std::string bvConst32(uint32_t value);
    std::string bvConst64(uint64_t value);
    std::string bvZExtConst(uint64_t value);
    std::string bvSExtConst(uint64_t value);

    std::string bvBoolExtract(std::string expr, int bit);
    std::string bvExtract(std::string expr, unsigned top, unsigned bottom);
    std::string eqExpr(std::string a, std::string b);

    // logical left and right shift (not arithmetic)
    std::string bvLeftShift(std::string expr, unsigned shift);
    std::string bvRightShift(std::string expr, unsigned shift);
    std::string bvVarLeftShift(std::string expr, std::string shift);
    std::string bvVarRightShift(std::string expr, std::string shift);
    std::string bvVarArithRightShift(std::string expr, std::string shift);

    // Some STP-style bitvector arithmetic
    std::string bvMinusExpr(std::string minuend, std::string subtrahend);
    std::string bvPlusExpr(std::string augend, std::string addend);
    std::string bvMultExpr(std::string multiplacand, std::string multiplier);
    std::string bvDivExpr(std::string dividend, std::string divisor);
    std::string sbvDivExpr(std::string dividend, std::string divisor);
    std::string bvModExpr(std::string dividend, std::string divisor);
    std::string sbvModExpr(std::string dividend, std::string divisor);
    std::string notExpr(std::string expr);
    std::string bvAndExpr(std::string lhs, std::string rhs);
    std::string bvOrExpr(std::string lhs, std::string rhs);
    std::string iffExpr(std::string lhs, std::string rhs);
    std::string bvXorExpr(std::string lhs, std::string rhs);
    std::string bvSignExtend(std::string src);

    // Some STP-style array domain interface
    std::string writeExpr(std::string array, std::string index,
                          std::string value);
    std::string readExpr(std::string array, std::string index);

    // ITE-expression constructor
    std::string iteExpr(std::string condition, std::string whenTrue,
                        std::string whenFalse);

    // Bitvector comparison
    std::string bvLtExpr(std::string lhs, std::string rhs);
    std::string bvLeExpr(std::string lhs, std::string rhs);
    std::string sbvLtExpr(std::string lhs, std::string rhs);
    std::string sbvLeExpr(std::string lhs, std::string rhs);

    std::string existsExpr(std::string body);

    std::string constructAShrByConstant(std::string expr, unsigned shift,
                                        std::string isSigned);
    std::string constructMulByConstant(std::string expr, uint64_t x);
    std::string constructUDivByConstant(std::string expr_n, uint64_t d);
    std::string constructSDivByConstant(std::string expr_n, uint64_t d);

    std::string getInitialArray(const Array *root);
    std::string getArrayForUpdate(const Array *root, const UpdateNode *un);

    std::string constructActual(ref<Expr> e);

    std::string buildArray(const char *name, unsigned indexWidth,
                           unsigned valueWidth);

    std::string getTrue();
    std::string getFalse();
    std::string getInitialRead(const Array *root, unsigned index);

    PrettyExpressionBuilder();

    ~PrettyExpressionBuilder();

  public:
    static std::string construct(ref<Expr> e);
  };

  /// Node information
  class Node {
    friend class SearchTree;

    /// @brief Interpolation tree node id
    uintptr_t iTreeNodeId;

    /// @brief The node id, also the order in which it is traversed
    unsigned long nodeId;

    /// @brief False and true children of this node
    SearchTree::Node *falseTarget, *trueTarget;

    /// @brief Indicates that node is subsumed
    bool subsumed;

    /// @brief Conditions under which this node is visited from its parent
    std::map<PathCondition *, std::pair<std::string, bool> > pathConditionTable;

    /// @brief Human-readable identifier of this node
    std::string name;

    Node(uintptr_t nodeId)
        : iTreeNodeId(nodeId), nodeId(0), falseTarget(0), trueTarget(0),
          subsumed(false) {}

    ~Node() {
      if (falseTarget)
        delete falseTarget;

      if (trueTarget)
        delete trueTarget;

      pathConditionTable.clear();
    }

    static SearchTree::Node *createNode(uintptr_t id) {
      return new SearchTree::Node(id);
    }
  };

  SearchTree::Node *root;
  std::map<ITreeNode *, SearchTree::Node *> itreeNodeMap;
  std::map<SubsumptionTableEntry *, SearchTree::Node *> tableEntryMap;
  std::map<SearchTree::Node *, SearchTree::Node *> subsumptionEdges;
  std::map<PathCondition *, SearchTree::Node *> pathConditionMap;

  static std::string recurseRender(const SearchTree::Node *node);

  std::string render();

  SearchTree(ITreeNode *_root);

  ~SearchTree();

public:
  static void initialize(ITreeNode *root) {
    if (!OUTPUT_INTERPOLATION_TREE)
      return;

    if (!instance)
      delete instance;
    instance = new SearchTree(root);
  }

  static void deallocate() {
    if (!OUTPUT_INTERPOLATION_TREE)
      return;

    if (!instance)
      delete instance;
    instance = 0;
  }

  static void addChildren(ITreeNode *parent, ITreeNode *falseChild,
                          ITreeNode *trueChild);

  static void setCurrentNode(ExecutionState &state,
                             const uintptr_t programPoint);

  static void markAsSubsumed(ITreeNode *iTreeNode,
                             SubsumptionTableEntry *entry);

  static void addPathCondition(ITreeNode *iTreeNode,
                               PathCondition *pathCondition,
                               ref<Expr> condition);

  static void addTableEntryMapping(ITreeNode *iTreeNode,
                                   SubsumptionTableEntry *entry);

  static void setAsCore(PathCondition *pathCondition);

  /// @brief Save the graph
  static void save(std::string dotFileName);
};

/**/

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
  bool core;

  /// @brief Previous path condition
  PathCondition *tail;

public:
  PathCondition(ref<Expr> &constraint, Dependency *dependency,
                llvm::Value *condition, PathCondition *prev);

  ~PathCondition();

  ref<Expr> car() const;

  PathCondition *cdr() const;

  void setAsCore(AllocationGraph *g);

  bool isCore() const;

  ref<Expr> packInterpolant(std::vector<const Array *> &replacements);

  void dump();

  void print(llvm::raw_ostream &stream);
};

class PathConditionMarker {
  bool maybeCore;

  PathCondition *pathCondition;

public:
  PathConditionMarker(PathCondition *pathCondition);

  ~PathConditionMarker();

  void setAsCore(AllocationGraph *g);

  void setAsMaybeCore();
};

class SubsumptionTableEntry {
  friend class ITree;

  class ApplySubstitutionVisitor : public ExprVisitor {
  private:
    const std::map<ref<Expr>, ref<Expr> > &replacements;

  public:
    ApplySubstitutionVisitor(
        const std::map<ref<Expr>, ref<Expr> > &_replacements)
        : ExprVisitor(true), replacements(_replacements) {}

    Action visitExprPost(const Expr &e) {
      std::map<ref<Expr>, ref<Expr> >::const_iterator it =
          replacements.find(ref<Expr>(const_cast<Expr *>(&e)));
      if (it != replacements.end()) {
        return Action::changeTo(it->second);
      } else {
        return Action::doChildren();
      }
    }
  };

  /// @brief Statistics for actual solver call time in subsumption check
  static StatTimer actualSolverCallTimer;

  /// @brief The number of solver calls for subsumption checks
  static unsigned long checkSolverCount;

  /// @brief The number of failed solver calls for subsumption checks
  static unsigned long checkSolverFailureCount;

  ref<Expr> interpolant;

  Dependency::ConcreteStore concreteAddressStore;

  std::vector<llvm::Value *> concreteAddressStoreKeys;

  Dependency::SymbolicStore symbolicAddressStore;

  std::vector<llvm::Value *> symbolicAddressStoreKeys;

  std::vector<const Array *> existentials;

  static bool hasExistentials(std::vector<const Array *> &existentials,
                              ref<Expr> expr);

  static bool hasFree(std::vector<const Array *> &existentials, ref<Expr> expr);

  static bool containShadowExpr(ref<Expr> expr, ref<Expr> shadowExpr);

  static ref<Expr> replaceExpr(ref<Expr> originalExpr, ref<Expr> replacedExpr,
                               ref<Expr> withExpr);

  /// @brief Simplifies the interpolant condition in subsumption check
  /// whenever it contains constant equalities or disequalities.
  static ref<Expr>
  simplifyInterpolantExpr(std::vector<ref<Expr> > &interpolantPack,
                          ref<Expr> expr);

  /// @brief Simplifies the equality conditions in subsumption check
  /// whenever it contains constant equalities.
  static ref<Expr> simplifyEqualityExpr(std::vector<ref<Expr> > &equalityPack,
                                        ref<Expr> expr);

  static ref<Expr> simplifyWithFourierMotzkin(ref<Expr> existsExpr);

  static ref<Expr> simplifyExistsExpr(ref<Expr> existsExpr,
                                      bool &hasExistentialsOnly);

  static ref<Expr> simplifyArithmeticBody(ref<Expr> existsExpr,
                                          bool &hasExistentialsOnly);

  static ref<Expr> getSubstitution(ref<Expr> equalities,
                                   std::map<ref<Expr>, ref<Expr> > &map);

  bool empty() {
    return !interpolant.get() && concreteAddressStoreKeys.empty();
  }

  /// @brief for printing method running time statistics
  static void printStat(llvm::raw_ostream &stream);

public:
  const uintptr_t nodeId;

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

  static StatTimer setCurrentINodeTimer;
  static StatTimer removeTimer;
  static StatTimer checkCurrentStateSubsumptionTimer;
  static StatTimer markPathConditionTimer;
  static StatTimer splitTimer;
  static StatTimer executeOnNodeTimer;

  ITreeNode *currentINode;

  std::map<uintptr_t, std::vector<SubsumptionTableEntry *> > subsumptionTable;

  void printNode(llvm::raw_ostream &stream, ITreeNode *n, std::string edges);

  /// @brief Displays method running time statistics
  static void printTimeStat(llvm::raw_ostream &stream);

  /// @brief Displays subsumption table statistics
  void printTableStat(llvm::raw_ostream &stream);

public:
  ITreeNode *root;

  ITree(ExecutionState *_root);

  ~ITree();

  void store(SubsumptionTableEntry *subItem);

  void setCurrentINode(ExecutionState &state, uintptr_t programPoint);

  void remove(ITreeNode *node);

  bool checkCurrentStateSubsumption(TimingSolver *solver, ExecutionState &state,
                                    double timeout);

  void markPathCondition(ExecutionState &state, TimingSolver *solver);

  std::pair<ITreeNode *, ITreeNode *>
  split(ITreeNode *parent, ExecutionState *left, ExecutionState *right);

  void execute(llvm::Instruction *instr);

  void execute(llvm::Instruction *instr, ref<Expr> arg1);

  void execute(llvm::Instruction *instr, ref<Expr> arg1, ref<Expr> arg2);

  void execute(llvm::Instruction *instr, ref<Expr> arg1, ref<Expr> arg2,
               ref<Expr> arg3);

  void execute(llvm::Instruction *instr, std::vector<ref<Expr> > &args);

  static void executeOnNode(ITreeNode *node, llvm::Instruction *instr,
                            std::vector<ref<Expr> > &args);

  void print(llvm::raw_ostream &stream);

  void dump();

  /// @brief Outputs interpolation statistics to standard error.
  void dumpInterpolationStat();
};

class ITreeNode {
  friend class ITree;

  friend class ExecutionState;

  static StatTimer getInterpolantTimer;
  static StatTimer addConstraintTimer;
  static StatTimer splitTimer;
  static StatTimer makeMarkerMapTimer;
  static StatTimer deleteMarkerMapTimer;
  static StatTimer executeTimer;
  static StatTimer bindCallArgumentsTimer;
  static StatTimer popAbstractDependencyFrameTimer;
  static StatTimer getConcreteAddressExpressionsTimer;
  static StatTimer getConcreteAddressCoreExpressionsTimer;
  static StatTimer computeCoreAllocationsTimer;

private:
  typedef ref<Expr> expression_type;

  typedef std::pair<expression_type, expression_type> pair_type;

  /// @brief The path condition
  PathCondition *pathCondition;

  /// @brief Abstract stack for value dependencies
  Dependency *dependency;

  ITreeNode *parent, *left, *right;

  uintptr_t nodeId;

  bool isSubsumed;

  /// @brief Graph for displaying as .dot file
  SearchTree *graph;

  void setNodeLocation(uintptr_t programPoint) {
    if (!nodeId)
      nodeId = programPoint;
  }

  /// @brief for printing method running time statistics
  static void printTimeStat(llvm::raw_ostream &stream);

  void execute(llvm::Instruction *instr, std::vector<ref<Expr> > &args);

public:
  uintptr_t getNodeId();

  ref<Expr> getInterpolant(std::vector<const Array *> &replacements) const;

  void addConstraint(ref<Expr> &constraint, llvm::Value *value);

  void split(ExecutionState *leftData, ExecutionState *rightData);

  std::map<Expr *, PathConditionMarker *> makeMarkerMap() const;

  static void
  deleteMarkerMap(std::map<Expr *, PathConditionMarker *> &markerMap);

  void bindCallArguments(llvm::Instruction *site,
                         std::vector<ref<Expr> > &arguments);

  void popAbstractDependencyFrame(llvm::CallInst *site, llvm::Instruction *inst,
                                  ref<Expr> returnValue);

  std::pair<Dependency::ConcreteStore, Dependency::SymbolicStore>
  getStoredExpressions() const;

  std::pair<Dependency::ConcreteStore, Dependency::SymbolicStore>
  getStoredCoreExpressions(std::vector<const Array *> &replacements) const;

  void computeCoreAllocations(AllocationGraph *g);

  void dump() const;

  void print(llvm::raw_ostream &stream) const;

private:
  ITreeNode(ITreeNode *_parent);

  ~ITreeNode();

  void print(llvm::raw_ostream &stream, const unsigned tabNum) const;
};
}
#endif /* ITREE_H_ */
