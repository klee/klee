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

#include "Context.h"
#include "TimingSolver.h"

#include "klee/Expr/Expr.h"

#include "llvm/ADT/StringExtras.h"

#include <string>
#include <vector>

namespace llvm {
  class Value;
}

namespace klee {

class ArrayCache;
class BitArray;
class ExecutionState;
class Executor;
class MemoryManager;
class Solver;

class MemoryObject {
  friend class STPBuilder;
  friend class ObjectState;
  friend class ExecutionState;
  friend class ref<MemoryObject>;
  friend class ref<const MemoryObject>;

private:
  static int counter;
  /// @brief Required by klee::ref-managed objects
  mutable class ReferenceCounter _refCount;

public:
  unsigned id;
  uint64_t address;

  /// size in bytes
  unsigned size;
  unsigned alignment;
  mutable std::string name;

  bool isLocal;
  mutable bool isGlobal;
  bool isFixed;

  bool isUserSpecified;

  MemoryManager *parent;

  /// "Location" for which this memory object was allocated. This
  /// should be either the allocating instruction or the global object
  /// it was allocated for (or whatever else makes sense).
  const llvm::Value *allocSite;

  // DO NOT IMPLEMENT
  MemoryObject(const MemoryObject &b);
  MemoryObject &operator=(const MemoryObject &b);

public:
  // XXX this is just a temp hack, should be removed
  explicit
  MemoryObject(uint64_t _address) 
    : id(counter++),
      address(_address),
      size(0),
      alignment(0),
      isFixed(true),
      parent(NULL),
      allocSite(0) {
  }

  MemoryObject(uint64_t _address, unsigned _size, unsigned _alignment,
               bool _isLocal, bool _isGlobal, bool _isFixed,
               const llvm::Value *_allocSite,
               MemoryManager *_parent)
    : id(counter++),
      address(_address),
      size(_size),
      alignment(_alignment),
      name("unnamed"),
      isLocal(_isLocal),
      isGlobal(_isGlobal),
      isFixed(_isFixed),
      isUserSpecified(false),
      parent(_parent), 
      allocSite(_allocSite) {
  }

  ~MemoryObject();

  /// Get an identifying string for this allocation.
  void getAllocInfo(std::string &result) const;

  void setName(std::string name) const {
    this->name = name;
  }

  ref<ConstantExpr> getBaseExpr() const { 
    return ConstantExpr::create(address, Context::get().getPointerWidth());
  }
  ref<ConstantExpr> getSizeExpr() const { 
    return ConstantExpr::create(size, Context::get().getPointerWidth());
  }
  ref<Expr> getOffsetExpr(ref<Expr> pointer) const {
    return SubExpr::create(pointer, getBaseExpr());
  }
  ref<Expr> getBoundsCheckPointer(ref<Expr> pointer) const {
    return getBoundsCheckOffset(getOffsetExpr(pointer));
  }
  ref<Expr> getBoundsCheckPointer(ref<Expr> pointer, unsigned bytes) const {
    return getBoundsCheckOffset(getOffsetExpr(pointer), bytes);
  }

  ref<Expr> getBoundsCheckOffset(ref<Expr> offset) const {
    if (size==0) {
      return EqExpr::create(offset, 
                            ConstantExpr::alloc(0, Context::get().getPointerWidth()));
    } else {
      return UltExpr::create(offset, getSizeExpr());
    }
  }
  ref<Expr> getBoundsCheckOffset(ref<Expr> offset, unsigned bytes) const {
    if (bytes<=size) {
      return UltExpr::create(offset, 
                             ConstantExpr::alloc(size - bytes + 1, 
                                                 Context::get().getPointerWidth()));
    } else {
      return ConstantExpr::alloc(0, Expr::Bool);
    }
  }

  /// Compare this object with memory object b.
  /// \param b memory object to compare with
  /// \return <0 if this is smaller, 0 if both are equal, >0 if b is smaller
  int compare(const MemoryObject &b) const {
    // Short-cut with id
    if (id == b.id)
      return 0;
    if (address != b.address)
      return (address < b.address ? -1 : 1);

    if (size != b.size)
      return (size < b.size ? -1 : 1);

    if (allocSite != b.allocSite)
      return (allocSite < b.allocSite ? -1 : 1);

    return 0;
  }
};

class ObjectState {
private:
  friend class AddressSpace;
  friend class ref<ObjectState>;

  unsigned copyOnWriteOwner; // exclusively for AddressSpace

  /// @brief Required by klee::ref-managed objects
  class ReferenceCounter _refCount;

  ref<const MemoryObject> object;

  /// @brief Holds all known concrete bytes
  uint8_t *concreteStore;

  /// @brief concreteMask[byte] is set if byte is known to be concrete
  BitArray *concreteMask;

  /// knownSymbolics[byte] holds the symbolic expression for byte,
  /// if byte is known to be symbolic
  ref<Expr> *knownSymbolics;

  /// unflushedMask[byte] is set if byte is unflushed
  /// mutable because may need flushed during read of const
  mutable BitArray *unflushedMask;

  // mutable because we may need flush during read of const
  mutable UpdateList updates;

public:
  unsigned size;

  bool readOnly;

public:
  /// Create a new object state for the given memory object with concrete
  /// contents. The initial contents are undefined, it is the callers
  /// responsibility to initialize the object contents appropriately.
  ObjectState(const MemoryObject *mo);

  /// Create a new object state for the given memory object with symbolic
  /// contents.
  ObjectState(const MemoryObject *mo, const Array *array);

  ObjectState(const ObjectState &os);
  ~ObjectState();

  const MemoryObject *getObject() const { return object.get(); }

  void setReadOnly(bool ro) { readOnly = ro; }

  /// Make contents all concrete and zero
  void initializeToZero();

  /// Make contents all concrete and random
  void initializeToRandom();

  ref<Expr> read(ref<Expr> offset, Expr::Width width) const;
  ref<Expr> read(unsigned offset, Expr::Width width) const;
  ref<Expr> read8(unsigned offset) const;

  void write(unsigned offset, ref<Expr> value);
  void write(ref<Expr> offset, ref<Expr> value);

  void write8(unsigned offset, uint8_t value);
  void write16(unsigned offset, uint16_t value);
  void write32(unsigned offset, uint32_t value);
  void write64(unsigned offset, uint64_t value);
  void print() const;

  /// Generate concrete values for each symbolic byte of the object and put them
  /// in the concrete store.
  ///
  /// \param executor
  /// \param state
  /// \param concretize if true, constraints for concretised bytes are added if
  /// necessary
  void flushToConcreteStore(Executor &executor, ExecutionState &state,
                            bool concretize);

private:
  const UpdateList &getUpdates() const;

  void makeConcrete();

  void makeSymbolic();

  ref<Expr> read8(ref<Expr> offset) const;
  void write8(unsigned offset, ref<Expr> value);
  void write8(ref<Expr> offset, ref<Expr> value);

  void fastRangeCheckOffset(ref<Expr> offset, unsigned *base_r, 
                            unsigned *size_r) const;
  void flushRangeForRead(unsigned rangeBase, unsigned rangeSize) const;
  void flushRangeForWrite(unsigned rangeBase, unsigned rangeSize);

  /// isByteConcrete ==> !isByteKnownSymbolic
  bool isByteConcrete(unsigned offset) const;

  /// isByteKnownSymbolic ==> !isByteConcrete
  bool isByteKnownSymbolic(unsigned offset) const;

  /// isByteUnflushed(i) => (isByteConcrete(i) || isByteKnownSymbolic(i))
  bool isByteUnflushed(unsigned offset) const;

  void markByteConcrete(unsigned offset);
  void markByteSymbolic(unsigned offset);
  void markByteFlushed(unsigned offset);
  void markByteUnflushed(unsigned offset);
  void setKnownSymbolic(unsigned offset, Expr *value);

  ArrayCache *getArrayCache() const;
};
  
} // End klee namespace

#endif /* KLEE_MEMORY_H */
