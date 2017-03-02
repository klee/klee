//===-- TxTree.h - Tracer-X symbolic execution tree -------------*- C++ -*-===//
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

#ifndef TXTREE_H_
#define TXTREE_H_

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

/// \brief The interpolation tree graph for outputting to .dot file.
class TxTreeGraph {

  /// \brief Global tree graph instance
  static TxTreeGraph *instance;

  /// Node information
  class Node {
    friend class TxTreeGraph;

    /// \brief The node id, also the order in which it is traversed
    uint64_t nodeSequenceNumber;

    /// \brief The id in case this was an internal node created in branchings
    /// due to memory access.
    uint64_t internalNodeId;

    /// \brief False and true children of this node
    TxTreeGraph::Node *falseTarget, *trueTarget;

    /// \brief Indicates that node is subsumed
    bool subsumed;

    /// \brief Conditions under which this node is visited from its parent
    std::map<PathCondition *, std::pair<std::string, bool> > pathConditionTable;

    /// \brief Human-readable identifier of this node
    std::string name;

    /// \brief Flag to indicate memory out-of-bound error within this node
    bool memoryError;

    /// \brief Location information of the memory out-of-bound error
    std::string memoryErrorLocation;

    Node()
        : nodeSequenceNumber(0), internalNodeId(0), falseTarget(0),
          trueTarget(0), subsumed(false), memoryError(false) {}

    ~Node() {
      if (falseTarget)
        delete falseTarget;

      if (trueTarget)
        delete trueTarget;

      pathConditionTable.clear();
    }

    static TxTreeGraph::Node *createNode() { return new TxTreeGraph::Node(); }
  };

  class NumberedEdge {
    TxTreeGraph::Node *source;
    TxTreeGraph::Node *destination;
    uint64_t number;

  public:
    NumberedEdge(TxTreeGraph::Node *_source, TxTreeGraph::Node *_destination,
                 uint64_t _number)
        : source(_source), destination(_destination), number(_number) {}

    ~NumberedEdge() {
      delete source;
      delete destination;
    }

    std::string render() const;
  };

  TxTreeGraph::Node *root;
  std::map<TxTreeNode *, TxTreeGraph::Node *> txTreeNodeMap;
  std::map<SubsumptionTableEntry *, TxTreeGraph::Node *> tableEntryMap;
  std::vector<TxTreeGraph::NumberedEdge *> subsumptionEdges;
  std::map<PathCondition *, TxTreeGraph::Node *> pathConditionMap;

  uint64_t subsumptionEdgeNumber;

  uint64_t internalNodeId;

  std::string recurseRender(TxTreeGraph::Node *node);

  std::string render();

  TxTreeGraph(TxTreeNode *_root);

  ~TxTreeGraph();

public:
  static void initialize(TxTreeNode *root) {
    if (!OUTPUT_INTERPOLATION_TREE)
      return;

    if (!instance)
      delete instance;
    instance = new TxTreeGraph(root);
  }

  static void deallocate() {
    if (!OUTPUT_INTERPOLATION_TREE)
      return;

    if (!instance)
      delete instance;
    instance = 0;
  }

  static void addChildren(TxTreeNode *parent, TxTreeNode *falseChild,
                          TxTreeNode *trueChild);

  static void setCurrentNode(ExecutionState &state,
                             const uint64_t _nodeSequenceNumber);

  static void markAsSubsumed(TxTreeNode *txTreeNode,
                             SubsumptionTableEntry *entry);

  static void addPathCondition(TxTreeNode *txTreeNode,
                               PathCondition *pathCondition,
                               ref<Expr> condition);

  static void addTableEntryMapping(TxTreeNode *txTreeNode,
                                   SubsumptionTableEntry *entry);

  static void setAsCore(PathCondition *pathCondition);

  static void setMemoryError(ExecutionState &state);

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

  /// \brief This is for dynamic setting up of debug messages.
  int debugLevel;

public:
  PathCondition(ref<Expr> &constraint, Dependency *dependency,
                llvm::Value *condition,
                const std::vector<llvm::Instruction *> &stack,
                PathCondition *prev, int _debugLevel);

  ~PathCondition();

  ref<Expr> car() const;

  PathCondition *cdr() const;

  void setAsCore();

  bool isCore() const;

  ref<Expr> packInterpolant(std::set<const Array *> &replacements);

  void dump() const;

  void print(llvm::raw_ostream &stream) const;
};

class SubsumptionTable {
  typedef std::deque<SubsumptionTableEntry *>::const_reverse_iterator
  EntryIterator;

  class StackIndexedTable {
    class Node {
      friend class StackIndexedTable;

      llvm::Instruction *id;

      std::deque<SubsumptionTableEntry *> entryList;

      Node *left, *right;

      Node(llvm::Instruction *_id) : id(_id) { left = right = 0; }

      void dump() const {
        this->print(llvm::errs());
        llvm::errs() << "\n";
      }

      void print(llvm::raw_ostream &stream) const;

      void print(llvm::raw_ostream &stream, const unsigned paddingAmount) const;

      void print(llvm::raw_ostream &stream, const std::string &prefix) const;
    };

    Node *root;

    void printNode(llvm::raw_ostream &stream, Node *n, std::string edges) const;

  public:
    StackIndexedTable() { root = new Node(0); }

    ~StackIndexedTable() { clearTree(root); }

    void clearTree(Node *node);

    void insert(const std::vector<llvm::Instruction *> &stack,
                SubsumptionTableEntry *entry);

    std::pair<EntryIterator, EntryIterator>
    find(const std::vector<llvm::Instruction *> &stack, bool &found) const;

    void dump() const {
      this->print(llvm::errs());
      llvm::errs() << "\n";
    }

    void print(llvm::raw_ostream &stream) const;
  };

  static std::map<uintptr_t, StackIndexedTable *> instance;

public:
  static void insert(uintptr_t id,
                     const std::vector<llvm::Instruction *> &stack,
                     SubsumptionTableEntry *entry);

  static bool check(TimingSolver *solver, ExecutionState &state, double timeout,
                    int debugLevel);

  static void clear();

  static void print(llvm::raw_ostream &stream) {
    for (std::map<uintptr_t, StackIndexedTable *>::const_iterator
             it = instance.begin(),
             ie = instance.end();
         it != ie; ++it) {
      stream << it->first << ": ";
      it->second->print(stream);
      stream << "\n";
    }
  }
};

/// \brief The class that implements an entry (record) in the subsumption table.
///
/// The subsumption table records the generalization of state such that
/// starting from this generalization, all symbolic execution will reach
/// the same conclusion (falsity of branch or successful memory bounds check).
/// Such generalization is a form of Craig interpolation. It is used to
/// subsume other symbolic execution paths encountered later, as any
/// extension of a subsumed path would only reach infeasibility or
/// the same conclusion that has already been seen.
///
/// This class implements an entry in the subsumption table. It is
/// instantiated when the a traversal of a symbolic execution subtree has
/// finished in the TxTree#remove method. This class basically stores a
/// subset of the path condition (the SubsumptionTableEntry#interpolant
/// field), plus the fragment of memory (allocations). They are components
/// that are needed to ensure the previously-seen conclusions. The memory
/// fragments are stored in either SubsumptionTableEntry#concreteAddressStore
/// or SubsumptionTableEntry#symbolicAddressStore, depending on whether
/// the memory fragment is concretely addressed or symbolically addressed.
/// Both fields are multi-level maps that are first indexed by the LLVM
/// value that represents the allocation (e.g., the call to <b>malloc</b>,
/// the <b>alloca</b> instruction, etc.).
///
/// \see TxTree
/// \see TxTreeNode
/// \see Dependency
class SubsumptionTableEntry {
  friend class TxTree;

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

  SubsumptionTableEntry(TxTreeNode *node,
                        const std::vector<llvm::Instruction *> &stack);

  ~SubsumptionTableEntry();

  bool subsumed(TimingSolver *solver, ExecutionState &state, double timeout,
                const std::pair<Dependency::ConcreteStore,
                                Dependency::SymbolicStore> storedExpressions,
                int debugLevel);

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

  void print(llvm::raw_ostream &stream, const unsigned paddingAmount) const;

  void print(llvm::raw_ostream &stream, const std::string &prefix) const;
};

/// \brief The Tracer-X symbolic execution tree node.
///
/// This class is a higher-level wrapper to the path condition (referenced
/// by the member variable TxTreeNode#pathCondition of type PathCondition), and
/// the shadow
/// memory for memory dependency computation (referenced by the member
/// variable TxTreeNode#dependency of type Dependency).
///
/// Each Tracer-X tree node has an associated KLEE execution state
/// (implemented using the type ExecutionState) from which it is referenced
/// via the member variable ExecutionState#txTreeNode.
/// It adds information for abstraction learning to the ExecutionState object.
/// The whole structure of the Tracer-X tree itself is maintained by
/// the class TxTree, which also refers to objects of type TxTreeNode via
/// its TxTree#root and TxTree#currentINode member variables.
///
/// \see TxTree
/// \see Dependency
/// \see SubsumptionTableEntry
/// \see PathCondition
class TxTreeNode {
  friend class TxTree;

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

  TxTreeNode *parent, *left, *right;

  uintptr_t programPoint;

  uint64_t nodeSequenceNumber;

  bool storable;

  /// \brief Graph for displaying as .dot file
  TxTreeGraph *graph;

  /// \brief For statistics on the number of instructions executed along a path.
  uint64_t instructionsDepth;

  /// \brief The data layout of the analysis target
  llvm::DataLayout *targetData;

public:
  bool isSubsumed;

  /// \brief The entry call stack
  std::vector<llvm::Instruction *> entryCallStack;

  /// \brief The current call stack
  std::vector<llvm::Instruction *> callStack;

  /// \brief This is for dynamic setting up of debug messages.
  int debugLevel;

private:
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
  /// \param stack The current callsite stack for this value
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
  getStoredExpressions(const std::vector<llvm::Instruction *> &stack) const;

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
  getStoredCoreExpressions(const std::vector<llvm::Instruction *> &stack,
                           std::set<const Array *> &replacements) const;

  void incInstructionsDepth();

  uint64_t getInstructionsDepth();

  /// \brief Marking the core constraints on the path condition, and all the
  /// relevant values on the dependency graph, given an unsatistiability core.
  void unsatCoreInterpolation(const std::vector<ref<Expr> > &unsatCore);

  /// \brief Memory bounds interpolation from a target address
  void pointerValuesInterpolation(llvm::Value *value, ref<Expr> address,
                                  std::set<ref<Expr> > &bounds,
                                  const std::string &reason) {
    dependency->markAllPointerValues(value, address, bounds, reason);
  }

  /// \brief Print the content of the tree node object to the LLVM error stream.
  void dump() const;

  /// \brief Print the content of the tree node object into a stream.
  ///
  /// \param stream The stream to print the data to.
  void print(llvm::raw_ostream &stream) const;

private:
  TxTreeNode(TxTreeNode *_parent, llvm::DataLayout *_targetData);

  ~TxTreeNode();

  void print(llvm::raw_ostream &stream, const unsigned paddingAmount) const;
};

/// \brief The top-level structure that implements abstraction learning.
///
/// The TxTree is just the symbolic execution tree, a parallel of what is implemented
/// by KLEE's existing PTree class, however, it implements shadow information for use
/// in abstraction learning. Each node of the tree is implemented by
/// the TxTreeNode class. The TxTree class itself contains several important
/// components:
///
/// 1. The subsumption table (as the member variable TxTree#subsumptionTable).
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
/// 2. The root of the Tracer-X tree, which is an object of type TxTreeNode,
///    and referenced by the member TxTree#root.
///
/// 3. The currently-active tree node, which is also an object of
///    type TxTreeNode, and referenced by the member TxTree#currentINode.
///
/// TxTree has several public member functions, most importantly, the various
/// versions of
/// the TxTree::execute member function. The TxTree::execute member functions are
/// called mainly from
/// the Executor class. The Executor class is the core symbolic executor of
/// KLEE.
/// Hooks are implemented in the Executor class that calls various polymorphic
/// variants of TxTree::execute. The main functionality of the TxTree::execute
/// themselves is to simply delegate the call to Dependency::execute, which
/// implements the shadow memory dependency computation used in computing the
/// regions of memory that need to be kept as part of the interpolant stored
/// in the subsumption table.
///
/// The member functions TxTree::store and TxTree::subsumptionCheck implement the
/// subsumption checking mechanism. TxTree::store is called from the
/// TxTree::remove
/// member function, which is invoked when the symbolic execution emanating from
/// a certain
/// state has finished and the state is to be removed. The completion of
/// the symbolic execution here is assumed to mean that the interpolants have
/// been
/// completely recorded from all the execution paths emanating from the state.
/// The TxTree::store member functions builds an object of SubsumptionTableEntry
/// and stores
/// it in the subsumption table (member variable TxTree#subsumptionTable).
///
/// To see how everything fits together, first we explain the important parts of
/// KLEE's
/// algorithm, and then we explain the modifications to KLEE's algorithm for
/// abstraction learning.
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
///    b. IF THE LEAF IS SUBSUMED (TxTree::subsumptionCheck)
///       i. REGISTER IT FOR DELETION
///       ii. MARK CONSTRAINTS NEEDED FOR SUBSUMPTION
///       iii. GOTO d
///    c. Symbolically execute the instruction (TxTree::execute, TxTree::executePHI,
///       TxTree::executeMemoryOperation, TxTree::executeOnNode):
///       i. If it is a branch, test if one of branches unsatisfiable
///          * If it is, execute the instruction without creating tree node
///            MARK CONSTRAINTS NEEDED FOR UNSATISFIABILITY
///            (TxTree::markPathCondition)
///          * Otherwise, generate the two tree nodes for the branches
///            ADDING THE CONDITION AND NEGATION TO PATH CONDITION
///            (TxTreeNode::addConstraint)
///       ii. If it is an error/end point, register the leaf for deletion
///    d. Delete nodes registered for deletion. Recursively, if a
///       parent tree node is found to no longer have any children,
///       delete the parent as well, and recursively its parent
///       FOR EACH DELETED NODE, STORE MARKED
///       CONSTRAINTS ON PC AS INTERPOLANT (TxTree::store)
/// </pre>
/// <hr>
///
/// \see TxTreeNode
/// \see Dependency
/// \see Executor
/// \see SubsumptionTableEntry
class TxTree {
  typedef std::vector<ref<Expr> > ExprList;
  typedef ExprList::iterator iterator;
  typedef ExprList::const_iterator const_iterator;

  TxTreeNode *currentINode;

  llvm::DataLayout *targetData;

  void printNode(llvm::raw_ostream &stream, TxTreeNode *n,
                 std::string edges) const;

  /// \brief Displays member functions running time statistics
  static void printTimeStat(std::stringstream &stream);

  /// \brief Displays subsumption table statistics
  static void printTableStat(std::stringstream &stream);

  /// \brief Utility function to represent double-precision floating point in
  /// two decimal points.
  static std::string inTwoDecimalPoints(const double n);

public:
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

  /// \brief The root node of the tree
  TxTreeNode *root;

  /// \brief This static member variable is to indicate if we recovered from an
  /// error,
  /// e.g., memory bounds error, where the value of the previous instruction
  /// may not have been computed.
  static bool symbolicExecutionError;

  TxTree(ExecutionState *_root, llvm::DataLayout *_targetData);

  ~TxTree() { SubsumptionTable::clear(); }

  /// \brief Set the reference to the KLEE state in the current interpolation
  /// data holder (Tracer-X tree node) that is currently being processed.
  /// This also sets the id of the Tracer-X tree node to be the given
  /// pointer value.
  ///
  /// \param state The KLEE execution state to associate the current node with.
  void setCurrentINode(ExecutionState &state);

  /// \brief Deletes the Tracer-X tree node
  void remove(TxTreeNode *node);

  /// \brief Invokes the subsumption check
  bool subsumptionCheck(TimingSolver *solver, ExecutionState &state,
                        double timeout);

  /// \brief Mark the path condition in the Tracer-X tree node associated
  /// with the given KLEE execution state.
  void markPathCondition(ExecutionState &state, TimingSolver *solver);

  /// \brief Creates fresh interpolation data holder for the two given KLEE
  /// execution states.
  /// This member function is to be invoked after KLEE splits its own state due
  /// to state
  /// forking.
  std::pair<TxTreeNode *, TxTreeNode *>
  split(TxTreeNode *parent, ExecutionState *left, ExecutionState *right);

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
    executeMemoryOperationOnNode(currentINode, instr, value, address,
                                 boundsCheck);
  }

  /// \brief Internal method for executing memory operations
  static void executeMemoryOperationOnNode(TxTreeNode *node,
                                           llvm::Instruction *instr,
                                           ref<Expr> value, ref<Expr> address,
                                           bool boundsCheck) {
    TimerStatIncrementer t(executeMemoryOperationTime);
    std::vector<ref<Expr> > args;
    args.push_back(value);
    args.push_back(address);
    node->dependency->executeMemoryOperation(
        instr, node->callStack, args, boundsCheck, symbolicExecutionError);
    symbolicExecutionError = false;
  }

  /// \brief General member function for executing an instruction for building
  /// dependency
  /// information, given a particular Tracer-X tree node.
  static void executeOnNode(TxTreeNode *node, llvm::Instruction *instr,
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
#endif /* TXTREE_H_ */
