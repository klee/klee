//===-- Memory.h ------------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_MEMORY_H
#define KLEE_MEMORY_H

#include "CodeLocation.h"
#include "MemoryManager.h"
#include "klee/ADT/Ref.h"
#include "klee/ADT/SparseStorage.h"

#include "klee/Expr/Assignment.h"
#include "klee/Expr/Expr.h"
#include "klee/Expr/SourceBuilder.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>

namespace llvm {
class Value;
}

namespace klee {

class BitArray;
class ExecutionState;
class KType;
class MemoryManager;
class Solver;

typedef uint64_t IDType;

extern llvm::cl::opt<bool> UseTypeBasedAliasAnalysis;
extern llvm::cl::opt<unsigned long> MaxFixedSizeStructureSize;

class MemoryObject {
  friend class STPBuilder;
  friend class ObjectState;
  friend class ExecutionState;
  friend class ref<MemoryObject>;
  friend class ref<const MemoryObject>;

private:
  // Counter is using for id's of MemoryObjects.
  //
  // Value 0 is reserved for erroneous objects.
  static IDType counter;
  static int time;
  /// @brief Required by klee::ref-managed objects
  mutable class ReferenceCounter _refCount;

public:
  IDType id;
  mutable unsigned timestamp;

  ref<Expr> addressExpr;
  std::optional<uint64_t> address;

  /// size in bytes
  ref<Expr> sizeExpr;

  ref<Expr> conditionExpr;

  uint64_t alignment;

  mutable std::string name;

  bool isLocal;
  mutable bool isGlobal;
  bool isFixed;
  bool isLazyInitialized;

  bool isUserSpecified;

  MemoryManager *parent;
  KType *type;
  const Array *content;

  /// "Location" for which this memory object was allocated. This
  /// should be either the allocating instruction or the global object
  /// it was allocated for (or whatever else makes sense).
  ref<CodeLocation> allocSite;

  // DO NOT IMPLEMENT
  MemoryObject(const MemoryObject &b);
  MemoryObject &operator=(const MemoryObject &b);

public:
  // XXX this is just a temp hack, should be removed
  explicit MemoryObject(uint64_t _address)
      : id(0), timestamp(0), addressExpr(Expr::createPointer(_address)),
        address(_address), sizeExpr(Expr::createPointer(0)),
        conditionExpr(Expr::createTrue()), alignment(0), isFixed(true),
        isLazyInitialized(false), parent(nullptr), type(nullptr),
        content(nullptr), allocSite(nullptr) {}

  MemoryObject(
      ref<Expr> _address, ref<Expr> _size, uint64_t alignment, bool _isLocal,
      bool _isGlobal, bool _isFixed, bool _isLazyInitialized,
      ref<CodeLocation> _allocSite, MemoryManager *_parent, KType *_type,
      ref<Expr> _condition = Expr::createTrue(),
      unsigned _timestamp = 0 /* unused if _isLazyInitialized is false*/,
      const Array *_content =
          nullptr /* unused if _isLazyInitialized is false*/)
      : id(counter++), timestamp(_timestamp), addressExpr(_address),
        sizeExpr(_size), conditionExpr(_condition), alignment(alignment),
        name("unnamed"), isLocal(_isLocal), isGlobal(_isGlobal),
        isFixed(_isFixed), isLazyInitialized(_isLazyInitialized),
        isUserSpecified(false), parent(_parent), type(_type), content(_content),
        allocSite(_allocSite) {
    if (isLazyInitialized) {
      timestamp = _timestamp;
    } else {
      timestamp = time++;
    }
    if (auto constAddress = dyn_cast<ConstantExpr>(_address)) {
      address = constAddress->getZExtValue();
    }
  }

  ~MemoryObject();

  /// Get an identifying string for this allocation.
  std::string getAllocInfo() const;

  void setName(const std::string &_name) const { this->name = _name; }

  void updateTimestamp() const { this->timestamp = time++; }

  bool hasSymbolicSize() const { return !isa<ConstantExpr>(getSizeExpr()); }
  ref<Expr> getBaseExpr() const { return addressExpr; }
  ref<Expr> getSizeExpr() const { return sizeExpr; }
  ref<Expr> getConditionExpr() const { return conditionExpr; }
  ref<Expr> getOffsetExpr(ref<Expr> pointer) const {
    return SubExpr::create(pointer, getBaseExpr());
  }
  ref<Expr> getOffsetExpr(ref<PointerExpr> pointer) const {
    return SubExpr::create(pointer->getValue(), getBaseExpr());
  }

  ref<Expr> getBoundsCheckOffset(ref<Expr> offset) const {
    ref<Expr> isZeroSizeExpr =
        EqExpr::create(Expr::createPointer(0), getSizeExpr());
    ref<Expr> isZeroOffsetExpr = EqExpr::create(Expr::createPointer(0), offset);
    return SelectExpr::create(isZeroSizeExpr, isZeroOffsetExpr,
                              UltExpr::create(offset, getSizeExpr()));
  }

  ref<Expr> getBoundsCheckOffset(ref<Expr> offset, unsigned bytes) const {
    ref<Expr> offsetSizeCheck =
        UleExpr::create(Expr::createPointer(bytes), getSizeExpr());
    ref<Expr> trueExpr = UltExpr::create(
        offset, AddExpr::create(
                    SubExpr::create(getSizeExpr(), Expr::createPointer(bytes)),
                    Expr::createPointer(1)));
    return SelectExpr::create(offsetSizeCheck, trueExpr, Expr::createFalse());
  }

  ref<Expr> getBoundsCheckAddress(ref<Expr> address) const {
    return getBoundsCheckOffset(getOffsetExpr(address));
  }
  ref<Expr> getBoundsCheckAddress(ref<Expr> address, unsigned bytes) const {
    return getBoundsCheckOffset(getOffsetExpr(address), bytes);
  }
  ref<Expr> getBaseCheck(ref<Expr> base) const {
    return EqExpr::create(base, getBaseExpr());
  }

  ref<Expr> getBoundsCheckPointer(ref<PointerExpr> pointer) const {
    ref<Expr> condition = getBaseCheck(pointer->getBase());
    condition =
        AndExpr::create(condition, getBoundsCheckAddress(pointer->getValue()));
    return condition;
  }
  ref<Expr> getBoundsCheckPointer(ref<PointerExpr> pointer,
                                  unsigned bytes) const {
    ref<Expr> condition = getBaseCheck(pointer->getBase());
    condition = AndExpr::create(
        condition, getBoundsCheckAddress(pointer->getValue(), bytes));
    return condition;
  }

  /// Compare this object with memory object b.
  /// \param b memory object to compare with
  /// \return <0 if this is smaller, 0 if both are equal, >0 if b is smaller
  int compare(const MemoryObject &b) const {
    // Short-cut with id
    if (id == b.id) {
      return 0;
    }
    if (addressExpr != b.addressExpr) {
      return (addressExpr < b.addressExpr ? -1 : 1);
    }
    if (sizeExpr != b.sizeExpr) {
      return (sizeExpr < b.sizeExpr ? -1 : 1);
    }
    if (allocSite->source != b.allocSite->source) {
      return (allocSite->source < b.allocSite->source ? -1 : 1);
    }
    assert(isLazyInitialized == b.isLazyInitialized);
    return 0;
  }

  bool equals(const MemoryObject &b) const { return compare(b) == 0; }
};

class ObjectStage {
private:
  using storage_ty = SparseStorage<ref<Expr>, OptionalRefEq<Expr>>;
  using bool_storage_ty = SparseStorage<bool>;
  /// knownSymbolics[byte] holds the expression for byte,
  /// if byte is known
  mutable std::unique_ptr<storage_ty> knownSymbolics;

  /// unflushedMask[byte] is set if byte is unflushed
  /// mutable because may need flushed during read of const
  mutable std::unique_ptr<bool_storage_ty> unflushedMask;

  // mutable because we may need flush during read of const
  mutable UpdateList updates;

  ref<Expr> size;
  bool safeRead;
  Expr::Width width;

public:
  ObjectStage(const Array *array, ref<Expr> defaultValue, bool safe = true,
              Expr::Width width = Expr::Int8);
  ObjectStage(ref<Expr> size, ref<Expr> defaultValue, bool safe = true,
              Expr::Width width = Expr::Int8);

  ObjectStage(const ObjectStage &os);
  ~ObjectStage() = default;

  ref<Expr> readWidth(unsigned offset) const;
  ref<Expr> readWidth(ref<Expr> offset) const;
  void writeWidth(unsigned offset, ref<Expr> value);
  void writeWidth(ref<Expr> offset, ref<Expr> value);
  void write(const ObjectStage &os);

  void write(unsigned offset, ref<Expr> value);
  void write(ref<Expr> offset, ref<Expr> value);

  void writeWidth(unsigned offset, uint64_t value);
  void print() const;

  size_t getSparseStorageEntries() {
    return knownSymbolics->storage().size() + unflushedMask->storage().size();
  }
  void initializeToZero();

private:
  const UpdateList &getUpdates() const;

  void makeConcrete();

  void flushForRead() const;
  void flushForWrite();
};

class ObjectState {
private:
  friend class AddressSpace;
  friend class ref<ObjectState>;
  friend class ref<const ObjectState>;

  unsigned copyOnWriteOwner; // exclusively for AddressSpace

  /// @brief Required by klee::ref-managed objects
  mutable class ReferenceCounter _refCount;

  ref<const MemoryObject> object;

  ObjectStage valueOS;
  ObjectStage baseOS;

  ref<UpdateNode> lastUpdate;

  ref<Expr> size;

  KType *dynamicType;

public:
  bool readOnly;
  bool wasWritten = false;

  /// Create a new object state for the given memory
  // For objects in memory
  ObjectState(const MemoryObject *mo, const Array *array, KType *dt);
  ObjectState(const MemoryObject *mo, KType *dt);

  // For symbolic objects not in memory (hack)

  ObjectState(const ObjectState &os);
  ~ObjectState() = default;

  const MemoryObject *getObject() const { return object.get(); }

  void setReadOnly(bool ro) { readOnly = ro; }
  void initializeToZero();

  size_t getSparseStorageEntries() {
    return valueOS.getSparseStorageEntries() + baseOS.getSparseStorageEntries();
  }

  void swapObjectHack(MemoryObject *mo) { object = mo; }

  ref<Expr> read(ref<Expr> offset, Expr::Width width) const;
  ref<Expr> read(unsigned offset, Expr::Width width) const;
  ref<Expr> read8(unsigned offset) const;
  ref<Expr> readValue(ref<Expr> offset, Expr::Width width) const;
  ref<Expr> readBase(ref<Expr> offset, Expr::Width width) const;
  ref<Expr> readValue(unsigned offset, Expr::Width width) const;
  ref<Expr> readBase(unsigned offset, Expr::Width width) const;
  ref<Expr> readValue8(unsigned offset) const;
  ref<Expr> readBase8(unsigned offset) const;

  void write(unsigned offset, ref<Expr> value);
  void write(ref<Expr> offset, ref<Expr> value);
  void write(ref<const ObjectState> os);

  void write8(unsigned offset, uint8_t value);
  void write16(unsigned offset, uint16_t value);
  void write32(unsigned offset, uint32_t value);
  void write64(unsigned offset, uint64_t value);
  void print() const;

  bool isAccessableFrom(KType *) const;

  KType *getDynamicType() const;

private:
  ref<Expr> read8(ref<Expr> offset) const;
  ref<Expr> readValue8(ref<Expr> offset) const;
  ref<Expr> readBase8(ref<Expr> offset) const;
  void write8(unsigned offset, ref<Expr> value);
  void write8(ref<Expr> offset, ref<Expr> value);
};

} // namespace klee

#endif /* KLEE_MEMORY_H */
