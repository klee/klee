//===-- Executor.h ----------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Class to perform actual execution, hides implementation details from external
// interpreter.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_EXECUTOR_H
#define KLEE_EXECUTOR_H

#include "ExecutionState.h"
#include "TargetedExecutionManager.h"
#include "UserSearcher.h"

#include "klee/ADT/RNG.h"
#include "klee/Core/BranchTypes.h"
#include "klee/Core/Interpreter.h"
#include "klee/Core/TerminationTypes.h"
#include "klee/Expr/ArrayCache.h"
#include "klee/Expr/ArrayExprOptimizer.h"
#include "klee/Expr/Constraints.h"
#include "klee/Expr/SourceBuilder.h"
#include "klee/Expr/SymbolicSource.h"
#include "klee/Module/Cell.h"
#include "klee/Module/KInstruction.h"
#include "klee/Module/KModule.h"
#include "klee/System/Time.h"

#include "klee/Support/CompilerWarning.h"
DISABLE_WARNING_PUSH
DISABLE_WARNING_DEPRECATED_DECLARATIONS
#include "llvm/ADT/Twine.h"
#include "llvm/IR/Argument.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/Support/raw_ostream.h"
DISABLE_WARNING_POP

#include <deque>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

struct KTest;

namespace llvm {
class BasicBlock;
class BranchInst;
class CallInst;
class LandingPadInst;
class Constant;
class ConstantExpr;
class Function;
class GlobalValue;
class Instruction;
class LLVMContext;
class DataLayout;
class Twine;
class Value;
} // namespace llvm

namespace klee {
class AddressManager;
class Array;
struct Cell;
class CodeGraphInfo;
class DistanceCalculator;
class ExecutionState;
class ExternalDispatcher;
class Expr;
template <class T> class ExprHashMap;
class KCallable;
struct KFunction;
struct KInstruction;
class KInstIterator;
class KModule;
class MemoryManager;
class MemoryObject;
class ObjectState;
class PForest;
class Searcher;
class SeedInfo;
class SpecialFunctionHandler;
struct StackFrame;
class SymbolicSource;
class TargetCalculator;
class TargetManager;
class StatsTracker;
class TimingSolver;
class TreeStreamWriter;
class TypeManager;
class MergeHandler;
class MergingSearcher;
template <class T> class ref;

/// \todo Add a context object to keep track of data only live
/// during an instruction step. Should contain addedStates,
/// removedStates, and haltExecution, among others.

class Executor : public Interpreter {
  friend class OwningSearcher;
  friend class WeightedRandomSearcher;
  friend class SpecialFunctionHandler;
  friend class StatsTracker;
  friend klee::Searcher *
  klee::constructUserSearcher(Executor &executor, bool stopAfterReachingTarget);

public:
  typedef std::pair<ExecutionState *, ExecutionState *> StatePair;

  /// The random number generator.
  RNG theRNG;

private:
  int *errno_addr;

  using SetOfStates = std::set<ExecutionState *, ExecutionStateIDCompare>;
  /* Set of Intrinsic::ID. Plain type is used here to avoid including llvm in
   * the header */
  static const std::unordered_set<llvm::Intrinsic::ID> supportedFPIntrinsics;
  static const std::unordered_set<llvm::Intrinsic::ID> modelledFPIntrinsics;

  std::unique_ptr<KModule> kmodule;
  InterpreterHandler *interpreterHandler;
  Searcher *searcher;

  ExternalDispatcher *externalDispatcher;
  std::unique_ptr<TimingSolver> solver;
  std::unique_ptr<AddressManager> addressManager;
  std::unique_ptr<MemoryManager> memory;
  TypeManager *typeSystemManager;

  SetOfStates states;
  SetOfStates pausedStates;
  StatsTracker *statsTracker;
  TreeStreamWriter *pathWriter, *symPathWriter;
  SpecialFunctionHandler *specialFunctionHandler;
  TimerGroup timers;
  std::unique_ptr<PForest> processForest;
  GuidanceKind guidanceKind;
  std::unique_ptr<CodeGraphInfo> codeGraphInfo;
  std::unique_ptr<DistanceCalculator> distanceCalculator;
  std::unique_ptr<TargetCalculator> targetCalculator;
  std::unique_ptr<TargetManager> targetManager;

  /// Used to track states that have been added during the current
  /// instructions step.
  /// \invariant \ref addedStates is a subset of \ref states.
  /// \invariant \ref addedStates and \ref removedStates are disjoint.
  std::vector<ExecutionState *> addedStates;
  /// Used to track states that have been removed during the current
  /// instructions step.
  /// \invariant \ref removedStates is a subset of \ref states.
  /// \invariant \ref addedStates and \ref removedStates are disjoint.
  std::vector<ExecutionState *> removedStates;

  /// When non-empty the Executor is running in "seed" mode. The
  /// states in this map will be executed in an arbitrary order
  /// (outside the normal search interface) until they terminate. When
  /// the states reach a symbolic branch then either direction that
  /// satisfies one or more seeds will be added to this map. What
  /// happens with other states (that don't satisfy the seeds) depends
  /// on as-yet-to-be-determined flags.
  std::map<ExecutionState *, std::vector<SeedInfo>> seedMap;

  /// Map of globals to their representative memory object.
  std::map<const llvm::GlobalValue *, MemoryObject *> globalObjects;

  /// Map of globals to their bound address. This also includes
  /// globals that have no representative object (i.e. functions).
  std::map<const llvm::GlobalValue *, ref<ConstantExpr>> globalAddresses;

  /// Map of legal function addresses to the corresponding Function.
  /// Used to validate and dereference function pointers.
  std::unordered_map<std::uint64_t, llvm::Function *> legalFunctions;

  /// Manager for everything related to targeted execution mode
  std::unique_ptr<TargetedExecutionManager> targetedExecutionManager;

  /// When non-null the bindings that will be used for calls to
  /// klee_make_symbolic in order replay.
  const struct KTest *replayKTest;

  /// When non-null a list of branch decisions to be used for replay.
  const std::vector<bool> *replayPath;

  /// The index into the current \ref replayKTest or \ref replayPath
  /// object.
  unsigned replayPosition;

  /// When non-null a list of "seed" inputs which will be used to
  /// drive execution.
  const std::vector<struct KTest *> *usingSeeds;

  /// Disables forking, instead a random path is chosen. Enabled as
  /// needed to control memory usage. \see fork()
  bool atMemoryLimit;

  /// Disables forking, set by client. \see setInhibitForking()
  bool inhibitForking;

  /// Signals the executor to halt execution at the next instruction
  /// step.
  HaltExecution::Reason haltExecution = HaltExecution::NotHalt;

  /// Whether implied-value concretization is enabled. Currently
  /// false, it is buggy (it needs to validate its writes).
  bool ivcEnabled;

  /// The maximum time to allow for a single core solver query.
  /// (e.g. for a single STP query)
  time::Span coreSolverTimeout;

  /// Maximum time to allow for a single instruction.
  time::Span maxInstructionTime;

  /// Assumes ownership of the created array objects
  ArrayCache arrayCache;

  /// File to print executed instructions to
  std::unique_ptr<llvm::raw_ostream> debugInstFile;

  // @brief Buffer used by logBuffer
  std::string debugBufferString;

  // @brief buffer to store logs before flushing to file
  llvm::raw_string_ostream debugLogBuffer;

  /// Optimizes expressions
  ExprOptimizer optimizer;

  std::unordered_map<KFunction *, TargetedHaltsOnTraces> targets;

  /// Typeids used during exception handling
  std::vector<ref<Expr>> eh_typeids;

  bool hasStateWhichCanReachSomeTarget = false;

  /// Return the typeid corresponding to a certain `type_info`
  ref<ConstantExpr> getEhTypeidFor(ref<Expr> type_info);

  void addHistoryResult(ExecutionState &state);

  void executeInstruction(ExecutionState &state, KInstruction *ki);

  void targetedRun(ExecutionState &initialState, KBlock *target,
                   ExecutionState **resultState = nullptr);

  void seed(ExecutionState &initialState);
  void run(std::vector<ExecutionState *> initialStates);
  void runWithTarget(ExecutionState &state, KFunction *kf, KBlock *target);

  void initializeTypeManager();

  // Given a concrete object in our [klee's] address space, add it to
  // objects checked code can reference.
  MemoryObject *addExternalObject(ExecutionState &state, void *addr, KType *,
                                  unsigned size, bool isReadOnly);

  void initializeGlobalAlias(const llvm::Constant *c, ExecutionState &state);
  void initializeGlobalObject(ExecutionState &state, ObjectState *os,
                              const llvm::Constant *c, unsigned offset);
  void initializeGlobals(ExecutionState &state);
  void allocateGlobalObjects(ExecutionState &state);
  void initializeGlobalAliases(ExecutionState &state);
  void initializeGlobalObjects(ExecutionState &state);

  void stepInstruction(ExecutionState &state);
  void updateStates(ExecutionState *current);
  void transferToBasicBlock(llvm::BasicBlock *dst, llvm::BasicBlock *src,
                            ExecutionState &state);
  void transferToBasicBlock(KBlock *dst, llvm::BasicBlock *src,
                            ExecutionState &state);

  void callExternalFunction(ExecutionState &state, KInstruction *target,
                            KCallable *callable,
                            std::vector<ref<Expr>> &arguments);

  ObjectState *bindObjectInState(ExecutionState &state, const MemoryObject *mo,
                                 KType *dynamicType, bool IsAlloca,
                                 const Array *array = nullptr);

  /// Resolve a pointer to the memory objects it could point to the
  /// start of, forking execution when necessary and generating errors
  /// for pointers to invalid locations (either out of bounds or
  /// address inside the middle of objects).
  ///
  /// \param results[out] A list of ((MemoryObject,ObjectState),
  /// state) pairs for each object the given address can point to the
  /// beginning of.
  typedef std::vector<std::pair<IDType, ExecutionState *>> ExactResolutionList;
  bool resolveExact(ExecutionState &state, ref<Expr> p, KType *type,
                    ExactResolutionList &results, const std::string &name);

  void concretizeSize(ExecutionState &state, ref<Expr> size, bool isLocal,
                      KInstruction *target, KType *type, bool zeroMemory,
                      const ObjectState *reallocFrom,
                      size_t allocationAlignment, bool checkOutOfMemory);

  bool computeSizes(ExecutionState &state, ref<Expr> size,
                    ref<Expr> symbolicSizesSum,
                    std::vector<const Array *> &objects,
                    std::vector<SparseStorage<unsigned char>> &values);

  MemoryObject *allocate(ExecutionState &state, ref<Expr> size, bool isLocal,
                         bool isGlobal, const llvm::Value *allocSite,
                         size_t allocationAlignment,
                         ref<Expr> lazyInitializationSource = ref<Expr>(),
                         unsigned timestamp = 0);

  /// Allocate and bind a new object in a particular state. NOTE: This
  /// function may fork.
  ///
  /// \param isLocal Flag to indicate if the object should be
  /// automatically deallocated on function return (this also makes it
  /// illegal to free directly).
  ///
  /// \param target Value at which to bind the base address of the new
  /// object.
  ///
  /// \param reallocFrom If non-zero and the allocation succeeds,
  /// initialize the new object from the given one and unbind it when
  /// done (realloc semantics). The initialized bytes will be the
  /// minimum of the size of the old and new objects, with remaining
  /// bytes initialized as specified by zeroMemory.
  ///
  /// \param allocationAlignment If non-zero, the given alignment is
  /// used. Otherwise, the alignment is deduced via
  /// Executor::getAllocationAlignment
  void executeAlloc(ExecutionState &state, ref<Expr> size, bool isLocal,
                    KInstruction *target, KType *type, bool zeroMemory = false,
                    const ObjectState *reallocFrom = 0,
                    size_t allocationAlignment = 0,
                    bool checkOutOfMemory = false);

  /// Free the given address with checking for errors. If target is
  /// given it will be bound to 0 in the resulting states (this is a
  /// convenience for realloc). Note that this function can cause the
  /// state to fork and that \ref state cannot be safely accessed
  /// afterwards.
  void executeFree(ExecutionState &state, ref<Expr> address,
                   KInstruction *target = 0);

  /// Serialize a landingpad instruction so it can be handled by the
  /// libcxxabi-runtime
  MemoryObject *serializeLandingpad(ExecutionState &state,
                                    const llvm::LandingPadInst &lpi,
                                    bool &stateTerminated);

  /// Unwind the given state until it hits a landingpad. This is used
  /// for exception handling.
  void unwindToNextLandingpad(ExecutionState &state);

  void executeCall(ExecutionState &state, KInstruction *ki, llvm::Function *f,
                   std::vector<ref<Expr>> &arguments);

  bool resolveMemoryObjects(ExecutionState &state, ref<Expr> address,
                            KType *targetType, KInstruction *target,
                            unsigned bytes,
                            std::vector<IDType> &mayBeResolvedMemoryObjects,
                            bool &mayBeOutOfBound, bool &mayLazyInitialize,
                            bool &incomplete, bool onlyLazyInitialize = false);

  bool checkResolvedMemoryObjects(
      ExecutionState &state, ref<Expr> address, KInstruction *target,
      unsigned bytes, const std::vector<IDType> &mayBeResolvedMemoryObjects,
      bool hasLazyInitialized, std::vector<IDType> &resolvedMemoryObjects,
      std::vector<ref<Expr>> &resolveConditions,
      std::vector<ref<Expr>> &unboundConditions, ref<Expr> &checkOutOfBounds,
      bool &mayBeOutOfBound);

  bool makeGuard(ExecutionState &state,
                 const std::vector<ref<Expr>> &resolveConditions,
                 const std::vector<ref<Expr>> &unboundConditions,
                 ref<Expr> checkOutOfBounds, bool hasLazyInitialized,
                 ref<Expr> &guard, bool &mayBeInBounds);

  bool collectConcretizations(ExecutionState &state,
                              const std::vector<ref<Expr>> &resolveConditions,
                              const std::vector<ref<Expr>> &unboundConditions,
                              const std::vector<IDType> &resolvedMemoryObjects,
                              ref<Expr> checkOutOfBounds,
                              bool hasLazyInitialized, ref<Expr> &guard,
                              std::vector<Assignment> &resolveConcretizations,
                              bool &mayBeInBounds);

  void collectReads(ExecutionState &state, ref<Expr> address, KType *targetType,
                    Expr::Width type, unsigned bytes,
                    const std::vector<IDType> &resolvedMemoryObjects,
                    const std::vector<Assignment> &resolveConcretizations,
                    std::vector<ref<Expr>> &results);

  // do address resolution / object binding / out of bounds checking
  // and perform the operation
  void executeMemoryOperation(ExecutionState &state, bool isWrite,
                              KType *targetType, ref<Expr> address,
                              ref<Expr> value /* undef if read */,
                              KInstruction *target /* undef if write */);

  bool lazyInitializeObject(ExecutionState &state, ref<Expr> address,
                            const KInstruction *target, KType *targetType,
                            uint64_t size, bool isLocal, IDType &id,
                            bool isConstant = true);

  IDType lazyInitializeLocalObject(ExecutionState &state, StackFrame &sf,
                                   ref<Expr> address,
                                   const KInstruction *target);

  IDType lazyInitializeLocalObject(ExecutionState &state, ref<Expr> address,
                                   const KInstruction *target);

  void executeMakeSymbolic(ExecutionState &state, const MemoryObject *mo,
                           KType *type, const ref<SymbolicSource> source,
                           bool isLocal);

  void updateStateWithSymcretes(ExecutionState &state,
                                const Assignment &assignment);

  /// Assume that `current` state stepped to `block`.
  /// Can we reach at least one target of `current` from there?
  bool canReachSomeTargetFromBlock(ExecutionState &current, KBlock *block);

  /// Create a new state where each input condition has been added as
  /// a constraint and return the results. The input state is included
  /// as one of the results. Note that the output vector may include
  /// NULL pointers for states which were unable to be created.
  void branch(ExecutionState &state, const std::vector<ref<Expr>> &conditions,
              std::vector<ExecutionState *> &result, BranchType reason);

  /// Fork current and return states in which condition holds / does
  /// not hold, respectively. One of the states is necessarily the
  /// current state, and one of the states may be null.
  /// if ifTrueBlock == ifFalseBlock, then fork is internal
  StatePair fork(ExecutionState &current, ref<Expr> condition,
                 KBlock *ifTrueBlock, KBlock *ifFalseBlock, BranchType reason);
  StatePair forkInternal(ExecutionState &current, ref<Expr> condition,
                         BranchType reason);

  // If the MaxStatic*Pct limits have been reached, concretize the condition and
  // return it. Otherwise, return the unmodified condition.
  ref<Expr> maxStaticPctChecks(ExecutionState &current, ref<Expr> condition);

  Assignment computeConcretization(const ConstraintSet &constraints,
                                   ref<Expr> condition,
                                   SolverQueryMetaData &queryMetaData);

  /// Add the given (boolean) condition as a constraint on state. This
  /// function is a wrapper around the state's addConstraint function
  /// which also manages propagation of implied values,
  /// validity checks, and seed patching.
  void addConstraint(ExecutionState &state, ref<Expr> condition);

  // Called on [for now] concrete reads, replaces constant with a symbolic
  // Used for testing.
  ref<Expr> replaceReadWithSymbolic(ExecutionState &state, ref<Expr> e);

  ref<Expr> makeMockValue(ExecutionState &state, const std::string &name,
                          Expr::Width width);

  const Cell &eval(const KInstruction *ki, unsigned index,
                   ExecutionState &state, StackFrame &sf,
                   bool isSymbolic = true);

  ref<Expr> readArgument(ExecutionState &state, StackFrame &frame,
                         const KFunction *kf, unsigned index) {
    ref<Expr> arg = frame.locals[kf->getArgRegister(index)].value;
    if (!arg) {
      prepareSymbolicArg(state, frame, index);
    }
    return frame.locals[kf->getArgRegister(index)].value;
  }

  ref<Expr> readDest(ExecutionState &state, StackFrame &frame,
                     const KInstruction *target) {
    unsigned index = target->getDest();
    ref<Expr> reg = frame.locals[index].value;
    if (!reg) {
      prepareSymbolicRegister(state, frame, index);
    }
    return frame.locals[index].value;
  }

  Cell &getArgumentCell(const StackFrame &frame, const KFunction *kf,
                        unsigned index) {
    return frame.locals[kf->getArgRegister(index)];
  }

  Cell &getDestCell(const StackFrame &frame, const KInstruction *target) {
    return frame.locals[target->getDest()];
  }

  const Cell &eval(const KInstruction *ki, unsigned index,
                   ExecutionState &state, bool isSymbolic = true);

  Cell &getArgumentCell(const ExecutionState &state, const KFunction *kf,
                        unsigned index) {
    return getArgumentCell(state.stack.valueStack().back(), kf, index);
  }

  Cell &getDestCell(const ExecutionState &state, const KInstruction *target) {
    return getDestCell(state.stack.valueStack().back(), target);
  }

  void bindLocal(const KInstruction *target, StackFrame &frame,
                 ref<Expr> value);

  void bindArgument(KFunction *kf, unsigned index, StackFrame &frame,
                    ref<Expr> value);

  void bindLocal(const KInstruction *target, ExecutionState &state,
                 ref<Expr> value);

  void bindArgument(KFunction *kf, unsigned index, ExecutionState &state,
                    ref<Expr> value);

  /// Evaluates an LLVM constant expression.  The optional argument ki
  /// is the instruction where this constant was encountered, or NULL
  /// if not applicable/unavailable.
  ref<klee::ConstantExpr> evalConstantExpr(const llvm::ConstantExpr *c,
                                           llvm::APFloat::roundingMode rm,
                                           const KInstruction *ki = NULL);

  /// Evaluates an LLVM float comparison. the operands are two float
  /// expressions.
  ref<klee::Expr> evaluateFCmp(unsigned int predicate, ref<klee::Expr> left,
                               ref<klee::Expr> right) const;

  /// Evaluates an LLVM constant.  The optional argument ki is the
  /// instruction where this constant was encountered, or NULL if
  /// not applicable/unavailable.
  ref<klee::ConstantExpr> evalConstant(const llvm::Constant *c,
                                       llvm::APFloat::roundingMode rm,
                                       const KInstruction *ki = NULL);

  /// Return a unique constant value for the given expression in the
  /// given state, if it has one (i.e. it provably only has a single
  /// value). Otherwise return the original expression.
  ref<Expr> toUnique(const ExecutionState &state, ref<Expr> &e);

  /// Return a constant value for the given expression, forcing it to
  /// be constant in the given state by adding a constraint if
  /// necessary. Note that this function breaks completeness and
  /// should generally be avoided.
  ///
  /// \param purpose An identify string to printed in case of concretization.
  ref<klee::ConstantExpr> toConstant(ExecutionState &state, ref<Expr> e,
                                     const char *purpose);

  /// Bind a constant value for e to the given target. NOTE: This
  /// function may fork state if the state has multiple seeds.
  void executeGetValue(ExecutionState &state, ref<Expr> e,
                       KInstruction *target);

  /// Get textual information regarding a memory address.
  std::string getAddressInfo(ExecutionState &state, ref<Expr> address,
                             unsigned size = 0,
                             const MemoryObject *mo = nullptr) const;

  // Determines the \param lastInstruction of the \param state which is not KLEE
  // internal and returns its KInstruction
  const KInstruction *
  getLastNonKleeInternalInstruction(const ExecutionState &state);

  /// Remove state from queue and delete state
  void terminateState(ExecutionState &state,
                      StateTerminationType terminationType);

  /// Call exit handler and terminate state normally
  /// (end of execution path)
  void terminateStateOnExit(ExecutionState &state);

  /// Call exit handler and terminate state early
  /// (e.g. due to state merging or memory pressure)
  void terminateStateEarly(ExecutionState &state, const llvm::Twine &message,
                           StateTerminationType reason);

  /// Call exit handler and terminate state early
  /// (e.g. caused by the applied algorithm as in state merging or replaying)
  void terminateStateEarlyAlgorithm(ExecutionState &state,
                                    const llvm::Twine &message,
                                    StateTerminationType reason);

  /// Call exit handler and terminate state early
  /// (e.g. due to klee_silent_exit issued by user)
  void terminateStateEarlyUser(ExecutionState &state,
                               const llvm::Twine &message);

  /// Call error handler and terminate state in case of errors.
  /// The underlying function of all error-handling termination functions
  /// below. This function should only be used in the termination functions
  /// below.
  void terminateStateOnError(ExecutionState &state, const llvm::Twine &message,
                             StateTerminationType reason,
                             const llvm::Twine &longMessage = "",
                             const char *suffix = nullptr);

  void reportStateOnTargetError(ExecutionState &state, ReachWithError error);

  /// Save extra information in targeted mode
  /// Then just call `terminateStateOnError`
  void terminateStateOnTargetError(ExecutionState &state, ReachWithError error);

  /// Call error handler and terminate state in case of program errors
  /// (e.g. free()ing globals, out-of-bound accesses)
  void terminateStateOnProgramError(ExecutionState &state,
                                    const llvm::Twine &message,
                                    StateTerminationType reason,
                                    const llvm::Twine &longMessage = "",
                                    const char *suffix = nullptr);

  void terminateStateOnTerminator(ExecutionState &state);

  /// Call error handler and terminate state in case of execution errors
  /// (things that should not be possible, like illegal instruction or
  /// unlowered intrinsic, or unsupported intrinsics, like inline assembly)
  void terminateStateOnExecError(
      ExecutionState &state, const llvm::Twine &message,
      StateTerminationType = StateTerminationType::Execution);

  /// Call error handler and terminate state in case of solver errors
  /// (solver error or timeout)
  void terminateStateOnSolverError(ExecutionState &state,
                                   const llvm::Twine &message);

  /// Call error handler and terminate state for user errors
  /// (e.g. wrong usage of klee.h API)
  void terminateStateOnUserError(ExecutionState &state,
                                 const llvm::Twine &message);

  void reportProgressTowardsTargets(std::string prefix,
                                    const SetOfStates &states) const;
  void reportProgressTowardsTargets() const;

  /// bindModuleConstants - Initialize the module constant table.
  void bindModuleConstants(const llvm::APFloat::roundingMode &rm);

  uint64_t updateNameVersion(ExecutionState &state, const std::string &name);

  const Array *makeArray(ref<Expr> size, const ref<SymbolicSource> source);

  ExecutionState *prepareStateForPOSIX(KInstIterator &caller,
                                       ExecutionState *state);

  void prepareTargetedExecution(ExecutionState &initialState,
                                ref<TargetForest> whitelist);

  void increaseProgressVelocity(ExecutionState &state, KBlock *block);

  void decreaseConfidenceFromStoppedStates(
      SetOfStates &leftStates,
      HaltExecution::Reason reason = HaltExecution::NotHalt);

  void checkNullCheckAfterDeref(ref<Expr> cond, ExecutionState &state,
                                ExecutionState *fstate, ExecutionState *sstate);

  template <typename TypeIt>
  void computeOffsetsSeqTy(KGEPInstruction *kgepi,
                           ref<ConstantExpr> &constantOffset, uint64_t index,
                           const TypeIt it);

  template <typename TypeIt>
  void computeOffsets(KGEPInstruction *kgepi, TypeIt ib, TypeIt ie);

  /// bindInstructionConstants - Initialize any necessary per instruction
  /// constant values.
  void bindInstructionConstants(KInstruction *KI);

  void doImpliedValueConcretization(ExecutionState &state, ref<Expr> e,
                                    ref<ConstantExpr> value);

  /// check memory usage and terminate states when over threshold of -max-memory
  /// + 100MB \return true if below threshold, false otherwise (states were
  /// terminated)
  bool checkMemoryUsage();

  /// check if branching/forking into N branches is allowed
  bool branchingPermitted(ExecutionState &state, unsigned N);

  void printDebugInstructions(ExecutionState &state);
  void doDumpStates();

  /// Only for debug purposes; enable via debugger or klee-control
  void dumpStates();
  void dumpPForest();

  const KInstruction *getKInst(const llvm::Instruction *ints) const;
  const KBlock *getKBlock(const llvm::BasicBlock *bb) const;
  const KFunction *getKFunction(const llvm::Function *f) const;

public:
  Executor(llvm::LLVMContext &ctx, const InterpreterOptions &opts,
           InterpreterHandler *ie);
  virtual ~Executor();

  const InterpreterHandler &getHandler() { return *interpreterHandler; }

  void setPathWriter(TreeStreamWriter *tsw) override { pathWriter = tsw; }

  bool hasTargetForest() const override { return !targets.empty(); }

  void setSymbolicPathWriter(TreeStreamWriter *tsw) override {
    symPathWriter = tsw;
  }

  void setReplayKTest(const struct KTest *out) override {
    assert(!replayPath && "cannot replay both buffer and path");
    replayKTest = out;
    replayPosition = 0;
  }

  void setReplayPath(const std::vector<bool> *path) override {
    assert(!replayKTest && "cannot replay both buffer and path");
    replayPath = path;
    replayPosition = 0;
  }

  llvm::Module *
  setModule(std::vector<std::unique_ptr<llvm::Module>> &userModules,
            std::vector<std::unique_ptr<llvm::Module>> &libsModules,
            const ModuleOptions &opts,
            std::set<std::string> &&mainModuleFunctions,
            std::set<std::string> &&mainModuleGlobals,
            FLCtoOpcode &&origInstructions) override;

  void useSeeds(const std::vector<struct KTest *> *seeds) override {
    usingSeeds = seeds;
  }

  ExecutionState *formState(llvm::Function *f);
  ExecutionState *formState(llvm::Function *f, int argc, char **argv,
                            char **envp);

  void clearGlobal();

  void clearMemory();

  void runFunctionAsMain(llvm::Function *f, int argc, char **argv,
                         char **envp) override;

  /*** Isolated execution ***/

  ref<Expr> makeSymbolicValue(llvm::Value *value, ExecutionState &state);

  void prepareSymbolicValue(ExecutionState &state, StackFrame &frame,
                            KInstruction *target);
  void prepareSymbolicValue(ExecutionState &state, KInstruction *target);

  void prepareMockValue(ExecutionState &state, StackFrame &frame,
                        const std::string &name, Expr::Width width,
                        KInstruction *target);

  void prepareMockValue(ExecutionState &state, const std::string &name,
                        KInstruction *target);

  void prepareSymbolicRegister(ExecutionState &state, StackFrame &frame,
                               unsigned index);

  void prepareSymbolicArg(ExecutionState &state, StackFrame &frame,
                          unsigned index);

  void prepareSymbolicArgs(ExecutionState &state, StackFrame &frame);
  void prepareSymbolicArgs(ExecutionState &state);

  /*** Runtime options ***/

  void setHaltExecution(HaltExecution::Reason value) override {
    haltExecution = value;
  }

  void setInhibitForking(bool value) override { inhibitForking = value; }

  void prepareForEarlyExit() override;

  /*** State accessor methods ***/

  unsigned getPathStreamID(const ExecutionState &state) override;

  unsigned getSymbolicPathStreamID(const ExecutionState &state) override;

  void
  getConstraintLog(const ExecutionState &state, std::string &res,
                   Interpreter::LogType logFormat = Interpreter::STP) override;

  void setInitializationGraph(const ExecutionState &state,
                              const std::vector<klee::Symbolic> &symbolics,
                              const Assignment &model, KTest &tc);

  void logState(const ExecutionState &state, int id,
                std::unique_ptr<llvm::raw_fd_ostream> &f) override;

  bool getSymbolicSolution(const ExecutionState &state, KTest &res) override;

  void getCoveredLines(const ExecutionState &state,
                       std::map<std::string, std::set<unsigned>> &res) override;

  void getBlockPath(const ExecutionState &state,
                    std::string &blockPath) override;

  Expr::Width getWidthForLLVMType(llvm::Type *type) const;
  size_t getAllocationAlignment(const llvm::Value *allocSite) const;

  /// Returns the errno location in memory of the state
  int *getErrnoLocation(const ExecutionState &state) const;

  void executeStep(ExecutionState &state);
};

} // namespace klee

#endif /* KLEE_EXECUTOR_H */
