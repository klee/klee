//===--- VersionedValue.h ---------------------------------------*- C++ -*-===//
//
//               The Tracer-X KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declarations of the classes related to versioned
/// value. Versioned values are data tokens that can constitute the nodes in
/// building up the dependency graph, for the purpose of computing interpolants.
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

/// \brief A class to represent memory locations.
class MemoryLocation {

public:
  unsigned refCount;

private:
  /// \brief The location's LLVM value
  llvm::Value *value;

  /// \brief The allocation's stack
  std::vector<llvm::Instruction *> stack;

  /// \brief The absolute address
  ref<Expr> address;

  /// \brief The base address
  ref<Expr> base;

  /// \brief The offset wrt. the allocation
  ref<Expr> offset;

  /// \brief Indicates concrete address / offset
  bool isConcrete;

  /// \brief The value of the concrete offset
  uint64_t concreteOffset;

  /// \brief The expressions representing the bound on the offset, i.e., the
  /// interpolant, in case it is symbolic.
  std::set<ref<Expr> > symbolicOffsetBounds;

  /// \brief This is the concrete offset bound
  uint64_t concreteOffsetBound;

  /// \brief The size of this allocation (0 means unknown)
  uint64_t size;

  MemoryLocation(llvm::Value *_value,
                 const std::vector<llvm::Instruction *> &_stack,
                 ref<Expr> &_address, ref<Expr> &_base, ref<Expr> &_offset,
                 uint64_t _size)
      : refCount(0), value(_value), stack(_stack), offset(_offset),
        concreteOffsetBound(_size), size(_size) {
    bool unknownBase = false;

    isConcrete = false;
    if (ConstantExpr *co = llvm::dyn_cast<ConstantExpr>(_offset)) {
      isConcrete = true;
      concreteOffset = co->getZExtValue();

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
  create(llvm::Value *value, const std::vector<llvm::Instruction *> &stack,
         ref<Expr> &address, uint64_t size) {
    ref<Expr> zeroPointer = Expr::createPointer(0);
    ref<MemoryLocation> ret(
        new MemoryLocation(value, stack, address, address, zeroPointer, size));
    return ret;
  }

  static ref<MemoryLocation> create(ref<MemoryLocation> loc,
                                    std::set<const Array *> &replacements);

  static ref<MemoryLocation> create(ref<MemoryLocation> loc, ref<Expr> &address,
                                    ref<Expr> &offsetDelta) {
    ConstantExpr *c = llvm::dyn_cast<ConstantExpr>(offsetDelta);
    if (c && c->getZExtValue() == 0) {
      ref<Expr> base = loc->base;
      ref<Expr> offset = loc->offset;
      ref<MemoryLocation> ret(new MemoryLocation(
          loc->value, loc->stack, address, base, offset, loc->size));
      return ret;
    }

    ref<Expr> base = loc->base;
    ref<Expr> newOffset = AddExpr::create(loc->offset, offsetDelta);
    ref<MemoryLocation> ret(new MemoryLocation(loc->value, loc->stack, address,
                                               base, newOffset, loc->size));
    return ret;
  }

  int compareContext(llvm::Value *otherValue,
                     const std::vector<llvm::Instruction *> &_stack) const {
    if (value == otherValue) {
      for (std::vector<llvm::Instruction *>::const_reverse_iterator
               it1 = stack.rbegin(),
               ie1 = stack.rend(), it2 = _stack.rbegin(), ie2 = _stack.rend();
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
    } else if (value < otherValue) {
      return -3;
    }
    return 3;
  }

  bool contextIsPrefixOf(const std::vector<llvm::Instruction *> &stack) const {
    int res = compareContext(value, stack);
    if (res == 0 || res == -1)
      return true;
    return false;
  }

  int compare(const MemoryLocation &other) const {
    int res = compareContext(other.value, other.stack);
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

  const std::vector<llvm::Instruction *> &getStack() const { return stack; }

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
  void print(llvm::raw_ostream &stream) const;
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
  std::vector<llvm::Instruction *> stack;

  /// \brief Do not compute bounds in interpolation of this value if it was a
  /// pointer; instead, use exact address
  bool doNotInterpolateBound;

  /// \brief The load address of this value, in case it was a load instruction
  ref<VersionedValue> loadAddress;

  /// \brief The address by which the loaded value was stored, in case this
  /// was a load instruction
  ref<VersionedValue> storeAddress;

  VersionedValue(llvm::Value *value,
                 const std::vector<llvm::Instruction *> &_stack,
                 ref<Expr> _valueExpr)
      : refCount(0), value(value), valueExpr(_valueExpr), core(false),
        id(reinterpret_cast<uint64_t>(this)), stack(_stack),
        doNotInterpolateBound(false) {}

  /// \brief Print the content of the object, but without showing its source
  /// values.
  ///
  /// \param stream The stream to print the data to.
  void printNoDependency(llvm::raw_ostream &stream) const;

public:
  ~VersionedValue() { locations.clear(); }

  static ref<VersionedValue>
  create(llvm::Value *value, const std::vector<llvm::Instruction *> &stack,
         ref<Expr> valueExpr) {
    ref<VersionedValue> vvalue(new VersionedValue(value, stack, valueExpr));
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

  std::vector<llvm::Instruction *> &getStack() { return stack; }

  /// \brief Print the content of the object into a stream.
  ///
  /// \param stream The stream to print the data to.
  void print(llvm::raw_ostream &stream) const;

  /// \brief Print the content of the object to the LLVM error stream
  void dump() const {
    print(llvm::errs());
    llvm::errs() << "\n";
  }
};
}

#endif
