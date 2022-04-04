//===-- ExecutionState.h ----------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_EXECUTIONSTATE_H
#define KLEE_EXECUTIONSTATE_H

#include "AddressSpace.h"

#include "klee/ADT/ImmutableSet.h"
#include "klee/ADT/TreeStream.h"
#include "klee/Core/TerminationTypes.h"
#include "klee/Expr/Assignment.h"
#include "klee/Expr/Constraints.h"
#include "klee/Expr/Expr.h"
#include "klee/Expr/ExprHashMap.h"
#include "klee/Module/KInstIterator.h"
#include "klee/Module/Target.h"
#include "klee/Module/TargetForest.h"
#include "klee/Module/TargetHash.h"
#include "klee/Solver/Solver.h"
#include "klee/System/Time.h"

#include "llvm/IR/Function.h"

#include <map>
#include <memory>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace klee {
class Array;
class CallPathNode;
struct Cell;
template <class T> class ExprHashMap;
struct KFunction;
struct KBlock;
struct KInstruction;
class MemoryObject;
class PTreeNode;
struct InstructionInfo;
struct Target;
struct TranstionHash;

llvm::raw_ostream &operator<<(llvm::raw_ostream &os, const MemoryMap &mm);

struct StackFrame {
  KInstIterator caller;
  KFunction *kf;
  CallPathNode *callPathNode;

  std::vector<IDType> allocas;
  Cell *locals;

  /// Minimum distance to an uncovered instruction once the function
  /// returns. This is not a good place for this but is used to
  /// quickly compute the context sensitive minimum distance to an
  /// uncovered instruction. This value is updated by the StatsTracker
  /// periodically.
  unsigned minDistToUncoveredOnReturn;

  // For vararg functions: arguments not passed via parameter are
  // stored (packed tightly) in a local (alloca) memory object. This
  // is set up to match the way the front-end generates vaarg code (it
  // does not pass vaarg through as expected). VACopy is lowered inside
  // of intrinsic lowering.
  MemoryObject *varargs;

  StackFrame(KInstIterator caller, KFunction *kf);
  StackFrame(const StackFrame &s);
  ~StackFrame();
};

/// Contains information related to unwinding (Itanium ABI/2-Phase unwinding)
class UnwindingInformation {
public:
  enum class Kind {
    SearchPhase, // first phase
    CleanupPhase // second phase
  };

private:
  const Kind kind;

public:
  // _Unwind_Exception* of the thrown exception, used in both phases
  ref<ConstantExpr> exceptionObject;

  Kind getKind() const { return kind; }

  explicit UnwindingInformation(ref<ConstantExpr> exceptionObject, Kind k)
      : kind(k), exceptionObject(exceptionObject) {}
  virtual ~UnwindingInformation() = default;

  virtual std::unique_ptr<UnwindingInformation> clone() const = 0;
};

struct SearchPhaseUnwindingInformation : public UnwindingInformation {
  // Keeps track of the stack index we have so far unwound to.
  std::size_t unwindingProgress;

  // MemoryObject that contains a serialized version of the last executed
  // landingpad, so we can clean it up after the personality fn returns.
  MemoryObject *serializedLandingpad = nullptr;

  SearchPhaseUnwindingInformation(ref<ConstantExpr> exceptionObject,
                                  std::size_t const unwindingProgress)
      : UnwindingInformation(exceptionObject,
                             UnwindingInformation::Kind::SearchPhase),
        unwindingProgress(unwindingProgress) {}

  std::unique_ptr<UnwindingInformation> clone() const {
    return std::make_unique<SearchPhaseUnwindingInformation>(*this);
  }

  static bool classof(const UnwindingInformation *u) {
    return u->getKind() == UnwindingInformation::Kind::SearchPhase;
  }
};

struct CleanupPhaseUnwindingInformation : public UnwindingInformation {
  // Phase 1 will try to find a catching landingpad.
  // Phase 2 will unwind up to this landingpad or return from
  // _Unwind_RaiseException if none was found.

  // The selector value of the catching landingpad that was found
  // during the search phase.
  ref<ConstantExpr> selectorValue;

  // Used to know when we have to stop unwinding and to
  // ensure that unwinding stops at the frame for which
  // we first found a handler in the search phase.
  const std::size_t catchingStackIndex;

  CleanupPhaseUnwindingInformation(ref<ConstantExpr> exceptionObject,
                                   ref<ConstantExpr> selectorValue,
                                   const std::size_t catchingStackIndex)
      : UnwindingInformation(exceptionObject,
                             UnwindingInformation::Kind::CleanupPhase),
        selectorValue(selectorValue), catchingStackIndex(catchingStackIndex) {}

  std::unique_ptr<UnwindingInformation> clone() const {
    return std::make_unique<CleanupPhaseUnwindingInformation>(*this);
  }

  static bool classof(const UnwindingInformation *u) {
    return u->getKind() == UnwindingInformation::Kind::CleanupPhase;
  }
};

struct Symbolic {
  ref<const MemoryObject> memoryObject;
  const Array *array;
  KType *type;
  Symbolic(ref<const MemoryObject> mo, const Array *a, KType *t)
      : memoryObject(mo), array(a), type(t) {}
  Symbolic &operator=(const Symbolic &other) = default;

  friend bool operator==(const Symbolic &lhs, const Symbolic &rhs) {
    return lhs.memoryObject == rhs.memoryObject && lhs.array == rhs.array &&
           lhs.type == rhs.type;
  }
};

struct MemorySubobject {
  ref<Expr> address;
  unsigned size;
  MemorySubobject(ref<Expr> address, unsigned size)
      : address(address), size(size) {}
  MemorySubobject &operator=(const MemorySubobject &other) = default;
};

struct MemorySubobjectHash {
  bool operator()(const MemorySubobject &a) const {
    return a.size * Expr::MAGIC_HASH_CONSTANT + a.address->hash();
  }
};

struct MemorySubobjectCompare {
  bool operator()(MemorySubobject a, MemorySubobject b) const {
    return a.address == b.address && a.size == b.size;
  }
};

typedef std::pair<llvm::BasicBlock *, llvm::BasicBlock *> Transition;

/// @brief ExecutionState representing a path under exploration
class ExecutionState {
#ifdef KLEE_UNITTEST
public:
#else
private:
#endif
  // copy ctor
  ExecutionState(const ExecutionState &state);

public:
  using stack_ty = std::vector<StackFrame>;
  using TargetHashSet =
      std::unordered_set<ref<Target>, RefTargetHash, RefTargetCmp>;

  // Execution - Control Flow specific

  /// @brief Pointer to initial instruction
  KInstIterator initPC;

  /// @brief Pointer to instruction to be executed after the current
  /// instruction
  KInstIterator pc;

  /// @brief Pointer to instruction which is currently executed
  KInstIterator prevPC;

  /// @brief Stack representing the current instruction stream
  stack_ty stack;

  int stackBalance = 0;

  /// @brief Remember from which Basic Block control flow arrived
  /// (i.e. to select the right phi values)
  std::int32_t incomingBBIndex = -1;

  // Overall state of the state - Data specific

  /// @brief Exploration depth, i.e., number of times KLEE branched for this
  /// state
  std::uint32_t depth = 0;

  /// @brief Exploration level, i.e., number of times KLEE cycled for this state
  std::unordered_map<llvm::BasicBlock *, unsigned long long> multilevel;
  std::unordered_set<llvm::BasicBlock *> level;
  std::unordered_set<Transition, TransitionHash> transitionLevel;

  /// @brief Address space used by this state (e.g. Global and Heap)
  AddressSpace addressSpace;

  /// @brief Constraints collected so far
  PathConstraints constraints;

  /// @brief Key points which should be visited through execution
  TargetForest targetForest;

  /// @brief Velocity and acceleration of this state investigating new blocks
  long long progressVelocity = 0;
  unsigned long progressAcceleration = 1;

  /// Statistics and information

  /// @brief Metadata utilized and collected by solvers for this state
  mutable SolverQueryMetaData queryMetaData;

  /// @brief History of complete path: represents branches taken to
  /// reach/create this state (both concrete and symbolic)
  TreeOStream pathOS;

  /// @brief History of symbolic path: represents symbolic branches
  /// taken to reach/create this state
  TreeOStream symPathOS;

  /// @brief Set containing which lines in which files are covered by this state
  std::map<const std::string *, std::set<std::uint32_t>> coveredLines;

  /// @brief Pointer to the process tree of the current state
  /// Copies of ExecutionState should not copy ptreeNode
  PTreeNode *ptreeNode = nullptr;

  /// @brief Ordered list of symbolics: used to generate test cases.
  //
  // FIXME: Move to a shared list structure (not critical).
  std::vector<Symbolic> symbolics;

  /// @brief map from memory accesses to accessed objects and access offsets.
  ExprHashMap<std::set<IDType>> resolvedPointers;
  std::unordered_map<MemorySubobject, std::set<IDType>, MemorySubobjectHash,
                     MemorySubobjectCompare>
      resolvedSubobjects;

  /// @brief A set of boolean expressions
  /// the user has requested be true of a counterexample.
  ImmutableSet<ref<Expr>> cexPreferences;

  /// @brief Set of used array names for this state.  Used to avoid collisions.
  std::map<std::string, uint64_t> arrayNames;

  /// @brief The numbers of times this state has run through
  /// Executor::stepInstruction
  std::uint64_t steppedInstructions = 0;

  /// @brief The numbers of times this state has run through
  /// Executor::stepInstruction with executeMemoryOperation
  std::uint64_t steppedMemoryInstructions = 0;

  /// @brief Counts how many instructions were executed since the last new
  /// instruction was covered.
  std::uint32_t instsSinceCovNew = 0;

  ///@brief State cfenv rounding mode
  llvm::APFloat::roundingMode roundingMode = llvm::APFloat::rmNearestTiesToEven;

  /// @brief Keep track of unwinding state while unwinding, otherwise empty
  std::unique_ptr<UnwindingInformation> unwindingInformation;

  /// @brief the global state counter
  static std::uint32_t nextID;

  /// @brief the state id
  std::uint32_t id = 0;

  /// @brief Whether a new instruction was covered in this state
  bool coveredNew = false;

  /// @brief Disables forking for this state. Set by user code
  bool forkDisabled = false;

  /// Needed for composition
  ref<Expr> returnValue;

  ExprHashMap<std::pair<ref<Expr>, llvm::Type *>> gepExprBases;

  ReachWithError error = ReachWithError::None;
  std::atomic<HaltExecution::Reason> terminationReasonType{
      HaltExecution::NotHalt};

public:
  // only to create the initial state
  explicit ExecutionState();
  explicit ExecutionState(KFunction *kf);
  explicit ExecutionState(KFunction *kf, KBlock *kb);
  // no copy assignment, use copy constructor
  ExecutionState &operator=(const ExecutionState &) = delete;
  // no move ctor
  ExecutionState(ExecutionState &&) noexcept = delete;
  // no move assignment
  ExecutionState &operator=(ExecutionState &&) noexcept = delete;
  // dtor
  ~ExecutionState();

  ExecutionState *branch();
  ExecutionState *withKFunction(KFunction *kf);
  ExecutionState *withStackFrame(KInstIterator caller, KFunction *kf);
  ExecutionState *withKInstruction(KInstruction *ki) const;
  ExecutionState *empty();
  ExecutionState *copy() const;

  bool inSymbolics(const MemoryObject *mo) const;

  void pushFrame(KInstIterator caller, KFunction *kf);
  void popFrame();

  void addSymbolic(const MemoryObject *mo, const Array *array,
                   KType *type = nullptr);

  ref<const MemoryObject> findMemoryObject(const Array *array) const;

  bool getBase(ref<Expr> expr,
               std::pair<ref<const MemoryObject>, ref<Expr>> &resolution) const;

  void removePointerResolutions(const MemoryObject *mo);
  void removePointerResolutions(ref<Expr> address, unsigned size);
  void addPointerResolution(ref<Expr> address, const MemoryObject *mo,
                            unsigned size = 0);
  void addUniquePointerResolution(ref<Expr> address, const MemoryObject *mo,
                                  unsigned size = 0);
  bool resolveOnSymbolics(const ref<ConstantExpr> &addr, IDType &result) const;

  void addConstraint(ref<Expr> e, const Assignment &c);
  void addCexPreference(const ref<Expr> &cond);

  void dumpStack(llvm::raw_ostream &out) const;

  bool visited(KBlock *block) const;

  std::uint32_t getID() const { return id; };
  void setID() { id = nextID++; };
  llvm::BasicBlock *getInitPCBlock() const;
  llvm::BasicBlock *getPrevPCBlock() const;
  llvm::BasicBlock *getPCBlock() const;
  void increaseLevel();
  bool isTransfered();
  bool isGEPExpr(ref<Expr> expr) const;

  bool reachedTarget(Target target) const;
  static std::uint32_t getLastID() { return nextID - 1; };
};

struct ExecutionStateIDCompare {
  bool operator()(const ExecutionState *a, const ExecutionState *b) const {
    return a->getID() < b->getID();
  }
};

using states_ty = std::set<ExecutionState *, ExecutionStateIDCompare>;

} // namespace klee

#endif /* KLEE_EXECUTIONSTATE_H */
