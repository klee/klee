//===--- TxValues.h ---------------------------------------------*- C++ -*-===//
//
//               The Tracer-X KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declarations of the classes related to values in the
/// symbolic execution state and interpolant table. Values that belong to the
/// interpolant are versioned such as MemoryLocation, which is distinguished by
/// its base address, and VersionedValue, which is distinguished by its version,
/// and VersionedValue, which is distinguished by its own object id.
///
//===----------------------------------------------------------------------===//

#ifndef KLEE_VERSIONEDVALUE_H
#define KLEE_VERSIONEDVALUE_H

#include "klee/Config/Version.h"
#include "klee/Expr.h"

#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 3)
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Value.h>
#else
#include <llvm/Instruction.h>
#include <llvm/Value.h>
#endif

#include <vector>

namespace klee {

class VersionedValue;

class AllocationContext {

public:
  unsigned refCount;

private:
  /// \brief The location's LLVM value
  llvm::Value *value;

  /// \brief The call history by which the allocation is reached
  std::vector<llvm::Instruction *> callHistory;

  AllocationContext(llvm::Value *_value,
                    const std::vector<llvm::Instruction *> &_callHistory)
      : refCount(0), value(_value), callHistory(_callHistory) {}

public:
  ~AllocationContext() { callHistory.clear(); }

  static ref<AllocationContext>
  create(llvm::Value *_value,
         const std::vector<llvm::Instruction *> &_callHistory) {
    ref<AllocationContext> ret(new AllocationContext(_value, _callHistory));
    return ret;
  }

  llvm::Value *getValue() const { return value; }

  const std::vector<llvm::Instruction *> &getCallHistory() const {
    return callHistory;
  }

  bool isPrefixOf(const std::vector<llvm::Instruction *> &_callHistory) const {
    for (std::vector<llvm::Instruction *>::const_iterator
             it1 = callHistory.begin(),
             ie1 = callHistory.end(), it2 = _callHistory.begin(),
             ie2 = _callHistory.end();
         it1 != ie1; ++it1, ++it2) {
      if (it2 == ie2 || (*it1) != (*it2)) {
        return false;
      }
    }
    return true;
  }

  int compare(const AllocationContext &other) const {
    if (value == other.value) {
      // Please note the use of reverse iterator here, which improves
      // performance.
      for (std::vector<llvm::Instruction *>::const_reverse_iterator
               it1 = callHistory.rbegin(),
               ie1 = callHistory.rend(), it2 = other.callHistory.rbegin(),
               ie2 = other.callHistory.rend();
           ; ++it1, ++it2) {
        if (it1 == ie1) {
          if (it2 == ie2) {
            break;
          } else
            return -1;
        }
        if (it2 == ie2)
          return 1;
        if (it1 == ie1)
          return 1;
        if ((*it1) > (*it2))
          return 2;
        if (it2 == ie2)
          return -1;
        if ((*it1) < (*it2))
          return -2;
      }
      return 0;
    } else if (value < other.value) {
      return -3;
    }
    return 3;
  }

  /// \brief Print the content of the object to the LLVM error stream
  void dump() const {
    print(llvm::errs());
    llvm::errs() << "\n";
  }

  /// \brief Print the content of the object into a stream.
  ///
  /// \param stream The stream to print the data to.
  void print(llvm::raw_ostream &stream) const {
    std::string emptyString;
    print(stream, emptyString);
  }

  /// \brief Print the content of the object into a stream.
  ///
  /// \param stream The stream to print the data to.
  /// \param prefix Padding spaces to print before the actual data.
  void print(llvm::raw_ostream &stream, const std::string &prefix) const;
};

/// \brief The address to be stored as an index in the subsumption table. This
/// class wraps a memory location, supplying weaker address equality comparison
/// for the purpose of subsumption checking
class StoredAddress {
public:
  unsigned refCount;

private:
  /// \brief the context (allocation site and call history) of the allocation of
  /// this address
  ref<AllocationContext> context;

  /// \brief The offset wrt. the allocation
  ref<Expr> offset;

  /// \brief Indicates concrete address / offset
  bool isConcrete;

  /// \brief The value of the concrete offset
  uint64_t concreteOffset;

  StoredAddress(ref<AllocationContext> _context, ref<Expr> _offset)
      : refCount(0), context(_context), offset(_offset) {
    isConcrete = false;
    concreteOffset = 0;

    if (ConstantExpr *ce = llvm::dyn_cast<ConstantExpr>(offset)) {
      isConcrete = true;
      concreteOffset = ce->getZExtValue();
    }
  }

public:
  static ref<StoredAddress> create(ref<AllocationContext> context,
                                   ref<Expr> offset) {
    ref<StoredAddress> ret(new StoredAddress(context, offset));
    return ret;
  }

  ref<AllocationContext> getContext() const { return context; }

  ref<Expr> getOffset() const { return offset; }

  /// \brief The comparator of this class' objects. This member function is
  /// weaker than standard comparator for MemoryLocation in that it does not
  /// check for the equality of allocation id. Allocation id is used in
  /// MemoryLocation (member variable MemoryLocation#allocationId) for the
  /// purpose of distinguishing memory allocations of the same callsite and call
  /// history, but of different loop iterations. This does not make sense when
  /// comparing states for subsumption as in subsumption, related allocations in
  /// different paths may have different allocation ids.
  int compare(const StoredAddress &other) const {
    int res = context->compare(*(other.context.get()));
    if (res)
      return res;

    if (offset == other.offset)
      return 0;
    if (isConcrete && other.isConcrete) {
      if (concreteOffset < other.concreteOffset)
        return -1;
      return 1;
    }

    if (offset->hash() < other.offset->hash())
      return -1;

    return 1;
  }

  void print(llvm::raw_ostream &stream) const { print(stream, ""); }

  void print(llvm::raw_ostream &stream, const std::string &prefix) const;

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
  /// to be checked.
  std::map<ref<AllocationContext>, std::set<ref<Expr> > > allocationBounds;

  /// \brief In case the stored value was a pointer, then this should be a
  /// non-empty map mapping of allocation sites to the set of offsets. This is
  /// the offset values of the current state to be checked against the offset
  /// bounds.
  std::map<ref<AllocationContext>, std::set<ref<Expr> > > allocationOffsets;

  /// \brief The id of this object
  uint64_t id;

  /// \brief The LLVM value of this object
  llvm::Value *value;

  /// \brief Do not use bound in subsumption check
  bool doNotUseBound;

  /// \brief Reason this was stored as needed value
  std::set<std::string> coreReasons;

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

  bool isPointer() const { return !allocationOffsets.empty(); }

  ref<Expr> getBoundsCheck(ref<StoredValue> svalue,
                           std::set<ref<Expr> > &bounds,
                           int debugSubsumptionLevel) const;

  ref<Expr> getExpression() const { return expr; }

  llvm::Value *getValue() const { return value; }

  void print(llvm::raw_ostream &stream) const;

  void print(llvm::raw_ostream &stream, const std::string &prefix) const;

  void dump() const {
    print(llvm::errs());
    llvm::errs() << "\n";
  }
};

/// \brief A class to represent memory locations.
class MemoryLocation {

public:
  unsigned refCount;

private:
  /// \brief Address for use in interpolants, with less information
  ref<StoredAddress> interpolantStyleAddress;

  /// \brief The absolute address
  ref<Expr> address;

  /// \brief The base address
  ref<Expr> base;

  /// \brief The expressions representing the bound on the offset, i.e., the
  /// interpolant, in case it is symbolic.
  std::set<ref<Expr> > symbolicOffsetBounds;

  /// \brief This is the concrete offset bound
  uint64_t concreteOffsetBound;

  /// \brief The size of this allocation (0 means unknown)
  uint64_t size;

  MemoryLocation(ref<AllocationContext> _context, ref<Expr> &_address,
                 ref<Expr> &_base, ref<Expr> &_offset, uint64_t _size)
      : refCount(0),
        interpolantStyleAddress(StoredAddress::create(_context, _offset)),
        concreteOffsetBound(_size), size(_size) {
    bool unknownBase = false;

    if (ConstantExpr *co = llvm::dyn_cast<ConstantExpr>(_offset)) {
      uint64_t concreteOffset = co->getZExtValue();

      if (ConstantExpr *ca = llvm::dyn_cast<ConstantExpr>(_address)) {
        if (ConstantExpr *cb = llvm::dyn_cast<ConstantExpr>(_base)) {
          uint64_t a = ca->getZExtValue();
          uint64_t b = cb->getZExtValue();

          if (b == 0 && a != 0) {
            unknownBase = true;
          } else {
            assert(concreteOffset == (a - b) && "wrong offset");
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

  static ref<MemoryLocation>
  create(llvm::Value *value,
         const std::vector<llvm::Instruction *> &_callHistory,
         ref<Expr> &address, uint64_t size) {
    ref<Expr> zeroPointer = Expr::createPointer(0);
    ref<MemoryLocation> ret(
        new MemoryLocation(AllocationContext::create(value, _callHistory),
                           address, address, zeroPointer, size));
    return ret;
  }

  static ref<MemoryLocation> create(ref<MemoryLocation> loc,
                                    std::set<const Array *> &replacements);

  static ref<MemoryLocation> create(ref<MemoryLocation> loc, ref<Expr> &address,
                                    ref<Expr> &offsetDelta) {
    ConstantExpr *c = llvm::dyn_cast<ConstantExpr>(offsetDelta);
    if (c && c->getZExtValue() == 0) {
      ref<Expr> base = loc->base;
      ref<Expr> offset = loc->getOffset();
      ref<MemoryLocation> ret(new MemoryLocation(loc->getContext(), address,
                                                 base, offset, loc->size));
      return ret;
    }

    ref<Expr> base = loc->base;
    ref<Expr> newOffset = AddExpr::create(loc->getOffset(), offsetDelta);
    ref<MemoryLocation> ret(new MemoryLocation(loc->getContext(), address, base,
                                               newOffset, loc->size));
    return ret;
  }

  ref<StoredAddress> &getStoredAddress() { return interpolantStyleAddress; }

  bool
  contextIsPrefixOf(const std::vector<llvm::Instruction *> &callHistory) const {
    return getContext()->isPrefixOf(callHistory);
  }

  int compare(const MemoryLocation &other) const {
    int res = interpolantStyleAddress->compare(
        *(other.interpolantStyleAddress.get()));
    if (res)
      return res;

    if (base == other.base)
      return 0;

    if (reinterpret_cast<uintptr_t>(base.get()) <
        reinterpret_cast<uintptr_t>(other.base.get()))
      return -3;

    return 3;
  }

  /// \brief Adjust the offset bound for interpolation (a.k.a. slackening)
  void adjustOffsetBound(ref<VersionedValue> checkedAddress,
                         std::set<ref<Expr> > &bounds);

  bool hasConstantAddress() const { return llvm::isa<ConstantExpr>(address); }

  uint64_t getUIntAddress() const {
    return llvm::dyn_cast<ConstantExpr>(address)->getZExtValue();
  }

  ref<Expr> getAddress() const { return address; }

  ref<Expr> getBase() const { return base; }

  const std::set<ref<Expr> > &getSymbolicOffsetBounds() const {
    return symbolicOffsetBounds;
  }

  ref<AllocationContext> getContext() const {
    return interpolantStyleAddress->getContext();
  }

  uint64_t getConcreteOffsetBound() const { return concreteOffsetBound; }

  ref<Expr> getOffset() const { return interpolantStyleAddress->getOffset(); }

  uint64_t getSize() const { return size; }

  /// \brief Print the content of the object to the LLVM error stream
  void dump() const {
    print(llvm::errs());
    llvm::errs() << "\n";
  }

  /// \brief Print the content of the object into a stream.
  ///
  /// \param stream The stream to print the data to.
  void print(llvm::raw_ostream &stream) const {
    std::string emptyString;
    print(stream, emptyString);
  }

  /// \brief Print the content of the object into a stream.
  ///
  /// \param stream The stream to print the data to.
  /// \param prefix Padding spaces to print before the actual data.
  void print(llvm::raw_ostream &stream, const std::string &prefix) const;
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

  /// \brief The context of this value
  std::vector<llvm::Instruction *> callHistory;

  /// \brief Do not compute bounds in interpolation of this value if it was a
  /// pointer; instead, use exact address
  bool doNotInterpolateBound;

  /// \brief The load address of this value, in case it was a load instruction
  ref<VersionedValue> loadAddress;

  /// \brief The address by which the loaded value was stored, in case this
  /// was a load instruction
  ref<VersionedValue> storeAddress;

  /// \brief Reasons for this value to be in the core
  std::set<std::string> coreReasons;

  VersionedValue(llvm::Value *value,
                 const std::vector<llvm::Instruction *> &_callHistory,
                 ref<Expr> _valueExpr)
      : refCount(0), value(value), valueExpr(_valueExpr), core(false),
        id(reinterpret_cast<uint64_t>(this)), callHistory(_callHistory),
        doNotInterpolateBound(false) {}

  /// \brief Print the content of the object, but without showing its source
  /// values.
  ///
  /// \param stream The stream to print the data to.
  void printNoDependency(llvm::raw_ostream &stream) const {
    std::string emptyString;
    printNoDependency(stream, emptyString);
  }

  /// \brief Print the content of the object, but without showing its source
  /// values.
  ///
  /// \param stream The stream to print the data to.
  /// \param prefix Padding spaces to print before the actual data.
  void printNoDependency(llvm::raw_ostream &stream,
                         const std::string &prefix) const;

public:
  ~VersionedValue() { locations.clear(); }

  static ref<VersionedValue>
  create(llvm::Value *value,
         const std::vector<llvm::Instruction *> &_callHistory,
         ref<Expr> valueExpr) {
    ref<VersionedValue> vvalue(
        new VersionedValue(value, _callHistory, valueExpr));
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

  const std::map<ref<VersionedValue>, ref<MemoryLocation> > &getSources() {
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

  const std::set<ref<MemoryLocation> > &getLocations() const {
    return locations;
  }

  bool hasValue(llvm::Value *value) const { return this->value == value; }

  ref<Expr> getExpression() const { return valueExpr; }

  void setAsCore(std::string reason) {
    core = true;
    if (!reason.empty())
      coreReasons.insert(reason);
  }

  bool isCore() const { return core; }

  llvm::Value *getValue() const { return value; }

  const std::vector<llvm::Instruction *> &getCallHistory() const {
    return callHistory;
  }

  const std::set<std::string> &getReasons() const { return coreReasons; }

  /// \brief Print the content of the object into a stream.
  ///
  /// \param stream The stream to print the data to.
  void print(llvm::raw_ostream &stream) const {
    std::string emptyString;
    print(stream, emptyString);
  }

  /// \brief Print the content of the object into a stream.
  ///
  /// \param stream The stream to print the data to.
  void print(llvm::raw_ostream &stream, const std::string &prefix) const;

  /// \brief Print the content of the object to the LLVM error stream
  void dump() const {
    print(llvm::errs());
    llvm::errs() << "\n";
  }
};
}

#endif
