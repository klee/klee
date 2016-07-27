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

/// \brief Time records for method running time statistics
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

  /// \brief Utility function to represent double-precision floating point in
  /// two decimal points.
  static std::string inTwoDecimalPoints(double n) {
    std::ostringstream stream;
    unsigned long x = (unsigned)((n - ((unsigned)n)) * 100);
    unsigned y = (unsigned)n;
    stream << y << ".";
    if (x > 9)
      stream << x;
    else if (x > 0)
      stream << "0" << x;
    else
      stream << "00";
    return stream.str();
  }
};

/// \brief The implementation of the search tree for outputting to .dot file.
class SearchTree {

  /// \brief counter for the next visited node id
  static unsigned long nextNodeId;

  /// \brief Global search tree instance
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

    /// \brief Interpolation tree node id
    uintptr_t iTreeNodeId;

    /// \brief The node id, also the order in which it is traversed
    unsigned long nodeId;

    /// \brief False and true children of this node
    SearchTree::Node *falseTarget, *trueTarget;

    /// \brief Indicates that node is subsumed
    bool subsumed;

    /// \brief Conditions under which this node is visited from its parent
    std::map<PathCondition *, std::pair<std::string, bool> > pathConditionTable;

    /// \brief Human-readable identifier of this node
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

  class NumberedEdge {
    SearchTree::Node *source;
    SearchTree::Node *destination;
    unsigned long number;

  public:
    NumberedEdge(SearchTree::Node *_source, SearchTree::Node *_destination,
                 unsigned long _number)
        : source(_source), destination(_destination), number(_number) {}

    ~NumberedEdge() {
      delete source;
      delete destination;
    }

    std::string render() const;
  };

  SearchTree::Node *root;
  std::map<ITreeNode *, SearchTree::Node *> itreeNodeMap;
  std::map<SubsumptionTableEntry *, SearchTree::Node *> tableEntryMap;
  std::vector<SearchTree::NumberedEdge *> subsumptionEdges;
  std::map<PathCondition *, SearchTree::Node *> pathConditionMap;

  unsigned long subsumptionEdgeNumber;

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

  /// \brief Save the graph
  static void save(std::string dotFileName);
};

/**/

/// \brief A node in a singly-linked list of conditions which constitute the
/// path condition.
class PathCondition {
  /// \brief KLEE expression
  ref<Expr> constraint;

  /// \brief KLEE expression with variables (arrays) replaced by their shadows
  ref<Expr> shadowConstraint;

  /// \brief If shadow constraint had been generated: We generate shadow
  /// constraint on demand only when the constraint is required in an
  /// interpolant.
  bool shadowed;

  /// \brief The set of bound variables
  std::set<const Array *> boundVariables;

  /// \brief The dependency information for the current interpolation tree node
  Dependency *dependency;

  /// \brief the condition value from which the constraint was generated
  VersionedValue *condition;

  /// \brief When true, indicates that the constraint should be included in the
  /// interpolant
  bool core;

  /// \brief Previous path condition
  PathCondition *tail;

public:
  PathCondition(ref<Expr> &constraint, Dependency *dependency,
                llvm::Value *condition, PathCondition *prev);

  ~PathCondition();

  ref<Expr> car() const;

  PathCondition *cdr() const;

  void setAsCore(AllocationGraph *g);

  bool isCore() const;

  ref<Expr> packInterpolant(std::set<const Array *> &replacements);

  void dump() const;

  void print(llvm::raw_ostream &stream) const;
};

/// \brief The class that implements an entry (record) in the subsumption table.
class SubsumptionTableEntry {
  friend class ITree;

  /// \brief General substitution mechanism
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

  /// \brief Statistics for actual solver call time in subsumption check
  static StatTimer actualSolverCallTimer;

  /// \brief The number of solver calls for subsumption checks
  static unsigned long checkSolverCount;

  /// \brief The number of failed solver calls for subsumption checks
  static unsigned long checkSolverFailureCount;

  ref<Expr> interpolant;

  Dependency::ConcreteStore concreteAddressStore;

  std::vector<llvm::Value *> concreteAddressStoreKeys;

  Dependency::SymbolicStore symbolicAddressStore;

  std::vector<llvm::Value *> symbolicAddressStoreKeys;

  std::set<const Array *> existentials;

  /// \brief Test for the existence of a variable in a set in an expression.
  ///
  /// \param A set of variables (KLEE arrays).
  /// \param The expression to test for the existence of the variables in the
  /// set.
  /// \return true if a variable in the set is found in the expression, false
  /// otherwise.
  static bool hasVariableInSet(std::set<const Array *> &existentials,
                               ref<Expr> expr);

  /// \brief Test for the non-existence of a variable in a set in an expression.
  ///
  /// \param A set of variables (KLEE arrays).
  /// \param The expression to test for the non-existence of the variables in
  /// the set.
  /// \return true if none of the variable in the set is found in the
  /// expression, false otherwise.
  static bool hasVariableNotInSet(std::set<const Array *> &existentials,
                                  ref<Expr> expr);

  /// \brief Determines if a subexpression is in an expression
  static bool hasSubExpression(ref<Expr> expr, ref<Expr> subExpr);

  /// \brief Replace a sub-expression with another within an original expression
  static ref<Expr> replaceExpr(ref<Expr> originalExpr, ref<Expr> replacedExpr,
                               ref<Expr> replacementExpr);

  /// \brief Simplifies the interpolant condition in subsumption check whenever
  /// it contains constant equalities or disequalities.
  static ref<Expr>
  simplifyInterpolantExpr(std::vector<ref<Expr> > &interpolantPack,
                          ref<Expr> expr);

  /// \brief Simplifies the equality conditions in subsumption check whenever it
  /// contains constant equalities.
  static ref<Expr> simplifyEqualityExpr(std::vector<ref<Expr> > &equalityPack,
                                        ref<Expr> expr);

  /// \brief Simplify if possible an existentially-quantified expression,
  /// possibly removing the quantification.
  static ref<Expr> simplifyExistsExpr(ref<Expr> existsExpr,
                                      bool &hasExistentialsOnly);

  /// \brief Detect contradictory equalities in subsumption check beforehand to
  /// reduce the expensive call to the actual solver.
  ///
  /// \return true if there is contradictory equality constraints between state
  /// constraints and query expression, otherwise, return false.
  static bool detectConflictPrimitives(ExecutionState &state, ref<Expr> query);

  /// \brief Get a conjunction of equalities that are top-level conjuncts in the
  /// query.
  ///
  /// \param The output conjunction of top-level conjuncts in the query
  /// expression.
  /// \param The query expression.
  /// \return false if there is an equality conjunct that is simplifiable to
  /// false, true otherwise.
  static bool fetchQueryEqualityConjuncts(std::vector<ref<Expr> > &conjunction,
                                          ref<Expr> query);

  static ref<Expr> simplifyArithmeticBody(ref<Expr> existsExpr,
                                          bool &hasExistentialsOnly);

  static ref<Expr> getSubstitution(ref<Expr> equalities,
                                   std::map<ref<Expr>, ref<Expr> > &map);

  bool empty() {
    return !interpolant.get() && concreteAddressStoreKeys.empty();
  }

  /// \brief For printing method running time statistics
  static void printStat(llvm::raw_ostream &stream);

public:
  const uintptr_t nodeId;

  SubsumptionTableEntry(ITreeNode *node);

  ~SubsumptionTableEntry();

  bool subsumed(
      TimingSolver *solver, ExecutionState &state, double timeout,
      std::pair<Dependency::ConcreteStore, Dependency::SymbolicStore> const);

  /// Tests if the argument is a variable. A variable here is defined to be
  /// either a symbolic concatenation or a symbolic read. A concatenation in
  /// KLEE concatenates reads, and hence can be considered to be a symbolic
  /// read.
  ///
  /// \param A KLEE expression.
  /// \return true if the parameter is either a concatenation or a read,
  ///         otherwise, return false.
  static bool isVariable(ref<Expr> expr) {
    return llvm::isa<ConcatExpr>(expr.get()) || llvm::isa<ReadExpr>(expr.get());
  }

  ref<Expr> getInterpolant() const;

  void dump() const {
    this->print(llvm::errs());
    llvm::errs() << "\n";
  }

  void print(llvm::raw_ostream &stream) const;

};

/// \brief The interpolation tree node: The implementation of the tree node to
/// be embedded in KLEE's execution state, which stores the data of
/// interpolation functionalities.
class ITreeNode {
  friend class ITree;

  friend class ExecutionState;

  // Timers for profiling the execution times of the methods of this class.
  static StatTimer getInterpolantTimer;
  static StatTimer addConstraintTimer;
  static StatTimer splitTimer;
  static StatTimer executeTimer;
  static StatTimer bindCallArgumentsTimer;
  static StatTimer bindReturnValueTimer;
  static StatTimer getStoredExpressionsTimer;
  static StatTimer getStoredCoreExpressionsTimer;
  static StatTimer computeCoreAllocationsTimer;

private:
  typedef ref<Expr> expression_type;

  typedef std::pair<expression_type, expression_type> pair_type;

  /// \brief The path condition
  PathCondition *pathCondition;

  /// \brief Abstract stack for value dependencies
  Dependency *dependency;

  ITreeNode *parent, *left, *right;

  uintptr_t nodeId;

  bool isSubsumed;

  /// \brief Graph for displaying as .dot file
  SearchTree *graph;

  void setNodeLocation(uintptr_t programPoint) {
    if (!nodeId)
      nodeId = programPoint;
  }

  /// \brief for printing method running time statistics
  static void printTimeStat(llvm::raw_ostream &stream);

  void execute(llvm::Instruction *instr, std::vector<ref<Expr> > &args);

public:
  uintptr_t getNodeId();

  /// \brief Retrieve the interpolant for this node as KLEE expression object
  ///
  /// \param The replacement bound variables for replacing the variables in the
  /// path condition.
  /// \return The interpolant expression.
  ref<Expr> getInterpolant(std::set<const Array *> &replacements) const;

  /// \brief Extend the path condition with another constraint
  ///
  /// \param The constraint to extend the current path condition with
  /// \param The LLVM value that corresponds to the constraint
  void addConstraint(ref<Expr> &constraint, llvm::Value *value);

  /// \brief Creates fresh interpolation data holder for the two given KLEE
  /// execution states.
  /// This method is to be invoked after KLEE splits its own state due to state
  /// forking.
  ///
  /// \param The first KLEE execution state
  /// \param The second KLEE execution state
  void split(ExecutionState *leftData, ExecutionState *rightData);

  /// \brief Record call arguments in a function call
  void bindCallArguments(llvm::Instruction *site,
                         std::vector<ref<Expr> > &arguments);

  /// \brief This propagates the dependency due to the return value of a call
  void bindReturnValue(llvm::CallInst *site, llvm::Instruction *inst,
                       ref<Expr> returnValue);

  /// \brief This retrieves the allocations known at this state, and the
  /// expressions stored in the allocations.
  ///
  /// \return A pair of the store part indexed by constants, and the store part
  /// indexed by symbolic expressions.
  std::pair<Dependency::ConcreteStore, Dependency::SymbolicStore>
  getStoredExpressions() const;

  /// \brief This retrieves the allocations known at this state, and the
  /// expressions stored in the allocations, as long as the allocation is
  /// relevant as an interpolant. This function is typically used when creating
  /// an entry in the subsumption table.
  ///
  /// \param The replacement bound variables: As the resulting expression will
  /// be used for storing in the subsumption table, the variables need to be
  /// replaced with the bound ones.
  /// \return A pair of the store part indexed by constants, and the store part
  /// indexed by symbolic expressions.
  std::pair<Dependency::ConcreteStore, Dependency::SymbolicStore>
  getStoredCoreExpressions(std::set<const Array *> &replacements) const;

  /// \brief Marking the core constraints on the path condition, and all the
  /// relevant values on the dependency graph, given an unsatistiability core.
  void unsatCoreMarking(std::vector<ref<Expr> > unsatCore);

  /// \brief Compute the allocations that are relevant for the interpolant.
  void computeCoreAllocations(AllocationGraph *g);

  /// \brief Print the content of the tree node object to the LLVM error stream.
  void dump() const;

  /// \brief Print the content of the tree node object into a stream.
  ///
  /// \param The stream to print the data to.
  void print(llvm::raw_ostream &stream) const;

private:
  ITreeNode(ITreeNode *_parent);

  ~ITreeNode();

  void print(llvm::raw_ostream &stream, const unsigned tabNum) const;
};

/// \brief The top-level structure that implements the interpolation
/// functionality
class ITree {
  typedef std::vector<ref<Expr> > ExprList;
  typedef ExprList::iterator iterator;
  typedef ExprList::const_iterator const_iterator;

  // Several static fields for profiling the execution time of this class's
  // methods.
  static StatTimer setCurrentINodeTimer;
  static StatTimer removeTimer;
  static StatTimer subsumptionCheckTimer;
  static StatTimer markPathConditionTimer;
  static StatTimer splitTimer;
  static StatTimer executeOnNodeTimer;

  /// \brief Number of subsumption checks for statistical purposes
  static unsigned long subsumptionCheckCount;

  ITreeNode *currentINode;

  std::map<uintptr_t, std::vector<SubsumptionTableEntry *> > subsumptionTable;

  void printNode(llvm::raw_ostream &stream, ITreeNode *n,
                 std::string edges) const;

  /// \brief Displays method running time statistics
  static void printTimeStat(llvm::raw_ostream &stream);

  /// \brief Displays subsumption table statistics
  void printTableStat(llvm::raw_ostream &stream) const;

public:
  ITreeNode *root;

  ITree(ExecutionState *_root);

  ~ITree();

  /// \brief Store an entry into the subsumption table.
  void store(SubsumptionTableEntry *subItem);

  /// \brief Set the reference to the KLEE state in the current interpolation
  /// data holder (interpolation tree node) that is currently being processed.
  /// This also sets the id of the interpolation tree node to be the given
  /// pointer value.
  ///
  /// \param The KLEE execution state to associate the current node with.
  /// \param The id to be set for the current node.
  void setCurrentINode(ExecutionState &state, uintptr_t programPoint);

  /// \brief Deletes the interpolation tree node
  void remove(ITreeNode *node);

  /// \brief Invokes the subsumption check
  bool subsumptionCheck(TimingSolver *solver, ExecutionState &state,
                        double timeout);

  /// \brief Mark the path condition in the interpolation tree node associated
  /// with the given KLEE execution state.
  void markPathCondition(ExecutionState &state, TimingSolver *solver);

  /// \brief Creates fresh interpolation data holder for the two given KLEE
  /// execution states.
  /// This method is to be invoked after KLEE splits its own state due to state
  /// forking.
  std::pair<ITreeNode *, ITreeNode *>
  split(ITreeNode *parent, ExecutionState *left, ExecutionState *right);

  /// \brief Abstractly execute an instruction of no argument for building
  /// dependency information.
  void execute(llvm::Instruction *instr);

  /// \brief Abstractly execute an instruction of one argument for building
  /// dependency information.
  void execute(llvm::Instruction *instr, ref<Expr> arg1);

  /// \brief Abstractly execute an instruction of two arguments for building
  /// dependency information.
  void execute(llvm::Instruction *instr, ref<Expr> arg1, ref<Expr> arg2);

  /// \brief Abstractly execute an instruction of three arguments for building
  /// dependency information.
  void execute(llvm::Instruction *instr, ref<Expr> arg1, ref<Expr> arg2,
               ref<Expr> arg3);

  /// \brief Abstractly execute an instruction of a number of arguments for
  /// building dependency information.
  void execute(llvm::Instruction *instr, std::vector<ref<Expr> > &args);

  /// \brief Abstractly execute a PHI instruction for building dependency
  /// information.
  void executePHI(llvm::Instruction *instr, unsigned int incomingBlock,
                  ref<Expr> valueExpr);

  /// \brief General method for executing an instruction for building dependency
  /// information, given a particular interpolation tree node.
  static void executeOnNode(ITreeNode *node, llvm::Instruction *instr,
                            std::vector<ref<Expr> > &args);

  /// \brief Print the content of the tree node object into a stream.
  ///
  /// \param The stream to print the data to.
  void print(llvm::raw_ostream &stream) const;

  /// \brief Print the content of the tree object to the LLVM error stream
  void dump() const;

  /// \brief Outputs interpolation statistics to LLVM error stream.
  void dumpInterpolationStat() const;
};
}
#endif /* ITREE_H_ */
