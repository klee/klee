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
    /// \brief The location's LLVM value
    llvm::Value *value;

    /// \brief The absolute address
    ref<Expr> address;

    /// \brief The base address
    ref<Expr> base;

    /// \brief The offset of the allocation
    ref<Expr> offset;

    /// \brief The size of this allocation (0 means unknown)
    uint64_t size;

    MemoryLocation(llvm::Value *_value, ref<Expr> &_address, ref<Expr> &_base,
                   ref<Expr> &_offset, uint64_t _size)
        : refCount(0), value(_value), address(_address), base(_base),
          offset(_offset), size(_size) {}

  public:
    ~MemoryLocation() {}

    static ref<MemoryLocation> create(llvm::Value *value, ref<Expr> &address,
                                      uint64_t size) {
      ref<Expr> zeroPointer = Expr::createPointer(0);
      ref<MemoryLocation> ret(
          new MemoryLocation(value, address, address, zeroPointer, size));
      return ret;
    }

    static ref<MemoryLocation> create(ref<MemoryLocation> loc,
                                      ref<Expr> &address, ref<Expr> &offset) {
      ConstantExpr *c = llvm::dyn_cast<ConstantExpr>(offset);
      if (c && c->getZExtValue() == 0) {
        ref<Expr> base = loc->getBase();
        ref<Expr> offset = loc->getOffset();
        ref<MemoryLocation> ret(new MemoryLocation(loc->getValue(), address,
                                                   base, offset, loc->size));
        return ret;
      }

      ref<Expr> base = loc->getBase();
      ref<Expr> newOffset = AddExpr::create(loc->getOffset(), offset);
      ref<MemoryLocation> ret(new MemoryLocation(loc->getValue(), address, base,
                                                 newOffset, loc->size));
      return ret;
    }

    int compare(const MemoryLocation other) const {
      uint64_t l = reinterpret_cast<uint64_t>(value),
               r = reinterpret_cast<uint64_t>(other.value);

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

    llvm::Value *getValue() const { return value; }

    ref<Expr> getAddress() const { return address; }

    ref<Expr> getBase() const { return base; }

    ref<Expr> getOffset() const { return offset; }

    uint64_t getSize() const { return size; }

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
  public:
    unsigned refCount;

  private:
    llvm::Value *value;

    const ref<Expr> valueExpr;

    /// \brief Set of memory locations possibly being pointed to
    std::set<ref<MemoryLocation> > locations;

    /// \brief Field to indicate if any unsatisfiability core depends on this
    /// value.
    bool core;

    /// \brief The id of this object
    uint64_t id;

    VersionedValue(llvm::Value *value, ref<Expr> valueExpr)
        : refCount(0), value(value), valueExpr(valueExpr), core(false),
          id(reinterpret_cast<uint64_t>(this)) {}

  public:
    ~VersionedValue() { locations.clear(); }

    static ref<VersionedValue> create(llvm::Value *value, ref<Expr> valueExpr) {
      ref<VersionedValue> vvalue(new VersionedValue(value, valueExpr));
      return vvalue;
    }

    int compare(const VersionedValue other) const {
      if (id == other.id)
        return 0;
      if (id < other.id)
        return -1;
      return 1;
    }

    void addLocation(ref<MemoryLocation> loc) { locations.insert(loc); }

    std::set<ref<MemoryLocation> > getLocations() { return locations; }

    bool hasValue(llvm::Value *value) const { return this->value == value; }

    ref<Expr> getExpression() const { return valueExpr; }

    void setAsCore() { core = true; }

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

  /// \brief A processed form of a value to be stored in the subsumption table
  class StoredValue {
  public:
    unsigned refCount;

  private:
    ref<Expr> expr;

    std::map<llvm::Value *, std::set<ref<Expr> > > bounds;

    /// \brief The id of this object
    uint64_t id;

    void init(ref<VersionedValue> vvalue, std::set<const Array *> &replacements,
              bool shadowing = false);

    StoredValue(ref<VersionedValue> vvalue,
                std::set<const Array *> &replacements) {
      init(vvalue, replacements, true);
    }

    StoredValue(ref<VersionedValue> vvalue) {
      std::set<const Array *> dummyReplacements;
      init(vvalue, dummyReplacements);
    }

  public:
    static ref<StoredValue> create(ref<VersionedValue> vvalue,
                                   std::set<const Array *> &replacements) {
      ref<StoredValue> sv(new StoredValue(vvalue, replacements));
      return sv;
    }

    static ref<StoredValue> create(ref<VersionedValue> vvalue) {
      ref<StoredValue> sv(new StoredValue(vvalue));
      return sv;
    }

    ~StoredValue() {}

    int compare(const StoredValue other) const {
      if (id == other.id)
        return 0;
      if (id < other.id)
        return -1;
      return 1;
    }

    bool isPointer() const { return !bounds.empty(); }

    ref<Expr> getExpression() const { return expr; }

    std::set<ref<Expr> > getBounds(llvm::Value *value) { return bounds[value]; }

    void print(llvm::raw_ostream &stream) const;

    void dump() const {
      print(llvm::errs());
      llvm::errs() << "\n";
    }
  };

  /// \brief Computation of memory regions the unsatisfiability core depends
  /// upon, which is used to compute the interpolant.
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
  /// The memory dependency computation is based on shadow data structure with
  /// the following main components:
  ///
  /// Domains:
  /// VersionedValue -> LLVM values (i.e., variables) with versioning index
  /// MemoryLocation -> A pointer value, which is associated with an
  ///                   allocation and its displacement (offset)
  ///
  /// The results of the computation is the set of relations _concreteStore,
  /// _symbolicStore, and flowsToMap, representing the VersionedValue stored in
  /// a memory location with concrete and symbolic address, and the flow
  /// relations between VersionedValue objects in flowsToMap field.
  class Dependency {

  public:
    typedef std::pair<ref<Expr>, ref<StoredValue> > AddressValuePair;
    typedef std::map<uint64_t, AddressValuePair> ConcreteStoreMap;
    typedef std::vector<AddressValuePair> SymbolicStoreMap;
    typedef std::map<llvm::Value *, ConcreteStoreMap> ConcreteStore;
    typedef std::map<llvm::Value *, SymbolicStoreMap> SymbolicStore;

    class Util {

    public:
      /// \brief Tests if a pointer points to a main function's argument
      static bool isMainArgument(llvm::Value *loc);
    };

  private:
    /// \brief Previous path condition
    Dependency *parent;

    /// \brief Argument values to be passed onto callee
    std::vector<ref<VersionedValue> > argumentValuesList;

    /// \brief The mapping of concrete locations to stored value
    std::map<ref<MemoryLocation>, ref<VersionedValue> > _concreteStore;

    /// \brief The mapping of symbolic locations to stored value
    std::map<ref<MemoryLocation>, ref<VersionedValue> > _symbolicStore;

    /// \brief Flow relations of target and its sources with location
    std::map<ref<VersionedValue>,
             std::map<ref<VersionedValue>, ref<MemoryLocation> > > flowsToMap;

    /// \brief The store of the versioned values
    std::map<llvm::Value *, std::vector<ref<VersionedValue> > > valuesMap;

    /// \brief Locations of this node and its ancestors that are needed for
    /// the core and dominates other locations.
    std::set<ref<MemoryLocation> > coreLocations;

    /// \brief The data layout of the analysis target program
    llvm::DataLayout *targetData;

    /// \brief Register new versioned value, used by getNewVersionedValue
    /// methods
    ref<VersionedValue> registerNewVersionedValue(llvm::Value *value,
                                                  ref<VersionedValue> vvalue);

    /// \brief Create a new versioned value object, typically when executing a
    /// new instruction, as a value for the instruction.
    ref<VersionedValue> getNewVersionedValue(llvm::Value *value,
                                             ref<Expr> valueExpr) {
      return registerNewVersionedValue(
          value, VersionedValue::create(value, valueExpr));
    }

    /// \brief Create a new versioned value object, which is a pointer with
    /// absolute address
    ref<VersionedValue> getNewPointerValue(llvm::Value *loc, ref<Expr> address,
                                           uint64_t size) {
      ref<VersionedValue> vvalue = VersionedValue::create(loc, address);
      vvalue->addLocation(MemoryLocation::create(loc, address, size));
      return registerNewVersionedValue(loc, vvalue);
    }

    /// \brief Create a new versioned value object, which is a pointer which
    /// offsets existing pointer
    ref<VersionedValue> getNewPointerValue(llvm::Value *value,
                                           ref<Expr> address,
                                           ref<MemoryLocation> loc,
                                           ref<Expr> offset) {
      ref<VersionedValue> vvalue = VersionedValue::create(value, address);
      vvalue->addLocation(MemoryLocation::create(loc, address, offset));
      return registerNewVersionedValue(value, vvalue);
    }

    /// \brief Gets the latest version of the location, but without checking
    /// for whether the value is constant or not
    ref<VersionedValue> getLatestValueNoConstantCheck(llvm::Value *value);

    /// \brief Newly relate an location with its stored value
    void updateStore(ref<MemoryLocation> loc, ref<VersionedValue> value);

    /// \brief The core routine for adding flow dependency between source and
    /// target value
    void addDependencyCore(ref<VersionedValue> source,
                           ref<VersionedValue> target, ref<MemoryLocation> via);

    /// \brief Add flow dependency between source and target value
    void addDependency(ref<VersionedValue> source, ref<VersionedValue> target);

    /// \brief Add flow dependency between source and target pointers, offset by
    /// some amount
    void addDependencyWithOffset(ref<VersionedValue> source,
                                 ref<VersionedValue> target, ref<Expr> offset);

    /// \brief Add flow dependency between source and target value, as the
    /// result of store/load via a memory location.
    void addDependencyViaLocation(ref<VersionedValue> source,
                                  ref<VersionedValue> target,
                                  ref<MemoryLocation> via);

    /// \brief All values that flows to the target in one step
    std::vector<ref<VersionedValue> >
    directFlowSources(ref<VersionedValue> target) const;

    /// \brief Mark as core all the values and locations that flows to the
    /// target
    void markFlow(ref<VersionedValue> target) const;

    /// \brief Record the expressions of a call's arguments
    std::vector<ref<VersionedValue> >
    populateArgumentValuesList(llvm::CallInst *site,
                               std::vector<ref<Expr> > &arguments);

  public:
    Dependency(Dependency *parent, llvm::DataLayout *_targetData);

    ~Dependency();

    Dependency *cdr() const;

    ref<VersionedValue> getLatestValue(llvm::Value *value, ref<Expr> valueExpr);

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
    void markAllValues(ref<VersionedValue> value);

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
