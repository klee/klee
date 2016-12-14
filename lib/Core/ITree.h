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
#include "klee/Solver.h"
#include "klee/Statistic.h"
#include "klee/TimerStatIncrementer.h"
#include "klee/util/ExprVisitor.h"

#include "Dependency.h"

#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace klee {
class ExecutionState;

class PathCondition;

class SubsumptionTableEntry;

/// \brief The implementation of the search tree for outputting to .dot file.
class SearchTree {

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

    /// \brief The node id, also the order in which it is traversed
    uint64_t nodeSequenceNumber;

    /// \brief False and true children of this node
    SearchTree::Node *falseTarget, *trueTarget;

    /// \brief Indicates that node is subsumed
    bool subsumed;

    /// \brief Conditions under which this node is visited from its parent
    std::map<PathCondition *, std::pair<std::string, bool> > pathConditionTable;

    /// \brief Human-readable identifier of this node
    std::string name;

    Node()
        : nodeSequenceNumber(0), falseTarget(0), trueTarget(0),
          subsumed(false) {}

    ~Node() {
      if (falseTarget)
        delete falseTarget;

      if (trueTarget)
        delete trueTarget;

      pathConditionTable.clear();
    }

    static SearchTree::Node *createNode() { return new SearchTree::Node(); }
  };

  class NumberedEdge {
    SearchTree::Node *source;
    SearchTree::Node *destination;
    uint64_t number;

  public:
    NumberedEdge(SearchTree::Node *_source, SearchTree::Node *_destination,
                 uint64_t _number)
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

  uint64_t subsumptionEdgeNumber;

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
                             const uint64_t _nodeSequenceNumber);

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
  ref<VersionedValue> condition;

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

  void setAsCore();

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

#ifdef ENABLE_Z3
  /// \brief Mark begin and end of subsumption check for use within a scope
  struct SubsumptionCheckMarker {
    SubsumptionCheckMarker() { Z3Solver::subsumptionCheck = true; }
    ~SubsumptionCheckMarker() { Z3Solver::subsumptionCheck = false; }
  };
#endif

  static Statistic concreteStoreExpressionBuildTime;
  static Statistic symbolicStoreExpressionBuildTime;
  static Statistic solverAccessTime;

  ref<Expr> interpolant;

  Dependency::ConcreteStore concreteAddressStore;

  std::vector<llvm::Value *> concreteAddressStoreKeys;

  Dependency::SymbolicStore symbolicAddressStore;

  std::vector<llvm::Value *> symbolicAddressStoreKeys;

  std::set<const Array *> existentials;

  /// \brief Test for the existence of a variable in a set in an expression.
  ///
  /// \param existentials A set of variables (KLEE arrays).
  /// \param expr The expression to test for the existence of the variables in
  /// the set.
  /// \return true if a variable in the set is found in the expression, false
  /// otherwise.
  static bool hasVariableInSet(std::set<const Array *> &existentials,
                               ref<Expr> expr);

  /// \brief Test for the non-existence of a variable in a set in an expression.
  ///
  /// \param existentials A set of variables (KLEE arrays).
  /// \param expr The expression to test for the non-existence of the variables
  /// in the set.
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
  /// \return false if there is contradictory equality constraints between state
  /// constraints and query expression, otherwise, return true.
  static bool detectConflictPrimitives(ExecutionState &state, ref<Expr> query);

  /// \brief Get a conjunction of equalities that are top-level conjuncts in the
  /// query.
  ///
  /// \param conjunction The output conjunction of top-level conjuncts in the
  /// query expression.
  /// \param query The query expression.
  /// \return false if there is an equality conjunct that is simplifiable to
  /// false, true otherwise.
  static bool fetchQueryEqualityConjuncts(std::vector<ref<Expr> > &conjunction,
                                          ref<Expr> query);

  static ref<Expr> simplifyArithmeticBody(ref<Expr> existsExpr,
                                          bool &hasExistentialsOnly);

  static ref<Expr> getSubstitution(ref<Expr> equalities,
                                   std::map<ref<Expr>, ref<Expr> > &map);

  bool empty() {
    return interpolant.isNull() && concreteAddressStoreKeys.empty() && symbolicAddressStoreKeys.empty();
  }

  /// \brief For printing member functions running time statistics
  static void printStat(std::stringstream &stream);

public:
  const uintptr_t programPoint;

  const uint64_t nodeSequenceNumber;

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
  /// \param expr A KLEE expression.
  /// \return true if the parameter is either a concatenation or a read,
  ///         otherwise, return false.
  static bool isVariable(ref<Expr> expr) {
    return llvm::isa<ConcatExpr>(expr) || llvm::isa<ReadExpr>(expr);
  }

  ref<Expr> getInterpolant() const;

  void dump() const {
    this->print(llvm::errs());
    llvm::errs() << "\n";
  }

  void print(llvm::raw_ostream &stream) const;

};

/// \brief The interpolation tree node.
///
/// This class is a higher-level wrapper to the path condition (referenced
/// by the member variable ITreeNode#pathCondition of type PathCondition), and
/// the shadow
/// memory for memory dependency computation (referenced by the member
/// variable ITreeNode#dependency of type Dependency).
///
/// The interpolation tree node has an associated KLEE execution state
/// (implemented using the type ExecutionState) from which it is referenced
/// via the member variable ExecutionState#itreeNode.
/// It adds information for lazy annotation to the ExecutionState object.
/// The whole structure of the interpolation tree itself is maintained by
/// the class ITree, which also refers to objects of type ITreeNode via
/// its ITree#root and ITree#currentINode member variables.
///
/// \see ITree
/// \see Dependency
/// \see PathCondition
class ITreeNode {
  friend class ITree;

  friend class ExecutionState;

  // Timers for profiling the execution times of the member functions of this
  // class

  static Statistic getInterpolantTime;
  static Statistic addConstraintTime;
  static Statistic splitTime;
  static Statistic executeTime;
  static Statistic bindCallArgumentsTime;
  static Statistic bindReturnValueTime;
  static Statistic getStoredExpressionsTime;
  static Statistic getStoredCoreExpressionsTime;

  /// \brief Counter for the next visited node id for logging and debugging
  /// purposes
  static uint64_t nextNodeSequenceNumber;

private:
  /// \brief The path condition
  PathCondition *pathCondition;

  /// \brief Abstract stack for value dependencies
  Dependency *dependency;

  ITreeNode *parent, *left, *right;

  uintptr_t programPoint;

  uint64_t nodeSequenceNumber;

  bool isSubsumed;

  bool storable;

  /// \brief Graph for displaying as .dot file
  SearchTree *graph;

  /// \brief For statistics on the number of instructions executed along a path.
  uint64_t instructionsDepth;

  /// \brief The data layout of the analysis target
  llvm::DataLayout *targetData;

  void setProgramPoint(llvm::Instruction *instr) {
    if (!programPoint)
      programPoint = reinterpret_cast<uintptr_t>(instr);

    // Disabling the subsumption check within KLEE's own API
    // (callsites of klee_ and at any location within the klee_ function)
    // by never store a table entry for KLEE's own API, marked with flag storable.
    storable = !(instr->getParent()->getParent()->getName().substr(0, 5).equals("klee_"));
  }

  /// \brief for printing member function running time statistics
  static void printTimeStat(std::stringstream &stream);

  void execute(llvm::Instruction *instr, std::vector<ref<Expr> > &args,
               bool symbolicExecutionError);

public:
  uintptr_t getProgramPoint() { return programPoint; }

  uint64_t getNodeSequenceNumber() { return nodeSequenceNumber; }

  /// \brief Retrieve the interpolant for this node as KLEE expression object
  ///
  /// \param replacements The replacement bound variables for replacing the
  /// variables in the path condition.
  /// \return The interpolant expression.
  ref<Expr> getInterpolant(std::set<const Array *> &replacements) const;

  /// \brief Extend the path condition with another constraint
  ///
  /// \param constraint The constraint to extend the current path condition with
  /// \param value The LLVM value that corresponds to the constraint
  void addConstraint(ref<Expr> &constraint, llvm::Value *value);

  /// \brief Creates fresh interpolation data holder for the two given KLEE
  /// execution states.
  /// This member function is to be invoked after KLEE splits its own state due
  /// to state
  /// forking.
  ///
  /// \param leftData The first KLEE execution state
  /// \param rightData The second KLEE execution state
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
  /// \param replacements The replacement bound variables: As the resulting
  /// expression will
  /// be used for storing in the subsumption table, the variables need to be
  /// replaced with the bound ones.
  /// \return A pair of the store part indexed by constants, and the store part
  /// indexed by symbolic expressions.
  std::pair<Dependency::ConcreteStore, Dependency::SymbolicStore>
  getStoredCoreExpressions(std::set<const Array *> &replacements) const;

  void incInstructionsDepth();

  uint64_t getInstructionsDepth();

  /// \brief Marking the core constraints on the path condition, and all the
  /// relevant values on the dependency graph, given an unsatistiability core.
  void unsatCoreInterpolation(std::vector<ref<Expr> > unsatCore);

  /// \brief Memory bounds interpolation from a target address
  void pointerValuesInterpolation(llvm::Value *address) {
    dependency->markAllPointerValues(address);
  }

  /// \brief Print the content of the tree node object to the LLVM error stream.
  void dump() const;

  /// \brief Print the content of the tree node object into a stream.
  ///
  /// \param stream The stream to print the data to.
  void print(llvm::raw_ostream &stream) const;

private:
  ITreeNode(ITreeNode *_parent, llvm::DataLayout *_targetData);

  ~ITreeNode();

  void print(llvm::raw_ostream &stream, const unsigned paddingAmount) const;
};

/// \brief The top-level structure that implements lazy annotation.
///
/// The name ITree is an abbreviation of <i>interpolation tree</i>. The
/// interpolation
/// tree is just the symbolic execution tree, a parallel of what is implemented
/// by KLEE's existing PTree class, however, it adds shadow information for use
/// in lazy annotation. Each node of the interpolation tree is implemented in
/// the ITreeNode class. The ITree class itself contains several important
/// components:
///
/// 1. The subsumption table (as the member variable ITree#subsumptionTable).
/// This is the
///    database of states that have been generalized by the interpolation
/// process
///    to be used in subsuming other states. This member variable is a map
/// indexed
///    by code fragment id, which is the pointer value of the first instruction
///    of a basic block. It represents control location in a program. In a
///    subsumption check, the map is queried for such id of a state. If the
///    entry of the id is found in the subsumption table, then the corresponding
///    subsubmption table entries are compared against the state, possibly via a
///    call to the constraint solver.
///
/// 2. The root of the interpolation tree, which is an object of type ITreeNode,
///    and referenced by the member ITree#root.
///
/// 3. The currently-active interpolation tree node, which is also an object of
///    type ITreeNode, and referenced by the member ITree#currentINode.
///
/// ITree has several public member functions, most importantly, the various
/// versions of
/// the ITree::execute member function. The ITree::execute member functions are
/// called mainly from
/// the Executor class. The Executor class is the core symbolic executor of
/// KLEE.
/// Hooks are implemented in the Executor class that calls various polymorphic
/// variants of ITree::execute. The main functionality of the ITree::execute
/// themselves is to simply delegate the call to Dependency::execute, which
/// implements the shadow memory dependency computation used in computing the
/// regions of memory that need to be kept as part of the interpolant stored
/// in the subsumption table.
///
/// The member functions ITree::store and ITree::subsumptionCheck implement the
/// subsumption checking mechanism. ITree::store is called from the
/// ITree::remove
/// member function, which is invoked when the symbolic execution emanating from
/// a certain
/// state has finished and the state is to be removed. The completion of
/// the symbolic execution here is assumed to mean that the interpolants have
/// been
/// completely recorded from all the execution paths emanating from the state.
/// The ITree::store member functions builds an object of SubsumptionTableEntry
/// and stores
/// it in the subsumption table (member variable ITree#subsumptionTable).
///
/// To see how everything fits together, first we explain the important parts of
/// KLEE's
/// algorithm, and then we explain the modifications to KLEE's algorithm for
/// lazy annotation.
///
/// Following is the pseudocode of KLEE relevant to our discussion:
///
/// <hr>
/// <pre>
/// 1. Put the initial LLVM instruction into the tree root
/// 2. While there are leaves, do the following:
///    a. Pick a leaf
///    b. Symbolically execute the instruction:
///       i. If it is a branch, test if one of branches unsatisfiable
///          * If it is, execute the instruction without creating tree node
///          * Otherwise, generate the two tree nodes for the branches
///       ii. If it is an error/end point, register the leaf for deletion
///    c. Delete nodes registered for deletion. Recursively, if a
///       parent tree node is found to no longer have any children,
///       delete the parent as well, and recursively its parent
/// </pre>
/// <hr>
///
/// For comparison, following is the pseudocode of Tracer-X KLEE. Please note that to
/// support interpolation, each leaf is now augmented with a path condition.
/// We highlight the added procedures using CAPITAL LETTERS, and we note the member
/// functions involved.
///
/// <hr>
/// <pre>
/// 1. Put the initial LLVM instruction into the tree root
/// 2. While there are leaves, do the following:
///    a. Pick a leaf
///    b. IF THE LEAF IS SUBSUMED (ITree::subsumptionCheck)
///       i. REGISTER IT FOR DELETION
///       ii. MARK CONSTRAINTS NEEDED FOR SUBSUMPTION
///       iii. GOTO d
///    c. Symbolically execute the instruction (ITree::execute, ITree::executePHI,
///       ITree::executeMemoryOperation, ITree::executeOnNode):
///       i. If it is a branch, test if one of branches unsatisfiable
///          * If it is, execute the instruction without creating tree node
///            MARK CONSTRAINTS NEEDED FOR UNSATISFIABILITY
///            (ITree::markPathCondition)
///          * Otherwise, generate the two tree nodes for the branches
///            ADDING THE CONDITION AND NEGATION TO PATH CONDITION
///            (ITreeNode::addConstraint)
///       ii. If it is an error/end point, register the leaf for deletion
///    d. Delete nodes registered for deletion. Recursively, if a
///       parent tree node is found to no longer have any children,
///       delete the parent as well, and recursively its parent
///       FOR EACH DELETED NODE, STORE MARKED
///       CONSTRAINTS ON PC AS INTERPOLANT (ITree::store)
/// </pre>
/// <hr>
///
/// \see ITreeNode
/// \see Dependency
/// \see Executor
/// \see SubsumptionTableEntry
class ITree {
  typedef std::vector<ref<Expr> > ExprList;
  typedef ExprList::iterator iterator;
  typedef ExprList::const_iterator const_iterator;

  // Several static member variables for profiling the execution time of
  // this class's member functions.
  static Statistic setCurrentINodeTime;
  static Statistic removeTime;
  static Statistic subsumptionCheckTime;
  static Statistic markPathConditionTime;
  static Statistic splitTime;
  static Statistic executeOnNodeTime;
  static Statistic executeMemoryOperationTime;
  static double entryNumber;
  static double programPointNumber;

  /// \brief Number of subsumption checks for statistical purposes
  static uint64_t subsumptionCheckCount;

  ITreeNode *currentINode;

  std::map<uintptr_t, std::deque<SubsumptionTableEntry *> > subsumptionTable;

  llvm::DataLayout *targetData;

  void printNode(llvm::raw_ostream &stream, ITreeNode *n,
                 std::string edges) const;

  /// \brief Displays member functions running time statistics
  static void printTimeStat(std::stringstream &stream);

  /// \brief Displays subsumption table statistics
  static void printTableStat(std::stringstream &stream);

  /// \brief Utility function to represent double-precision floating point in
  /// two decimal points.
  static std::string inTwoDecimalPoints(const double n);

public:
  ITreeNode *root;

  /// \brief This static member variable is to indicate if we recovered from an
  /// error,
  /// e.g., memory bounds error, where the value of the previous instruction
  /// may not have been computed.
  static bool symbolicExecutionError;

  ITree(ExecutionState *_root, llvm::DataLayout *_targetData);

  ~ITree();

  /// \brief Store an entry into the subsumption table.
  void store(SubsumptionTableEntry *subItem);

  /// \brief Set the reference to the KLEE state in the current interpolation
  /// data holder (interpolation tree node) that is currently being processed.
  /// This also sets the id of the interpolation tree node to be the given
  /// pointer value.
  ///
  /// \param state The KLEE execution state to associate the current node with.
  void setCurrentINode(ExecutionState &state);

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
  /// This member function is to be invoked after KLEE splits its own state due
  /// to state
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
  void executePHI(llvm::Instruction *instr, unsigned incomingBlock,
                  ref<Expr> valueExpr);

  /// \brief For executing memory operations, called by
  /// Executor::executeMemoryOperation
  void executeMemoryOperation(llvm::Instruction *instr, ref<Expr> value,
                              ref<Expr> address, bool boundsCheck) {
    TimerStatIncrementer t(executeMemoryOperationTime);
    std::vector<ref<Expr> > args;
    args.push_back(value);
    args.push_back(address);
    currentINode->dependency->executeMemoryOperation(instr, args, boundsCheck,
                                                     symbolicExecutionError);
    symbolicExecutionError = false;
  }

  /// \brief General member function for executing an instruction for building
  /// dependency
  /// information, given a particular interpolation tree node.
  static void executeOnNode(ITreeNode *node, llvm::Instruction *instr,
                            std::vector<ref<Expr> > &args);

  /// \brief Print the content of the tree node object into a stream.
  ///
  /// \param stream The stream to print the data to.
  void print(llvm::raw_ostream &stream) const;

  /// \brief Print the content of the tree object to the LLVM error stream
  void dump() const;

  /// \brief Retrieve subsumption statistics result in std::string format
  static std::string getInterpolationStat();
};
}
#endif /* ITREE_H_ */
