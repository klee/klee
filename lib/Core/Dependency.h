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
/// This file contains the declarations for the flow-insensitive dependency
/// analysis to compute the memory locations upon which the unsatisfiability
/// core depends, which is used in computing the interpolant.
///
//===----------------------------------------------------------------------===//

#ifndef KLEE_DEPENDENCY_H
#define KLEE_DEPENDENCY_H

#include "AddressSpace.h"
#include "klee/Config/Version.h"

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
#include <stack>

namespace klee {

class Dependency;

/// \brief Implements the replacement mechanism for replacing variables, used in
/// replacing free with bound variables.
class ShadowArray {
  static std::map<const Array *, const Array *> shadowArray;

  static UpdateNode *getShadowUpdate(const UpdateNode *chain,
                                     std::set<const Array *> &replacements);

public:
  static ref<Expr> createBinaryOfSameKind(ref<Expr> originalExpr,
                                          ref<Expr> newLhs, ref<Expr> newRhs);

  static void addShadowArrayMap(const Array *source, const Array *target);

  static ref<Expr> getShadowExpression(ref<Expr> expr,
                                       std::set<const Array *> &replacements);

  static std::string getShadowName(std::string name) {
    return "__shadow__" + name;
  }
};

/// \brief A class to represent memory locations.
class MemoryLocation {

public:
  unsigned refCount;

private:
    bool core;

    /// \brief The location's LLVM value
    llvm::Value *loc;

    /// \brief The absolute address
    ref<Expr> address;

    /// \brief The base address
    ref<Expr> base;

    /// \brief The offset of the allocation
    ref<Expr> offset;

    MemoryLocation(llvm::Value *_loc, ref<Expr> &_address, ref<Expr> &_base,
                   ref<Expr> &_offset)
        : refCount(0), core(false), loc(_loc), address(_address), base(_base),
          offset(_offset) {}

  public:
    ~MemoryLocation() {}

    static ref<MemoryLocation> create(llvm::Value *loc, ref<Expr> &address) {
      ref<Expr> zeroPointer = Expr::createPointer(0);
      ref<MemoryLocation> ret(
          new MemoryLocation(loc, address, address, zeroPointer));
      return ret;
    }

    static ref<MemoryLocation> create(ref<MemoryLocation> loc,
                                      ref<Expr> &address, ref<Expr> &offset) {
      ConstantExpr *c = llvm::dyn_cast<ConstantExpr>(offset);
      if (c->getZExtValue() == 0) {
        ref<Expr> base = loc->getBase();
        ref<Expr> offset = loc->getOffset();
        ref<MemoryLocation> ret(
            new MemoryLocation(loc->getValue(), address, base, offset));
        return ret;
      }

      ref<Expr> base = loc->getBase();
      ref<Expr> newOffset = AddExpr::create(loc->getOffset(), offset);
      ref<MemoryLocation> ret(
          new MemoryLocation(loc->getValue(), address, base, newOffset));
      return ret;
    }

    int compare(const MemoryLocation other) const {
      uint64_t l = reinterpret_cast<uint64_t>(loc),
               r = reinterpret_cast<uint64_t>(other.loc);

      if (l == r) {
        ConstantExpr *lc = llvm::dyn_cast<ConstantExpr>(address);
        if (lc) {
          ConstantExpr *rc = llvm::dyn_cast<ConstantExpr>(other.address);
          if (rc) {
            l = lc->getZExtValue();
            r = rc->getZExtValue();
            if (l == r)
              return 0;
            if (l < r)
              return -1;
            return 1;
          }
        }
        l = reinterpret_cast<uint64_t>(address.get());
        r = reinterpret_cast<uint64_t>(address.get());
        if (l == r)
          return 0;
        if (l < r)
          return -1;
        return 1;
      } else if (l < r)
        return -1;
      return 1;
    }

    bool hasConstantAddress() const { return llvm::isa<ConstantExpr>(address); }

    uint64_t getUIntAddress() const {
      return llvm::dyn_cast<ConstantExpr>(address)->getZExtValue();
    }

    llvm::Value *getValue() const { return loc; }

    ref<Expr> getAddress() const { return address; }

    ref<Expr> getBase() const { return base; }

    ref<Expr> getOffset() const { return offset; }

    void setAsCore() { core = true; }

    bool isCore() { return core; }

    /// \brief Print the content of the object to the LLVM error stream
    void dump() const {
      print(llvm::errs());
      llvm::errs() << "\n";
    }

    /// \brief Print the content of the object into a stream.
    ///
    /// \param The stream to print the data to.
    void print(llvm::raw_ostream& stream) const;
  };

  /// \brief A class that represents LLVM value that can be destructively
  /// updated (versioned).
  class VersionedValue {

    llvm::Value *value;

    const ref<Expr> valueExpr;

    /// \brief Set of memory locations possibly being pointed to
    std::set<ref<MemoryLocation> > locations;

    /// \brief Field to indicate if any unsatisfiability core depends on this
    /// value.
    bool core;

  public:
    VersionedValue(llvm::Value *value, ref<Expr> valueExpr)
        : value(value), valueExpr(valueExpr), core(false) {}

    ~VersionedValue() { locations.clear(); }

    void addLocation(ref<MemoryLocation> loc) { locations.insert(loc); }

    std::set<ref<MemoryLocation> > getLocations() { return locations; }

    bool hasValue(llvm::Value *value) const { return this->value == value; }

    ref<Expr> getExpression() const { return valueExpr; }

    void setAsCore() {
      core = true;
      for (std::set<ref<MemoryLocation> >::iterator it = locations.begin(),
                                                    ie = locations.end();
           it != ie; ++it) {
        (*it)->setAsCore();
      }
    }

    bool isCore() const { return core; }

    llvm::Value *getValue() const { return value; }

    /// \brief Print the content of the object into a stream.
    ///
    /// \param The stream to print the data to.
    void print(llvm::raw_ostream& stream) const;

    /// \brief Print the content of the object to the LLVM error stream
    void dump() const {
      print(llvm::errs());
      llvm::errs() << "\n";
    }
  };

  /// \brief Implementation of value dependency for computing locations the
  /// unsatisfiability core depends upon, which is used to compute the
  /// interpolant.
  ///
  /// Following is the analysis rules to compute value dependency relations
  /// useful for computing the interpolant. Given a finite symbolic execution
  /// path, the computation of the relations terminates. The analysis rules
  /// serve as a guide to the implementation.
  ///
  /// Problems solved:
  /// 1. Components of program states upon which the unsatisfiability core
  ///    depends need to be computed. These components may not be represented
  ///    in the constraints.
  /// 2. To gain more subsumption, we need to store interpolation at more
  ///    program points. More specifically, here we would like to compute the
  ///    instructions that are related to the unsatisfiability core in order
  ///    to compute the right interpolant. That is, given a constraint c(x0)
  ///    in the core, we want to compute the set of state update statements S
  ///    from which we compose the state update function f_S where the next
  ///    state x' = f_S(x0), such that the interpolant after the state update
  ///    is exists x0 . c(x0) /\ x' = f_S(x0)
  ///
  /// Solution:
  /// The dependency computation is based on shadow data structure
  /// representing the following:
  ///
  /// Domains:
  /// VersionedValue -> LLVM values (i.e., variables) with versioning index
  /// VersionedLocation -> Memory locations with versioning index
  ///
  /// Basic Relations:
  /// stores(VersionedLocation, VersionedValue) - Memory state
  /// depends(VersionedValue, VersionedValue) - Value dependency: The output
  ///    of the analysis.
  /// equals(VersionedValue, VersionedLocation) - Pointer value equality
  ///
  /// Derived Relations:
  /// Transitive Closure of depends
  ///
  /// depends*(v, v)
  /// depends*(v, v') /\ v != v' iff depends(v, v') /\ depends*(v', v'')
  ///
  /// Indirection Relation
  ///
  /// ind(v, m, 0) iff depends*(v, v') /\ equals(v', m)
  /// ind(v, m, i) /\ i>=1 iff
  ///    depends*(v, v') /\ stores(v'', v') /\ ind(v'', m, i-1)
  ///
  /// In the following abstract operational semantics of LLVM instructions,
  /// R and R' represent the abstract states before and after the execution.
  /// An abstract state is a set having as elements ground substitutions of
  /// the above relations. Below, v and its primed versions represent
  /// VersionedValue elements whereas m and its primed versions represent
  /// VersionedLocation elements.
  ///
  /// Location: v = alloca
  ///
  /// ---------------------------------------------------
  /// R --> R U {equals(succ(v), m) | R |/- equals(_, m)}
  ///
  /// Here succ(v) denotes the next (new) version of v.
  ///
  /// Store: store v', v
  ///
  /// ----------------------------------------------------
  /// R --> R U { stores(succ(m),v) | R |- ind(v', m, 0) }
  ///
  /// Here we use succ(m) to denote the next version of m as this was a
  /// destructive update.
  ///
  /// -------------------------------------------------------------
  /// R --> R U { stores(succ(ind(m,i)), v) | R |- ind(v', m, i), i > 0 }
  ///
  /// Here ind(m,i) is an abstract memory location representing any
  /// memory location that is i-step-reachable via indirection from m.
  ///
  /// R |/- ind(v, _, _)
  /// --------------------------
  /// R --> R U {stores(UNK, v)}
  ///
  /// Here UNK represents an unknown memory location. We assume that
  /// UNK cannot be versioned (non-destructive update applies to it).
  ///
  /// Load: v = load v'
  ///
  /// Here the rules are not mutually exclusive such that we avoid using set
  /// union to denote abstract states after the execution.
  ///
  /// R |- ind(v', latest(m), 0) /\ stores(latest(m), v''')
  /// R' |- depends(succ(v), v''')
  /// -----------------------------------------------------
  /// R --> R'
  ///
  /// Here latest(m) is only the latest version of Location m.
  ///
  /// R |- ind(v', m, i) /\ i > 0 /\ stores(m, v''')
  /// R' |- depends(succ(v), v''')
  /// ----------------------------------------------
  /// R --> R'
  ///
  /// R |/- ind(v', _, _)          R' |- stores(UNK, succ(v))
  /// -------------------------------------------------------
  /// R --> R'
  ///
  /// R |- stores(UNK, v'')                R' |- depends(v, v'')
  /// ----------------------------------------------------------
  /// R --> R'
  ///
  /// Here, any stores to an unknown address would be loaded.
  ///
  /// Getelementptr: v = getelementptr v', idx
  ///
  /// --------------------------------
  /// R --> R U {depends(succ(v), v')}
  ///
  /// Unary Operation: v = UNARY_OP(v') (including binary operation with 1
  /// constant argument)
  ///
  /// --------------------------------
  /// R --> R U {depends(succ(v), v')}
  ///
  /// Binary Operation: v = BINARY_OP(v', v'')
  ///
  /// -------------------------------------------------------
  /// R --> R U {depends(succ(v), v'), depends(succ(v), v'')}
  ///
  /// Phi Node: v = PHI(v'1, ..., v'n)
  ///
  /// -------------------------------------------------------------
  /// R --> R U {depends(succ(v), v'1), ..., depends(succ(v), v'n)}
  ///
  class Dependency {

  public:
    typedef std::pair<ref<Expr>, ref<Expr> > AddressValuePair;
    typedef std::map<uint64_t, AddressValuePair> ConcreteStoreMap;
    typedef std::vector<AddressValuePair> SymbolicStoreMap;
    typedef std::map<llvm::Value *, ConcreteStoreMap> ConcreteStore;
    typedef std::map<llvm::Value *, SymbolicStoreMap> SymbolicStore;

    class Util {

    public:
      template <typename K, typename T>
      static void deletePointerMap(std::map<K, T *> &map);

      template <typename K, typename T>
      static void
      deletePointerMapWithVectorValue(std::map<K *, std::vector<T> > &map);

      template <typename K, typename T>
      static void
      deletePointerMapWithMapValue(std::map<K *, std::map<K *, T> > &map);

      /// \brief Tests if a pointer points to a main function's argument
      static bool isMainArgument(llvm::Value *loc);
    };

  private:
    /// \brief Previous path condition
    Dependency *parent;

    /// \brief Argument values to be passed onto callee
    std::vector<VersionedValue *> argumentValuesList;

    /// \brief The mapping of locations to stored value
    std::map<ref<MemoryLocation>, VersionedValue *> storesMap;

    /// \brief Flow relations of target and its sources with location
    std::map<VersionedValue *,
             std::map<VersionedValue *, ref<MemoryLocation> > > flowsToMap;

    /// \brief The store of the versioned values
    std::map<llvm::Value *, std::vector<VersionedValue *> > valuesMap;

    /// \brief Locations of this node and its ancestors that are needed for
    /// the core and dominates other locations.
    std::set<ref<MemoryLocation> > coreLocations;

    /// \brief Register new versioned value, used by getNewVersionedValue
    /// methods
    VersionedValue *registerNewVersionedValue(llvm::Value *value,
                                              VersionedValue *vvalue);

    /// \brief Create a new versioned value object, typically when executing a
    /// new instruction, as a value for the instruction.
    VersionedValue *getNewVersionedValue(llvm::Value *value,
                                         ref<Expr> valueExpr) {
      return registerNewVersionedValue(value,
                                       new VersionedValue(value, valueExpr));
    }

    /// \brief Create a new versioned value object, which is a pointer with
    /// absolute address
    VersionedValue *getNewPointerValue(llvm::Value *loc, ref<Expr> address) {
      VersionedValue *vvalue = new VersionedValue(loc, address);
      vvalue->addLocation(MemoryLocation::create(loc, address));
      return registerNewVersionedValue(loc, vvalue);
    }

    /// \brief Create a new versioned value object, which is a pointer which
    /// offsets existing pointer
    VersionedValue *getNewPointerValue(llvm::Value *value, ref<Expr> address,
                                       ref<MemoryLocation> loc,
                                       ref<Expr> offset) {
      VersionedValue *vvalue = new VersionedValue(value, address);
      vvalue->addLocation(MemoryLocation::create(loc, address, offset));
      return registerNewVersionedValue(value, vvalue);
    }

    /// \brief Get all versioned locations for the current node an all of its
    /// parents
    std::vector<ref<MemoryLocation> > getAllVersionedLocations(bool coreOnly =
                                                                   false) const;

    /// \brief Gets the latest version of the location, but without checking
    /// for whether the value is constant or not
    VersionedValue *getLatestValueNoConstantCheck(llvm::Value *value);

    /// \brief Newly relate an location with its stored value
    void updateStore(ref<MemoryLocation> loc, VersionedValue *value);

    /// \brief Add flow dependency between source and target value
    void addDependency(VersionedValue *source, VersionedValue *target);

    /// \brief Add flow dependency between source and target pointers, offset by
    /// some amount
    void addDependencyWithOffset(VersionedValue *source, VersionedValue *target,
                                 ref<Expr> offset);

    /// \brief Add flow dependency between source and target value, as the
    /// result of store/load via a memory location.
    void addDependencyViaLocation(VersionedValue *source,
                                  VersionedValue *target,
                                  ref<MemoryLocation> via);

    /// \brief All values that flows to the target in one step
    std::vector<VersionedValue *> directFlowSources(VersionedValue *target) const;

    /// \brief Mark as core all the values and locations that flows to the
    /// target
    void markFlow(VersionedValue *target) const;

    /// \brief Record the expressions of a call's arguments
    std::vector<VersionedValue *>
    populateArgumentValuesList(llvm::CallInst *site,
                               std::vector<ref<Expr> > &arguments);

  public:
    Dependency(Dependency *parent);

    ~Dependency();

    Dependency *cdr() const;

    VersionedValue *getLatestValue(llvm::Value *value, ref<Expr> valueExpr);

    /// \brief Abstract dependency state transition with argument(s)
    void execute(llvm::Instruction *instr, std::vector<ref<Expr> > &args,
                 bool symbolicExecutionError);

    /// \brief Build dependencies from PHI node
    void executePHI(llvm::Instruction *instr, unsigned int incomingBlock,
                    ref<Expr> valueExpr, bool symbolicExecutionError);

    /// \brief Execute memory operation (load/store)
    void executeMemoryOperation(llvm::Instruction *instr,
                                std::vector<ref<Expr> > &args, bool boundsCheck,
                                bool symbolicExecutionError);

    /// \brief This retrieves the locations known at this state, and the
    /// expressions stored in the locations.
    ///
    /// \param The replacement bound variables when retrieving state for
    /// creating subsumption table entry: As the resulting expression will
    /// be used for storing in the subsumption table, the variables need to be
    /// replaced with the bound ones.
    /// \param Indicate whether we are retrieving only data for locations
    /// relevant to an unsatisfiability core.
    /// \return A pair of the store part indexed by constants, and the store
    /// part
    /// indexed by symbolic expressions.
    std::pair<ConcreteStore, SymbolicStore>
    getStoredExpressions(std::set<const Array *> &replacements, bool coreOnly);

    /// \brief Record call arguments in a function call
    void bindCallArguments(llvm::Instruction *instr,
                           std::vector<ref<Expr> > &arguments);

    /// \brief This propagates the dependency due to the return value of a call
    void bindReturnValue(llvm::CallInst *site, llvm::Instruction *inst,
                         ref<Expr> returnValue);

    /// \brief Given a versioned value, retrieve all its sources and mark them
    /// as in the core.
    void markAllValues(VersionedValue *value);

    /// \brief Given an LLVM value, retrieve all its sources and mark them as in
    /// the core.
    void markAllValues(llvm::Value *value);

    /// \brief Print the content of the object to the LLVM error stream
    void dump() const {
      this->print(llvm::errs());
      llvm::errs() << "\n";
    }

    /// \brief Print the content of the object into a stream.
    ///
    /// \param The stream to print the data to.
    void print(llvm::raw_ostream& stream) const;

    /// \brief Print the content of the object into a stream.
    ///
    /// \param The stream to print the data to.
    /// \param The number of whitespaces to be printed before each line.
    void print(llvm::raw_ostream &stream, const unsigned paddingAmount) const;
  };

  std::string makeTabs(const unsigned paddingAmount);

  std::string appendTab(const std::string &prefix);

}

#endif
