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

#include "klee/ADT/ImmutableList.h"
#include "klee/ADT/ImmutableSet.h"
#include "klee/ADT/PersistentMap.h"
#include "klee/ADT/PersistentSet.h"
#include "klee/ADT/TreeStream.h"
#include "klee/Core/TerminationTypes.h"
#include "klee/Expr/Assignment.h"
#include "klee/Expr/Constraints.h"
#include "klee/Expr/Expr.h"
#include "klee/Expr/ExprHashMap.h"
#include "klee/Module/KInstIterator.h"
#include "klee/Module/KInstruction.h"
#include "klee/Module/Target.h"
#include "klee/Module/TargetForest.h"
#include "klee/Module/TargetHash.h"
#include "klee/Solver/Solver.h"
#include "klee/System/Time.h"
#include "klee/Utilities/Math.h"

#include "klee/Support/CompilerWarning.h"
DISABLE_WARNING_PUSH
DISABLE_WARNING_DEPRECATED_DECLARATIONS
#include "llvm/IR/Function.h"
DISABLE_WARNING_POP

#include <cstddef>
#include <deque>
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
class Target;

llvm::raw_ostream &operator<<(llvm::raw_ostream &os, const MemoryMap &mm);

extern llvm::cl::opt<unsigned long long> MaxCyclesBeforeStuck;

struct CallStackFrame {
  KInstIterator caller;
  KFunction *kf;

  CallStackFrame(KInstIterator caller_, KFunction *kf_)
      : caller(caller_), kf(kf_) {}
  ~CallStackFrame() = default;

  bool equals(const CallStackFrame &other) const;

  bool operator==(const CallStackFrame &other) const { return equals(other); }
};

struct StackFrame {
  KFunction *kf;
  std::vector<IDType> allocas;
  Cell *locals;

  // For vararg functions: arguments not passed via parameter are
  // stored (packed tightly) in a local (alloca) memory object. This
  // is set up to match the way the front-end generates vaarg code (it
  // does not pass vaarg through as expected). VACopy is lowered inside
  // of intrinsic lowering.
  MemoryObject *varargs;

  StackFrame(KFunction *kf);
  StackFrame(const StackFrame &s);
  ~StackFrame();
};

struct InfoStackFrame {
  KFunction *kf;
  CallPathNode *callPathNode = nullptr;
  PersistentMap<llvm::BasicBlock *, unsigned long long> multilevel;

  /// Minimum distance to an uncovered instruction once the function
  /// returns. This is not a good place for this but is used to
  /// quickly compute the context sensitive minimum distance to an
  /// uncovered instruction. This value is updated by the StatsTracker
  /// periodically.
  unsigned minDistToUncoveredOnReturn = 0;

  InfoStackFrame(KFunction *kf);
  ~InfoStackFrame() = default;
};

struct ExecutionStack {
public:
  using value_stack_ty = std::vector<StackFrame>;
  using call_stack_ty = std::vector<CallStackFrame>;
  using info_stack_ty = std::vector<InfoStackFrame>;

private:
  value_stack_ty valueStack_;
  call_stack_ty callStack_;
  info_stack_ty infoStack_;
  call_stack_ty uniqueFrames_;
  size_t stackSize = 0;
  unsigned stackBalance = 0;

public:
  PersistentMap<KFunction *, unsigned long long> multilevel;
  void pushFrame(KInstIterator caller, KFunction *kf);
  void popFrame();
  inline value_stack_ty &valueStack() { return valueStack_; }
  inline const value_stack_ty &valueStack() const { return valueStack_; }
  inline const call_stack_ty &callStack() const { return callStack_; }
  inline const info_stack_ty &infoStack() const { return infoStack_; }
  inline info_stack_ty &infoStack() { return infoStack_; }
  inline const call_stack_ty &uniqueFrames() const { return uniqueFrames_; }

  inline unsigned size() const { return callStack_.size(); }
  inline size_t stackRegisterSize() const { return stackSize; }
  inline bool empty() const { return callStack_.empty(); }
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
      : memoryObject(std::move(mo)), array(a), type(t) {}
  Symbolic(const Symbolic &other) = default;
  Symbolic &operator=(const Symbolic &other) = default;

  friend bool operator==(const Symbolic &lhs, const Symbolic &rhs) {
    return lhs.memoryObject == rhs.memoryObject && lhs.array == rhs.array &&
           lhs.type == rhs.type;
  }
};

struct MemorySubobject {
  ref<Expr> address;
  unsigned size;
  explicit MemorySubobject(ref<Expr> address, unsigned size)
      : address(address), size(size) {}
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
  using stack_ty = ExecutionStack;

  // Execution - Control Flow specific

  /// @brief Pointer to initial instruction
  KInstIterator initPC;

  /// @brief Pointer to instruction to be executed after the current
  /// instruction
  KInstIterator pc;

  /// @brief Pointer to instruction which is currently executed
  KInstIterator prevPC;

  /// @brief Execution stack representing the current instruction stream
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
  PersistentSet<KBlock *, KBlockCompare> level;

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
  std::map<std::string, std::set<unsigned>> coveredLines;

  /// @brief Pointer to the process tree of the current state
  /// Copies of ExecutionState should not copy ptreeNode
  PTreeNode *ptreeNode = nullptr;

  /// @brief Ordered list of symbolics: used to generate test cases.
  ImmutableList<Symbolic> symbolics;

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
  mutable std::deque<ref<box<bool>>> coveredNew;
  mutable ref<box<bool>> coveredNewError;

  /// @brief Disables forking for this state. Set by user code
  bool forkDisabled = false;

  /// Needed for composition
  ref<Expr> returnValue;

  ExprHashMap<std::pair<ref<Expr>, llvm::Type *>> gepExprBases;

  ReachWithError error = ReachWithError::None;
  std::atomic<HaltExecution::Reason> terminationReasonType{
      HaltExecution::NotHalt};

private:
  PersistentSet<ref<Target>> prevTargets_;
  PersistentSet<ref<Target>> targets_;
  ref<TargetsHistory> prevHistory_;
  ref<TargetsHistory> history_;
  bool isTargeted_ = false;
  bool areTargetsChanged_ = false;

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

  void addConstraint(ref<Expr> e, const Assignment &c);
  void addCexPreference(const ref<Expr> &cond);

  Query toQuery(ref<Expr> head) const;
  Query toQuery() const;

  void dumpStack(llvm::raw_ostream &out) const;

  bool visited(KBlock *block) const;

  std::uint32_t getID() const { return id; };
  void setID() {
    id = nextID++;
    queryMetaData.id = id;
  };
  llvm::BasicBlock *getInitPCBlock() const;
  llvm::BasicBlock *getPrevPCBlock() const;
  llvm::BasicBlock *getPCBlock() const;
  void increaseLevel();

  inline bool isTransfered() const { return getPrevPCBlock() != getPCBlock(); }

  bool isGEPExpr(ref<Expr> expr) const;

  inline const PersistentSet<ref<Target>> &prevTargets() const {
    return prevTargets_;
  }

  inline const PersistentSet<ref<Target>> &targets() const { return targets_; }

  inline ref<const TargetsHistory> prevHistory() const { return prevHistory_; }

  inline ref<const TargetsHistory> history() const { return history_; }

  inline bool isTargeted() const { return isTargeted_; }

  inline bool areTargetsChanged() const { return areTargetsChanged_; }

  void stepTargetsAndHistory() {
    prevHistory_ = history_;
    prevTargets_ = targets_;
    areTargetsChanged_ = false;
  }

  inline void setTargeted(bool targeted) { isTargeted_ = targeted; }

  inline void setTargets(const TargetHashSet &targets) {
    targets_ = PersistentSet<ref<Target>>();
    for (auto target : targets) {
      targets_.insert(target);
    }
    areTargetsChanged_ = true;
  }

  inline void setHistory(ref<TargetsHistory> history) {
    history_ = history;
    areTargetsChanged_ = true;
  }

  bool reachedTarget(ref<ReachBlockTarget> target) const;
  static std::uint32_t getLastID() { return nextID - 1; };

  inline bool isCycled(unsigned long long bound) const {
    if (bound == 0)
      return false;
    if (prevPC->inst->isTerminator() && stack.size() > 0) {
      auto &ml = stack.infoStack().back().multilevel;
      auto level = ml.find(getPCBlock());
      return level != ml.end() && level->second > bound;
    }
    if (pc == pc->parent->getFirstInstruction() &&
        pc->parent == pc->parent->parent->entryKBlock) {
      auto level = stack.multilevel.at(stack.callStack().back().kf);
      return level > bound;
    }
    return false;
  }

  inline bool isStuck(unsigned long long bound) const {
    if (depth == 0)
      return false;
    return isCycled(bound) && klee::util::ulog2(depth) > bound;
  }

  bool isCoveredNew() const {
    return !coveredNew.empty() && coveredNew.back()->value;
  }
  bool isCoveredNewError() const { return coveredNewError->value; }
  void coverNew() const {
    coveredNew.push_back(new box<bool>(true));
    coveredNewError->value = false;
    coveredNewError = new box<bool>(true);
  }
  void updateCoveredNew() const {
    while (!coveredNew.empty() && !coveredNew.front()->value) {
      coveredNew.pop_front();
    }
  }
  void clearCoveredNew() const {
    for (auto signal : coveredNew) {
      signal->value = false;
    }
    coveredNew.clear();
  }
  void clearCoveredNewError() const { coveredNewError->value = false; }
};

struct ExecutionStateIDCompare {
  bool operator()(const ExecutionState *a, const ExecutionState *b) const {
    return a->getID() < b->getID();
  }
};

using states_ty = std::set<ExecutionState *, ExecutionStateIDCompare>;

} // namespace klee

#endif /* KLEE_EXECUTIONSTATE_H */
