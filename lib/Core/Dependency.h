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

  class VersionedValue;

  /// \brief Output function name to the output stream
  extern bool outputFunctionName(llvm::Value *value, llvm::raw_ostream &stream);

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

    /// \brief The offset wrt. the allocation
    ref<Expr> offset;

    /// \brief The expressions representing the bound on the offset, i.e., the
    /// interpolant, in case it is symbolic.
    std::set<ref<Expr> > symbolicOffsetBounds;

    /// \brief This is the concrete offset bound
    uint64_t concreteOffsetBound;

    /// \brief The size of this allocation (0 means unknown)
    uint64_t size;

    MemoryLocation(llvm::Value *_value, ref<Expr> &_address, ref<Expr> &_base,
                   ref<Expr> &_offset, uint64_t _size)
        : refCount(0), value(_value), offset(_offset),
          concreteOffsetBound(_size), size(_size) {
      bool unknownBase = false;

      ConstantExpr *ca = llvm::dyn_cast<ConstantExpr>(_address);
      if (ca) {
        ConstantExpr *cb = llvm::dyn_cast<ConstantExpr>(_base);
        if (cb) {
          ConstantExpr *co = llvm::dyn_cast<ConstantExpr>(_offset);
          if (co) {
            uint64_t a = ca->getZExtValue();
            uint64_t b = cb->getZExtValue();
            uint64_t o = co->getZExtValue();
            if (b == 0 && a != 0) {
              unknownBase = true;
            } else {
              assert(o == (a - b) && "wrong offset");
            }
          }
        }
      }

      Expr::Width pointerWidth = Expr::createPointer(0)->getWidth();
      if (_address->getWidth() < pointerWidth) {
        address = ZExtExpr::create(_address, pointerWidth);
      } else {
        address = _address;
      }

      if (_base->getWidth() < pointerWidth) {
        base = ZExtExpr::create(_base, pointerWidth);
      } else {
        base = _base;
      }

      if (unknownBase) {
        ref<Expr> tmpOffset;
        if (_offset->getWidth() < pointerWidth) {
          tmpOffset = ZExtExpr::create(_offset, pointerWidth);
        } else {
          tmpOffset = _offset;
        }
        base = SubExpr::create(address, tmpOffset);
      }
    }

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
                                      ref<Expr> &address,
                                      ref<Expr> &offsetDelta) {
      ConstantExpr *c = llvm::dyn_cast<ConstantExpr>(offsetDelta);
      if (c && c->getZExtValue() == 0) {
        ref<Expr> base = loc->base;
        ref<Expr> offset = loc->offset;
        ref<MemoryLocation> ret(
            new MemoryLocation(loc->value, address, base, offset, loc->size));
        return ret;
      }

      ref<Expr> base = loc->base;
      ref<Expr> newOffset = AddExpr::create(loc->offset, offsetDelta);
      ref<MemoryLocation> ret(
          new MemoryLocation(loc->value, address, base, newOffset, loc->size));
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
        r = reinterpret_cast<uint64_t>(other.address.get());
        if (l == r)
          return 0;
        if (l < r)
          return -1;
        return 1;
      } else if (l < r)
        return -1;
      return 1;
    }

    /// \brief Adjust the offset bound for interpolation (a.k.a. slackening)
    void adjustOffsetBound(ref<VersionedValue> checkedAddress,
                           std::set<ref<Expr> > &bounds);

    bool hasConstantAddress() const { return llvm::isa<ConstantExpr>(address); }

    uint64_t getUIntAddress() const {
      return llvm::dyn_cast<ConstantExpr>(address)->getZExtValue();
    }

    llvm::Value *getValue() const { return value; }

    ref<Expr> getAddress() const { return address; }

    ref<Expr> getBase() const { return base; }

    std::set<ref<Expr> > getSymbolicOffsetBounds() const {
      return symbolicOffsetBounds;
    }

    uint64_t getConcreteOffsetBound() const { return concreteOffsetBound; }

    ref<Expr> getOffset() const { return offset; }

    uint64_t getSize() const { return size; }

    /// \brief Print the content of the object to the LLVM error stream
    void dump() const {
      print(llvm::errs());
      llvm::errs() << "\n";
    }

    /// \brief Print the content of the object into a stream.
    ///
    /// \param stream The stream to print the data to.
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

    /// \brief Member variable to indicate if any unsatisfiability core depends
    /// on this value.
    bool core;

    /// \brief The id of this object
    uint64_t id;

    /// \brief Dependency sources of this value
    std::map<ref<VersionedValue>, ref<MemoryLocation> > sources;

    /// \brief Do not compute bounds in interpolation of this value if it was a
    /// pointer; instead, use exact address
    bool doNotInterpolateBound;

    /// \brief The load address of this value, in case it was a load instruction
    ref<VersionedValue> loadAddress;

    /// \brief The address by which the loaded value was stored, in case this
    /// was a load instruction
    ref<VersionedValue> storeAddress;

    VersionedValue(llvm::Value *value, ref<Expr> valueExpr)
        : refCount(0), value(value), valueExpr(valueExpr), core(false),
          id(reinterpret_cast<uint64_t>(this)), doNotInterpolateBound(false) {}

    /// \brief Print the content of the object, but without showing its source
    /// values.
    ///
    /// \param stream The stream to print the data to.
    void printNoDependency(llvm::raw_ostream &stream) const;

  public:
    ~VersionedValue() { locations.clear(); }

    static ref<VersionedValue> create(llvm::Value *value, ref<Expr> valueExpr) {
      ref<VersionedValue> vvalue(new VersionedValue(value, valueExpr));
      return vvalue;
    }

    bool canInterpolateBound() { return !doNotInterpolateBound; }

    void disableBoundInterpolation() { doNotInterpolateBound = true; }

    void setLoadAddress(ref<VersionedValue> _loadAddress) {
      loadAddress = _loadAddress;
    }

    ref<VersionedValue> getLoadAddress() { return loadAddress; }

    void setStoreAddress(ref<VersionedValue> _storeAddress) {
      storeAddress = _storeAddress;
    }

    ref<VersionedValue> getStoreAddress() { return storeAddress; }

    /// \brief The core routine for adding flow dependency between source and
    /// target value
    void addDependency(ref<VersionedValue> source, ref<MemoryLocation> via) {
      sources[source] = via;
    }

    std::map<ref<VersionedValue>, ref<MemoryLocation> > getSources() {
      return sources;
    }

    int compare(const VersionedValue other) const {
      if (id == other.id)
        return 0;
      if (id < other.id)
        return -1;
      return 1;
    }

    void addLocation(ref<MemoryLocation> loc) {
      if (locations.find(loc) == locations.end())
        locations.insert(loc);
    }

    std::set<ref<MemoryLocation> > getLocations() const { return locations; }

    bool hasValue(llvm::Value *value) const { return this->value == value; }

    ref<Expr> getExpression() const { return valueExpr; }

    void setAsCore() { core = true; }

    bool isCore() const { return core; }

    llvm::Value *getValue() const { return value; }

    /// \brief Print the content of the object into a stream.
    ///
    /// \param stream The stream to print the data to.
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

    /// \brief In case the stored value was a pointer, then this should be a
    /// non-empty map mapping of allocation sites to the set of offset bounds.
    /// This constitutes the weakest liberal precondition of the memory checks
    /// against which the offsets of the pointer values of the current state are
    /// to be checked against.
    std::map<llvm::Value *, std::set<ref<Expr> > > allocationBounds;

    /// \brief In case the stored value was a pointer, then this should be a
    /// non-empty map mapping of allocation sites to the set of offsets. This is
    /// the offset values of the current state to be checked against the offset
    /// bounds.
    std::map<llvm::Value *, std::set<ref<Expr> > > allocationOffsets;

    /// \brief The id of this object
    uint64_t id;

    /// \brief The LLVM value of this object
    llvm::Value *value;

    /// \brief Do not use bound in subsumption check
    bool doNotUseBound;

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

    bool useBound() { return !doNotUseBound; }

    bool isPointer() const { return !allocationBounds.empty(); }

    ref<Expr> getBoundsCheck(ref<StoredValue> svalue,
                             std::set<ref<Expr> > &bounds) const;

    ref<Expr> getExpression() const { return expr; }

    std::set<ref<Expr> > getBounds(llvm::Value *value) const {
      return allocationBounds.at(value);
    }

    llvm::Value *getValue() const { return value; }

    void print(llvm::raw_ostream &stream) const;

    void dump() const {
      print(llvm::errs());
      llvm::errs() << "\n";
    }
  };

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
  /// - VersionedValue: LLVM values (i.e., variables) with versioning
  ///   index. This represents the values loaded from memory into LLVM
  ///   temporary variables. They have versioning index, as, different
  ///   from LLVM values themselves which are static entities, a
  ///   (symbolic) execution may go through the same instruction
  ///   multiple times. Hence the value of that instruction has to be
  ///   versioned.
  /// - MemoryLocation: A representation of pointers. It is important
  ///   to note that each pointer is associated with memory allocation
  ///   and its displacement (offset) wrt. the base address of the
  ///   allocation.
  ///
  /// The results of the computation is stored in several member
  /// variables as follows, mainly Dependency#concretelyAddressedStore and
  /// Dependency#symbolicallyAddressedStore which represent the components of
  /// the state associated with the owner ITreeNode object of the Dependency
  /// object. Dependency#concretelyAddressedStore is the part of the state that
  /// are concretely addressed, whereas Dependency#symbolicallyAddressedStore is
  /// the part that is symbolically addressed.
  ///
  /// <b>Notes on pointer flow propagation</b>
  ///
  /// A VersionedValue object may represent a pointer value, in which
  /// case it is linked to possibly several MemoryLocation objects via
  /// VersionedValue#locations member variable. Such VersionedValue
  /// object may be used in memory access operations of LLVM
  /// (<b>load</b> or <b>store</b>). The memory dependency computation
  /// propagates such pointer value information in MemoryLocation from
  /// one VersionedValue to another such that there is no need to
  /// inefficiently hunt for the pointer value at the point of use of
  /// the pointer. For example, a symbolic execution of LLVM's
  /// <b>getelementptr</b> instruction would create a new
  /// VersionedValue representing the return value of the
  /// instruction. This new VersionedValue would inherit all members
  /// of the VersionedValue#locations variable of the VersionedValue
  /// object representing the pointer argument of the instruction,
  /// with modified offsets according to the offset argument of the
  /// instruction.
  ///
  /// <b>Notes on why stacks are not used</b>
  ///
  /// Here we are not using stacks to implement the shadow memory
  /// for dependency information. The reason is that when a function
  /// is called and then it returned, its local allocations can not
  /// normally be removed from consideration as the function may
  /// contain unexplored branches. So therefore, it is in principle the
  /// case that subsequently allocated memory regions are disjoint from
  /// whatever local memories that have ever been allocated throughout
  /// the execution anyway. This, however, conflicts with the notion
  /// of the latest value of a variable, where there is a potential
  /// for confusion where, given a recursive function, the latest
  /// value of a local variable is considered to be the one allocated
  /// in its recursive call that has been exited (because the local
  /// variable in the recursive call is allocated lated). But for this,
  /// we add another check to ensure that the stored value match
  /// (in Dependency#getLatestValue).
  ///
  /// \see ITree
  /// \see ITreeNode
  /// \see VersionedValue
  /// \see MemoryLocation
  class Dependency {

  public:
    typedef std::pair<ref<Expr>, ref<StoredValue> > AddressValuePair;
    typedef std::map<uint64_t, ref<StoredValue> > ConcreteStoreMap;
    typedef std::vector<AddressValuePair> SymbolicStoreMap;
    typedef std::map<llvm::Value *, ConcreteStoreMap> ConcreteStore;
    typedef std::map<llvm::Value *, SymbolicStoreMap> SymbolicStore;

  private:
    /// \brief Previous path condition
    Dependency *parent;

    /// \brief Argument values to be passed onto callee
    std::vector<ref<VersionedValue> > argumentValuesList;

    /// \brief The mapping of concrete locations to stored value
    std::map<ref<MemoryLocation>,
             std::pair<ref<VersionedValue>, ref<VersionedValue> > >
    concretelyAddressedStore;

    /// \brief The mapping of symbolic locations to stored value
    std::map<ref<MemoryLocation>,
             std::pair<ref<VersionedValue>, ref<VersionedValue> > >
    symbolicallyAddressedStore;

    /// \brief The store of the versioned values
    std::map<llvm::Value *, std::vector<ref<VersionedValue> > > valuesMap;

    /// \brief Locations of this node and its ancestors that are needed for
    /// the core and dominates other locations.
    std::set<ref<MemoryLocation> > coreLocations;

    /// \brief The data layout of the analysis target program
    llvm::DataLayout *targetData;

    /// \brief Tests if a pointer points to a main function's argument
    static bool isMainArgument(llvm::Value *loc);

    /// \brief Register new versioned value, used by getNewVersionedValue
    /// member functions
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
    /// for whether the value is constant or not.
    ref<VersionedValue> getLatestValueNoConstantCheck(llvm::Value *value,
                                                      ref<Expr> expr);

    /// \brief Gets the latest pointer value for marking
    ref<VersionedValue> getLatestValueForMarking(llvm::Value *val,
                                                 ref<Expr> expr);

    /// \brief Newly relate an location with its stored value
    void updateStore(ref<MemoryLocation> loc, ref<VersionedValue> address,
                     ref<VersionedValue> value);

    /// \brief Add flow dependency between source and target value
    void addDependency(ref<VersionedValue> source, ref<VersionedValue> target,
                       bool multiLocationsCheck = true);

    /// \brief Add flow dependency between source and target value
    void addDependencyIntToPtr(ref<VersionedValue> source,
                               ref<VersionedValue> target);

    /// \brief Add flow dependency between source and target pointers, offset by
    /// some amount
    void addDependencyWithOffset(ref<VersionedValue> source,
                                 ref<VersionedValue> target,
                                 ref<Expr> offsetDelta);

    /// \brief Add flow dependency between source and target value, as the
    /// result of store/load via a memory location.
    void addDependencyViaLocation(ref<VersionedValue> source,
                                  ref<VersionedValue> target,
                                  ref<MemoryLocation> via);

    /// \brief Add a flow dependency from a pointer value to a non-pointer
    /// value, for an external function call.
    ///
    /// Here the target is not a pointer, and we assume that the source is
    /// is checked for memory access validity at the current index, meaning that
    /// we assumed all memory access within the external function is valid.
    void addDependencyViaExternalFunction(ref<VersionedValue> source,
                                          ref<VersionedValue> target);

    /// \brief Add a flow dependency from a pointer value to a non-pointer
    /// value.
    void addDependencyToNonPointer(ref<VersionedValue> source,
                                   ref<VersionedValue> target);

    /// \brief All values that flows to the target in one step
    std::vector<ref<VersionedValue> >
    directFlowSources(ref<VersionedValue> target) const;

    /// \brief Mark as core all the values and locations that flows to the
    /// target
    void markFlow(ref<VersionedValue> target) const;

    /// \brief Mark as core all the pointer values and that flows to the target;
    /// and adjust its offset bound for memory bounds interpolation (a.k.a.
    /// slackening)
    void markPointerFlow(ref<VersionedValue> target,
                         ref<VersionedValue> checkedOffset) const {
      std::set<ref<Expr> > bounds;
      markPointerFlow(target, checkedOffset, bounds);
    }

    /// \brief Mark as core all the pointer values and that flows to the target;
    /// and adjust its offset bound for memory bounds interpolation (a.k.a.
    /// slackening)
    void markPointerFlow(ref<VersionedValue> target,
                         ref<VersionedValue> checkedOffset,
                         std::set<ref<Expr> > &bounds) const;

    /// \brief Record the expressions of a call's arguments
    std::vector<ref<VersionedValue> >
    populateArgumentValuesList(llvm::CallInst *site,
                               std::vector<ref<Expr> > &arguments);

  public:
    Dependency(Dependency *parent, llvm::DataLayout *_targetData);

    ~Dependency();

    Dependency *cdr() const;

    ref<VersionedValue> getLatestValue(llvm::Value *value, ref<Expr> valueExpr,
                                       bool constraint = false);

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
    /// \param replacements The replacement bound variables when
    /// retrieving state for creating subsumption table entry: As the
    /// resulting expression will be used for storing in the
    /// subsumption table, the variables need to be replaced with the
    /// bound ones.
    /// \param coreOnly Indicate whether we are retrieving only data
    /// for locations relevant to an unsatisfiability core.
    /// \return A pair of the store part indexed by constants, and the
    /// store part indexed by symbolic expressions.
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
    void markAllValues(llvm::Value *value, ref<Expr> expr);

    /// \brief Given an LLVM value which is used as an address, retrieve all its
    /// sources and mark them as in the core.
    void markAllPointerValues(llvm::Value *val, ref<Expr> address) {
      std::set<ref<Expr> > bounds;
      markAllPointerValues(val, address, bounds);
    }

    /// \brief Given an LLVM value which is used as an address, retrieve all its
    /// sources and mark them as in the core.
    void markAllPointerValues(llvm::Value *val, ref<Expr> address,
                              std::set<ref<Expr> > &bounds);

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

  std::string makeTabs(const unsigned paddingAmount);

  std::string appendTab(const std::string &prefix);

}

#endif
