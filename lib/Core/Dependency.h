//===--- Dependency.h - Memory location dependency --------------*- C++ -*-===//
//
//               The Tracer-X KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declarations for the dependency analysis to compute
/// the memory locations upon which the unsatisfiability core depends, which is
/// used in computing the interpolant.
///
//===----------------------------------------------------------------------===//

#ifndef KLEE_DEPENDENCY_H
#define KLEE_DEPENDENCY_H

#include "klee/Config/Version.h"
#include "klee/Internal/Module/TxValues.h"

#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 3)
#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Value.h>
#else
#include <llvm/Function.h>
#include <llvm/Instruction.h>
#include <llvm/Instructions.h>
#include <llvm/Value.h>
#endif

#include "llvm/Support/raw_ostream.h"

#include <vector>

namespace klee {

  /// \brief Computation of memory regions the unsatisfiability core depends
  /// upon, which is used to compute the interpolant stored in the table.
  ///
  /// <b>Problem statement</b>
  ///
  /// Memory regions of program states upon which the unsatisfiability
  /// core depends need to be computed. These regions are represented
  /// neither on the path condition nor the unsatisfiability core,
  /// which is just the subset of the path condition. More precisely,
  /// at a particular point \f$p\f$ of the symbolic execution, we have
  /// a program state that can be represented as \f$x_p\f$, which is a
  /// mapping of memory regions (e.g., variables) to their
  /// values. Within the KLEE implementation, constraints on the path
  /// condition always constrain only the initial symbolic values, for
  /// example, the constraint \f$c(f(x_0))\f$, where \f$x_0\f$
  /// represents the initial state, and \f$f\f$ (a function with
  /// program state as its domain and codomain) represents the state
  /// update from the initial state to the point where \f$c\f$ is
  /// introduced via e.g., branch condition. The problem here is how
  /// to relate \f$c(f(x_0))\f$ with \f$x_p\f$ such the constraints
  /// can be used to constrain \f$x_p\f$, which is the state at an
  /// arbitrary point \f$p\f$ in the execution.
  ///
  /// <b>Solution</b>
  ///
  /// When \f$g\f$ is a function with program state as its domain and
  /// codomain represents all the state updates from the initial state
  /// \f$x_0\f$ to the point \f$p\f$, then \f$c(f(x_0))\f$ constrains
  /// \f$x_p\f$ in the following way:
  /// \f[
  /// \exists x_0 ~.~ c(f(x_0)) \wedge x_p = g(x_0)
  /// \f]
  /// This is indeed the specification of an already-visited
  /// intermediate state at stage \f$p\f$ of the execution that can be
  /// used to subsume other states. We could store the whole of
  /// \f$x_p\f$ as an interpolant, however, we would not gain much
  /// subsumptions in this way, as only a subset of \f$x_p\f$ is
  /// relevant for \f$c(f(x_0))\f$. Other components of \f$x_p\f$
  /// would provide extra constraints that fail subsumption.
  ///
  /// Here we note that the function \f$f\f$ can be composed of
  /// functions \f$g\f$ and \f$h\f$ such that \f$f = h \cdot g\f$, and
  /// therefore \f$c(f(x_0))\f$ is equivalent to
  /// \f$c(h(g(x_0)))\f$. Here, only a subset of \f$g\f$ and \f$h\f$
  /// are actually relevant to \f$c\f$. Call these \f$g'\f$ and
  /// \f$h'\f$ respectively, where \f$c(h'(g'(x_0)))\f$ is equivalent
  /// to \f$c(h(g(x_0)))\f$, and therefore is also equivalent to
  /// \f$c(f(x_0))\f$. Now, instead of the above equation, the
  /// following alternative provides a specification that constrains
  /// only a subset \f$x_p'\f$ of \f$x_p\f$:
  /// \f[
  /// \exists x_0 ~.~ c(h'(g'(x_0))) \wedge x_p' = g'(x_0)
  /// \f]
  /// or simply:
  /// \f[
  /// \exists x_0 ~.~ c(f(x_0)) \wedge x_p' = g'(x_0)
  /// \f]
  /// The essence of the dependency computation implemented by this
  /// class is the computation of the <b>domain</b> of \f$h'\f$, which
  /// represents all the mappings relevant to the constraint
  /// \f$c(f(x_0))\f$. For this we build \f$h'\f$ utilizing the flow
  /// dependency relation from stage \f$p\f$ to the point of
  /// introduction of the constraint \f$c(f(x_0))\f$, such that when
  /// \f$c(f(x_0))\f$ is found to be in the core, we know which subset
  /// \f$x_p'\f$ of \f$x_p\f$ that are relevant based on the computed
  /// domain of \f$h'\f$.
  ///
  /// <b>Data types</b>
  ///
  /// In the LLVM language, a <i>state</i> is a mapping of memory
  /// locations to the values stored in them. <i>State update</i> is
  /// the loading of values from the memory locations, their
  /// manipulation, and the subsequent storing of their values into
  /// memory locations. The memory dependency computation to compute
  /// the domain of \f$h'\f$ is based on shadow data structure with
  /// the following main components:
  ///
/// - TxStateValue: LLVM values (i.e., variables) with versioning
  ///   index. This represents the values loaded from memory into LLVM
  ///   temporary variables. They have versioning index, as, different
  ///   from LLVM values themselves which are static entities, a
  ///   (symbolic) execution may go through the same instruction
  ///   multiple times. Hence the value of that instruction has to be
  ///   versioned.
/// - TxStateAddress: A representation of pointers. It is important
  ///   to note that each pointer is associated with memory allocation
  ///   and its displacement (offset) wrt. the base address of the
  ///   allocation.
  ///
  /// The results of the computation is stored in several member
  /// variables as follows, mainly Dependency#concretelyAddressedStore and
  /// Dependency#symbolicallyAddressedStore which represent the components of
  /// the state associated with the owner TxTreeNode object of the Dependency
  /// object. Dependency#concretelyAddressedStore is the part of the state that
  /// are concretely addressed, whereas Dependency#symbolicallyAddressedStore is
  /// the part that is symbolically addressed.
  ///
  /// <b>Notes on pointer flow propagation</b>
  ///
/// A TxStateValue object may represent a pointer value, in which
/// case it is linked to possibly several TxStateAddress objects via
/// TxStateValue#locations member variable. Such TxStateValue
  /// object may be used in memory access operations of LLVM
  /// (<b>load</b> or <b>store</b>). The memory dependency computation
/// propagates such pointer value information in TxStateAddress from
/// one TxStateValue to another such that there is no need to
  /// inefficiently hunt for the pointer value at the point of use of
  /// the pointer. For example, a symbolic execution of LLVM's
  /// <b>getelementptr</b> instruction would create a new
/// TxStateValue representing the return value of the
/// instruction. This new TxStateValue would inherit all members
/// of the TxStateValue#locations variable of the TxStateValue
  /// object representing the pointer argument of the instruction,
  /// with modified offsets according to the offset argument of the
  /// instruction.
  ///
  /// \see TxTree
  /// \see TxTreeNode
/// \see TxStateValue
/// \see TxStateAddress
  class Dependency {

  public:
    typedef std::map<ref<TxInterpolantAddress>, ref<TxInterpolantValue> >
    InterpolantStoreMap;
    typedef std::map<const llvm::Value *, InterpolantStoreMap> InterpolantStore;

  private:
    /// \brief Previous path condition
    Dependency *parent;

    /// \brief Argument values to be passed onto callee
    std::vector<ref<TxStateValue> > argumentValuesList;

    /// \brief The mapping of concrete locations to stored value
    std::map<ref<TxStateAddress>,
             std::pair<ref<TxStateValue>, ref<TxStateValue> > >
    concretelyAddressedStore;

    /// \brief Ordered keys of the concretely-addressed store.
    std::vector<ref<TxStateAddress> > concretelyAddressedStoreKeys;

    /// \brief The mapping of symbolic locations to stored value
    std::map<ref<TxStateAddress>,
             std::pair<ref<TxStateValue>, ref<TxStateValue> > >
    symbolicallyAddressedStore;

    /// \brief Ordered keys of the symbolically-addressed store.
    std::vector<ref<TxStateAddress> > symbolicallyAddressedStoreKeys;

    /// \brief The store of the versioned values
    std::map<llvm::Value *, std::vector<ref<TxStateValue> > > valuesMap;

    /// \brief Locations of this node and its ancestors that are needed for
    /// the core and dominates other locations.
    std::set<ref<TxStateAddress> > coreLocations;

    /// \brief The data layout of the analysis target program
    llvm::DataLayout *targetData;

    /// \brief Tests if a pointer points to a main function's argument
    static bool isMainArgument(const llvm::Value *loc);

    /// \brief Register new versioned value, used by getNewTxStateValue
    /// member functions
    ref<TxStateValue> registerNewTxStateValue(llvm::Value *value,
                                              ref<TxStateValue> vvalue);

    /// \brief Create a new versioned value object, typically when executing a
    /// new instruction, as a value for the instruction.
    ref<TxStateValue>
    getNewTxStateValue(llvm::Value *value,
                       const std::vector<llvm::Instruction *> &callHistory,
                       ref<Expr> valueExpr) {
      return registerNewTxStateValue(
          value, TxStateValue::create(value, callHistory, valueExpr));
    }

    /// \brief Create a new versioned value object, which is a pointer with
    /// absolute address
    ref<TxStateValue>
    getNewPointerValue(llvm::Value *loc,
                       const std::vector<llvm::Instruction *> &callHistory,
                       ref<Expr> address, uint64_t size) {
      ref<TxStateValue> vvalue =
          TxStateValue::create(loc, callHistory, address);
      vvalue->addLocation(
          TxStateAddress::create(loc, callHistory, address, size));
      return registerNewTxStateValue(loc, vvalue);
    }

    /// \brief Create a new versioned value object, which is a pointer which
    /// offsets existing pointer
    ref<TxStateValue> getNewPointerValue(
        llvm::Value *value, const std::vector<llvm::Instruction *> &callHistory,
        ref<Expr> address, ref<TxStateAddress> loc, ref<Expr> offset) {
      ref<TxStateValue> vvalue =
          TxStateValue::create(value, callHistory, address);
      vvalue->addLocation(TxStateAddress::create(loc, address, offset));
      return registerNewTxStateValue(value, vvalue);
    }

    /// \brief Gets the latest version of the location, but without checking
    /// for whether the value is constant or not.
    ref<TxStateValue> getLatestValueNoConstantCheck(llvm::Value *value,
                                                    ref<Expr> expr);

    /// \brief Gets the latest pointer value for marking
    ref<TxStateValue> getLatestValueForMarking(llvm::Value *val,
                                               ref<Expr> expr);

    /// \brief Newly relate a location with its stored value, when the value is
    /// loaded from the location
    void updateStoreWithLoadedValue(ref<TxStateAddress> loc,
                                    ref<TxStateValue> address,
                                    ref<TxStateValue> value);

    /// \brief Newly relate an location with its stored value
    void updateStore(ref<TxStateAddress> loc, ref<TxStateValue> address,
                     ref<TxStateValue> value);

    /// \brief Add flow dependency between source and target value
    void addDependency(ref<TxStateValue> source, ref<TxStateValue> target,
                       bool multiLocationsCheck = true);

    /// \brief Add flow dependency between source and target value
    void addDependencyIntToPtr(ref<TxStateValue> source,
                               ref<TxStateValue> target);

    /// \brief Add flow dependency between source and target pointers, offset by
    /// some amount
    void addDependencyWithOffset(ref<TxStateValue> source,
                                 ref<TxStateValue> target,
                                 ref<Expr> offsetDelta);

    /// \brief Add flow dependency between source and target value, as the
    /// result of store/load via a memory location.
    void addDependencyViaLocation(ref<TxStateValue> source,
                                  ref<TxStateValue> target,
                                  ref<TxStateAddress> via);

    /// \brief Add a flow dependency from a pointer value to a non-pointer
    /// value, for an external function call.
    ///
    /// Here the target is not a pointer, and we assume that the source is
    /// is checked for memory access validity at the current index, meaning that
    /// we assumed all memory access within the external function is valid.
    void addDependencyViaExternalFunction(
        const std::vector<llvm::Instruction *> &callHistory,
        ref<TxStateValue> source, ref<TxStateValue> target);

    /// \brief Add a flow dependency from a pointer value to a non-pointer
    /// value.
    void addDependencyToNonPointer(ref<TxStateValue> source,
                                   ref<TxStateValue> target);

    /// \brief All values that flows to the target in one step
    std::vector<ref<TxStateValue> >
    directFlowSources(ref<TxStateValue> target) const;

    /// \brief Mark as core all the values and locations that flows to the
    /// target
    void markFlow(ref<TxStateValue> target, const std::string &reason,
                  bool incrementDirectUseCount = true) const;

    /// \brief Mark as core all the pointer values and that flows to the target;
    /// and adjust its offset bound for memory bounds interpolation (a.k.a.
    /// slackening)
    void markPointerFlow(ref<TxStateValue> target,
                         ref<TxStateValue> checkedOffset,
                         const std::string &reason) const {
      std::set<ref<Expr> > bounds;
      markPointerFlow(target, checkedOffset, bounds, reason);
    }

    /// \brief Mark as core all the pointer values and that flows to the target;
    /// and adjust its offset bound for memory bounds interpolation (a.k.a.
    /// slackening)
    void markPointerFlow(ref<TxStateValue> target,
                         ref<TxStateValue> checkedOffset,
                         std::set<ref<Expr> > &bounds,
                         const std::string &reason,
                         bool incrementUseCount = true) const;

    /// \brief Record the expressions of a call's arguments
    void populateArgumentValuesList(
        llvm::CallInst *site,
        const std::vector<llvm::Instruction *> &callHistory,
        std::vector<ref<Expr> > &arguments,
        std::vector<ref<TxStateValue> > &argumentValuesList);

    void removeAddressValue(
        std::map<ref<TxStateAddress>, ref<TxStateValue> > &simpleStore,
        Dependency::InterpolantStore &concreteStore,
        std::set<const Array *> &replacements, bool coreOnly) const;

    void getConcreteStore(
        const std::vector<llvm::Instruction *> &callHistory,
        const std::map<ref<TxStateAddress>,
                       std::pair<ref<TxStateValue>, ref<TxStateValue> > > &
            store,
        const std::vector<ref<TxStateAddress> > &orderedStoreKeys,
        std::set<const Array *> &replacements, bool coreOnly,
        Dependency::InterpolantStore &concreteStore) const;

    void getSymbolicStore(
        const std::vector<llvm::Instruction *> &callHistory,
        const std::map<ref<TxStateAddress>,
                       std::pair<ref<TxStateValue>, ref<TxStateValue> > > &
            store,
        const std::vector<ref<TxStateAddress> > &orderedStoreKeys,
        std::set<const Array *> &replacements, bool coreOnly,
        Dependency::InterpolantStore &symbolicStore) const;

  public:
    /// \brief This is for dynamic setting up of debug messages.
    int debugSubsumptionLevel;

    /// \brief Flag to display debug information on the state.
    uint64_t debugStateLevel;

    Dependency(Dependency *parent, llvm::DataLayout *_targetData);

    ~Dependency();

    Dependency *cdr() const;

    ref<TxStateValue>
    getLatestValue(llvm::Value *value,
                   const std::vector<llvm::Instruction *> &callHistory,
                   ref<Expr> valueExpr, bool constraint = false);

    /// \brief Abstract dependency state transition with argument(s)
    void execute(llvm::Instruction *instr,
                 const std::vector<llvm::Instruction *> &callHistory,
                 std::vector<ref<Expr> > &args, bool symbolicExecutionError);

    /// \brief Execution of klee_make_symbolic
    void
    executeMakeSymbolic(llvm::Instruction *instr,
                        const std::vector<llvm::Instruction *> &callHistory,
                        ref<Expr> address, const Array *array);

    /// \brief Build dependencies from PHI node
    void executePHI(llvm::Instruction *instr, unsigned int incomingBlock,
                    const std::vector<llvm::Instruction *> &callHistory,
                    ref<Expr> valueExpr, bool symbolicExecutionError);

    /// \brief Execute memory operation (load/store)
    void
    executeMemoryOperation(llvm::Instruction *instr,
                           const std::vector<llvm::Instruction *> &callHistory,
                           std::vector<ref<Expr> > &args, bool inBounds,
                           bool symbolicExecutionError);

    /// \brief This retrieves the locations known at this state, and the
    /// expressions stored in the locations. Returns as the last argument a pair
    /// of the store part indexed by constants, and the store part indexed by
    /// symbolic expressions.
    ///
    /// \param replacements The replacement bound variables when
    /// retrieving state for creating subsumption table entry: As the
    /// resulting expression will be used for storing in the
    /// subsumption table, the variables need to be replaced with the
    /// bound ones.
    /// \param coreOnly Indicate whether we are retrieving only data
    /// for locations relevant to an unsatisfiability core.
    void
    getStoredExpressions(const std::vector<llvm::Instruction *> &callHistory,
                         std::set<const Array *> &replacements, bool coreOnly,
                         InterpolantStore &_concretelyAddressedStore,
                         InterpolantStore &_symbolicallyAddressedStore);

    /// \brief Record call arguments in a function call
    void bindCallArguments(llvm::Instruction *instr,
                           std::vector<llvm::Instruction *> &callHistory,
                           std::vector<ref<Expr> > &arguments);

    /// \brief This propagates the dependency due to the return value of a call
    void bindReturnValue(llvm::CallInst *site,
                         std::vector<llvm::Instruction *> &callHistory,
                         llvm::Instruction *inst, ref<Expr> returnValue);

    /// \brief Given a versioned value, retrieve all its sources and mark them
    /// as in the core.
    void markAllValues(ref<TxStateValue> value, const std::string &reason);

    /// \brief Given an LLVM value, retrieve all its sources and mark them as in
    /// the core.
    void markAllValues(llvm::Value *value, ref<Expr> expr,
                       const std::string &reason);

    /// \brief Given an LLVM value which is used as an address, retrieve all its
    /// sources and mark them as in the core.
    void markAllPointerValues(llvm::Value *val, ref<Expr> address,
                              const std::string &reason) {
      std::set<ref<Expr> > bounds;
      markAllPointerValues(val, address, bounds, reason);
    }

    /// \brief Given an LLVM value which is used as an address, retrieve all its
    /// sources and mark them as in the core.
    void markAllPointerValues(llvm::Value *val, ref<Expr> address,
                              std::set<ref<Expr> > &bounds,
                              const std::string &reason);

    /// \brief Print the content of the object to the LLVM error stream
    void dump() const {
      this->print(llvm::errs());
      llvm::errs() << "\n";
    }

    /// \brief Print the content of the object into a stream.
    ///
    /// \param stream The stream to print the data to.
    void print(llvm::raw_ostream& stream) const;

    /// \brief Print the content of the object into a stream.
    ///
    /// \param stream The stream to print the data to.
    /// \param paddingAmount The number of whitespaces to be printed before each
    /// line.
    void print(llvm::raw_ostream &stream, const unsigned paddingAmount) const;
  };

}

#endif
