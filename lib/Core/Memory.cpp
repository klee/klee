//===-- Memory.cpp --------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "Common.h"

#include "Memory.h"

#include "klee/Expr.h"
#include "klee/Machine.h"
#include "klee/Solver.h"
#include "klee/util/BitArray.h"

#include "ObjectHolder.h"

#include <llvm/Function.h>
#include <llvm/Instruction.h>
#include <llvm/Value.h>

#include <iostream>
#include <cassert>
#include <sstream>

using namespace llvm;
using namespace klee;

/***/

ObjectHolder::ObjectHolder(const ObjectHolder &b) : os(b.os) { 
  if (os) ++os->refCount; 
}

ObjectHolder::ObjectHolder(ObjectState *_os) : os(_os) { 
  if (os) ++os->refCount; 
}

ObjectHolder::~ObjectHolder() { 
  if (os && --os->refCount==0) delete os; 
}
  
ObjectHolder &ObjectHolder::operator=(const ObjectHolder &b) {
  if (b.os) ++b.os->refCount;
  if (os && --os->refCount==0) delete os;
  os = b.os;
  return *this;
}

/***/

int MemoryObject::counter = 0;

extern "C" void vc_DeleteExpr(void*);

MemoryObject::~MemoryObject() {
  // FIXME: This shouldn't be necessary. Array's should be ref-counted
  // just like everything else, and the interaction with the STP array
  // should hide at least inside the Expr/Solver layers.
  if (array) {
    if (array->stpInitialArray) {
      ::vc_DeleteExpr(array->stpInitialArray);
      array->stpInitialArray = 0;
    }
    delete array;
  }
}

void MemoryObject::getAllocInfo(std::string &result) const {
  std::ostringstream info;

  info << "MO" << id << "[" << size << "]";

  if (allocSite) {
    info << " allocated at ";
    if (const Instruction *i = dyn_cast<Instruction>(allocSite)) {
      info << i->getParent()->getParent()->getName() << "():";
      info << *i;
    } else if (const GlobalValue *gv = dyn_cast<GlobalValue>(allocSite)) {
      info << "global:" << gv->getName();
    } else {
      info << "value:" << *allocSite;
    }
  } else {
    info << " (no allocation info)";
  }
  
  result = info.str();
}

/***/

ObjectState::ObjectState(const MemoryObject *mo, unsigned _size)
  : copyOnWriteOwner(0),
    refCount(0),
    object(mo),
    concreteStore(new uint8_t[_size]),
    concreteMask(0),
    flushMask(0),
    knownSymbolics(0),
    size(_size),
    updates(mo->array, false, 0),
    readOnly(false) {
}

ObjectState::ObjectState(const ObjectState &os) 
  : copyOnWriteOwner(0),
    refCount(0),
    object(os.object),
    concreteStore(new uint8_t[os.size]),
    concreteMask(os.concreteMask ? new BitArray(*os.concreteMask, os.size) : 0),
    flushMask(os.flushMask ? new BitArray(*os.flushMask, os.size) : 0),
    knownSymbolics(0),
    size(os.size),
    updates(os.updates),
    readOnly(false) {
  assert(!os.readOnly && "no need to copy read only object?");

  if (os.knownSymbolics) {
    knownSymbolics = new ref<Expr>[size];
    for (unsigned i=0; i<size; i++)
      knownSymbolics[i] = os.knownSymbolics[i];
  }

  memcpy(concreteStore, os.concreteStore, size*sizeof(*concreteStore));
}

ObjectState::~ObjectState() {
  if (concreteMask) delete concreteMask;
  if (flushMask) delete flushMask;
  if (knownSymbolics) delete[] knownSymbolics;
  delete[] concreteStore;
}

/***/

void ObjectState::makeConcrete() {
  if (concreteMask) delete concreteMask;
  if (flushMask) delete flushMask;
  if (knownSymbolics) delete[] knownSymbolics;
  concreteMask = 0;
  flushMask = 0;
  knownSymbolics = 0;
}

void ObjectState::makeSymbolic() {
  assert(!updates.head &&
         "XXX makeSymbolic of objects with symbolic values is unsupported");
  updates.isRooted = true;

  // XXX simplify this, can just delete various arrays I guess
  for (unsigned i=0; i<size; i++) {
    markByteSymbolic(i);
    setKnownSymbolic(i, 0);
    markByteFlushed(i);
  }
}

void ObjectState::initializeToZero() {
  makeConcrete();
  memset(concreteStore, 0, size);
}

void ObjectState::initializeToRandom() {  
  makeConcrete();
  for (unsigned i=0; i<size; i++) {
    // randomly selected by 256 sided die
    concreteStore[i] = 0xAB;
  }
}

/*
Cache Invariants
--
isByteKnownSymbolic(i) => !isByteConcrete(i)
isByteConcrete(i) => !isByteKnownSymbolic(i)
!isByteFlushed(i) => (isByteConcrete(i) || isByteKnownSymbolic(i))
 */

void ObjectState::fastRangeCheckOffset(ref<Expr> offset,
                                       unsigned *base_r,
                                       unsigned *size_r) const {
  *base_r = 0;
  *size_r = size;
}

void ObjectState::flushRangeForRead(unsigned rangeBase, 
                                    unsigned rangeSize) const {
  if (!flushMask) flushMask = new BitArray(size, true);
 
  for (unsigned offset=rangeBase; offset<rangeBase+rangeSize; offset++) {
    if (!isByteFlushed(offset)) {
      if (isByteConcrete(offset)) {
        updates.extend(ConstantExpr::create(offset, kMachinePointerType),
                       ConstantExpr::create(concreteStore[offset], Expr::Int8));
      } else {
        assert(isByteKnownSymbolic(offset) && "invalid bit set in flushMask");
        updates.extend(ConstantExpr::create(offset, kMachinePointerType),
                       knownSymbolics[offset]);
      }

      flushMask->unset(offset);
    }
  } 
}

void ObjectState::flushRangeForWrite(unsigned rangeBase, 
                                     unsigned rangeSize) {
  if (!flushMask) flushMask = new BitArray(size, true);

  for (unsigned offset=rangeBase; offset<rangeBase+rangeSize; offset++) {
    if (!isByteFlushed(offset)) {
      if (isByteConcrete(offset)) {
        updates.extend(ConstantExpr::create(offset, kMachinePointerType),
                       ConstantExpr::create(concreteStore[offset], Expr::Int8));
        markByteSymbolic(offset);
      } else {
        assert(isByteKnownSymbolic(offset) && "invalid bit set in flushMask");
        updates.extend(ConstantExpr::create(offset, kMachinePointerType),
                       knownSymbolics[offset]);
        setKnownSymbolic(offset, 0);
      }

      flushMask->unset(offset);
    } else {
      // flushed bytes that are written over still need
      // to be marked out
      if (isByteConcrete(offset)) {
        markByteSymbolic(offset);
      } else if (isByteKnownSymbolic(offset)) {
        setKnownSymbolic(offset, 0);
      }
    }
  } 
}

bool ObjectState::isByteConcrete(unsigned offset) const {
  return !concreteMask || concreteMask->get(offset);
}

bool ObjectState::isByteFlushed(unsigned offset) const {
  return flushMask && !flushMask->get(offset);
}

bool ObjectState::isByteKnownSymbolic(unsigned offset) const {
  return knownSymbolics && knownSymbolics[offset].get();
}

void ObjectState::markByteConcrete(unsigned offset) {
  if (concreteMask)
    concreteMask->set(offset);
}

void ObjectState::markByteSymbolic(unsigned offset) {
  if (!concreteMask)
    concreteMask = new BitArray(size, true);
  concreteMask->unset(offset);
}

void ObjectState::markByteUnflushed(unsigned offset) {
  if (flushMask)
    flushMask->set(offset);
}

void ObjectState::markByteFlushed(unsigned offset) {
  if (!flushMask) {
    flushMask = new BitArray(size, false);
  } else {
    flushMask->unset(offset);
  }
}

void ObjectState::setKnownSymbolic(unsigned offset, 
                                   Expr *value /* can be null */) {
  if (knownSymbolics) {
    knownSymbolics[offset] = value;
  } else {
    if (value) {
      knownSymbolics = new ref<Expr>[size];
      knownSymbolics[offset] = value;
    }
  }
}

/***/

ref<Expr> ObjectState::read8(unsigned offset) const {
  if (isByteConcrete(offset)) {
    return ConstantExpr::create(concreteStore[offset], Expr::Int8);
  } else if (isByteKnownSymbolic(offset)) {
    return knownSymbolics[offset];
  } else {
    assert(isByteFlushed(offset) && "unflushed byte without cache value");
    
    return ReadExpr::create(updates, 
                            ConstantExpr::create(offset, kMachinePointerType));
  }    
}

ref<Expr> ObjectState::read8(ref<Expr> offset) const {
  assert(!isa<ConstantExpr>(offset) && "constant offset passed to symbolic read8");
  unsigned base, size;
  fastRangeCheckOffset(offset, &base, &size);
  flushRangeForRead(base, size);

  if (size>4096) {
    std::string allocInfo;
    object->getAllocInfo(allocInfo);
    klee_warning_once(0, "flushing %d bytes on read, may be slow and/or crash: %s", 
                      size,
                      allocInfo.c_str());
  }
  
  return ReadExpr::create(updates, offset);
}

void ObjectState::write8(unsigned offset, uint8_t value) {
  //assert(read_only == false && "writing to read-only object!");
  concreteStore[offset] = value;
  setKnownSymbolic(offset, 0);

  markByteConcrete(offset);
  markByteUnflushed(offset);
}

void ObjectState::write8(unsigned offset, ref<Expr> value) {
  // can happen when ExtractExpr special cases
  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(value)) {
    write8(offset, (uint8_t) CE->getConstantValue());
  } else {
    setKnownSymbolic(offset, value.get());
      
    markByteSymbolic(offset);
    markByteUnflushed(offset);
  }
}

void ObjectState::write8(ref<Expr> offset, ref<Expr> value) {
  assert(!isa<ConstantExpr>(offset) && "constant offset passed to symbolic write8");
  unsigned base, size;
  fastRangeCheckOffset(offset, &base, &size);
  flushRangeForWrite(base, size);

  if (size>4096) {
    std::string allocInfo;
    object->getAllocInfo(allocInfo);
    klee_warning_once(0, "flushing %d bytes on read, may be slow and/or crash: %s", 
                      size,
                      allocInfo.c_str());
  }
  
  updates.extend(offset, value);
}

/***/

ref<Expr> ObjectState::read(ref<Expr> offset, Expr::Width width) const {
  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(offset)) {
    return read((unsigned) CE->getConstantValue(), width);
  } else { 
    switch (width) {
    default: assert(0 && "invalid type");
    case  Expr::Bool: return  read1(offset);
    case  Expr::Int8: return  read8(offset);
    case Expr::Int16: return read16(offset);
    case Expr::Int32: return read32(offset);
    case Expr::Int64: return read64(offset);
    }
  }
}

ref<Expr> ObjectState::read(unsigned offset, Expr::Width width) const {
  switch (width) {
  default: assert(0 && "invalid type");
  case  Expr::Bool: return  read1(offset);
  case  Expr::Int8: return  read8(offset);
  case Expr::Int16: return read16(offset);
  case Expr::Int32: return read32(offset);
  case Expr::Int64: return read64(offset);
  }
}

ref<Expr> ObjectState::read1(unsigned offset) const {
  return ExtractExpr::createByteOff(read8(offset), 0, Expr::Bool);
}

ref<Expr> ObjectState::read1(ref<Expr> offset) const {
  return ExtractExpr::createByteOff(read8(offset), 0, Expr::Bool);
}

ref<Expr> ObjectState::read16(unsigned offset) const {
  if (kMachineByteOrder == machine::MSB) {
    return ConcatExpr::create(read8(offset+0),
			      read8(offset+1));
  } else {
    return ConcatExpr::create(read8(offset+1),
			      read8(offset+0));
  }
}

ref<Expr> ObjectState::read16(ref<Expr> offset) const {
  if (kMachineByteOrder == machine::MSB) {
    return ConcatExpr::create
      (read8(AddExpr::create(offset,
                             ConstantExpr::create(0,
                                                  kMachinePointerType))),
       read8(AddExpr::create(offset,
                             ConstantExpr::create(1,
                                                  kMachinePointerType))));
  } else {
    return ConcatExpr::create
      (read8(AddExpr::create(offset,
                             ConstantExpr::create(1,
                                                  kMachinePointerType))),
       read8(AddExpr::create(offset,
                             ConstantExpr::create(0,
                                                  kMachinePointerType))));
  }
}

ref<Expr> ObjectState::read32(unsigned offset) const {
  if (kMachineByteOrder == machine::MSB) {
    return ConcatExpr::create4(read8(offset+0),
                               read8(offset+1),
                               read8(offset+2),
                               read8(offset+3));
  } else {
    return ConcatExpr::create4(read8(offset+3),
                               read8(offset+2),
                               read8(offset+1),
                               read8(offset+0));
  }
}

ref<Expr> ObjectState::read32(ref<Expr> offset) const {
  if (kMachineByteOrder == machine::MSB) {
    return ConcatExpr::create4
      (read8(AddExpr::create(offset,
                             ConstantExpr::create(0,
                                                  kMachinePointerType))),
       read8(AddExpr::create(offset,
                             ConstantExpr::create(1,
                                                  kMachinePointerType))),
       read8(AddExpr::create(offset,
                             ConstantExpr::create(2,
                                                  kMachinePointerType))),
       read8(AddExpr::create(offset,
                             ConstantExpr::create(3,
                                                  kMachinePointerType))));
  } else {
    return ConcatExpr::create4
      (read8(AddExpr::create(offset,
                             ConstantExpr::create(3,
                                                  kMachinePointerType))),
       read8(AddExpr::create(offset,
                             ConstantExpr::create(2,
                                                  kMachinePointerType))),
       read8(AddExpr::create(offset,
                             ConstantExpr::create(1,
                                                  kMachinePointerType))),
       read8(AddExpr::create(offset,
                             ConstantExpr::create(0,
                                                  kMachinePointerType))));
  }
}

ref<Expr> ObjectState::read64(unsigned offset) const {
  if (kMachineByteOrder == machine::MSB) {
    return ConcatExpr::create8(read8(offset+0),
                               read8(offset+1),
                               read8(offset+2),
                               read8(offset+3),
                               read8(offset+4),
                               read8(offset+5),
                               read8(offset+6),
                               read8(offset+7));
  } else {
    return ConcatExpr::create8(read8(offset+7),
                               read8(offset+6),
                               read8(offset+5),
                               read8(offset+4),
                               read8(offset+3),
                               read8(offset+2),
                               read8(offset+1),
                               read8(offset+0));
  }
}

ref<Expr> ObjectState::read64(ref<Expr> offset) const {
  if (kMachineByteOrder == machine::MSB) {
    return ConcatExpr::create8
      (read8(AddExpr::create(offset,
                             ConstantExpr::create(0,
                                                  kMachinePointerType))),
       read8(AddExpr::create(offset,
                             ConstantExpr::create(1,
                                                  kMachinePointerType))),
       read8(AddExpr::create(offset,
                             ConstantExpr::create(2,
                                                  kMachinePointerType))),
       read8(AddExpr::create(offset,
                             ConstantExpr::create(3,
                                                  kMachinePointerType))),
       read8(AddExpr::create(offset,
                             ConstantExpr::create(4,
                                                  kMachinePointerType))),
       read8(AddExpr::create(offset,
                             ConstantExpr::create(5,
                                                  kMachinePointerType))),
       read8(AddExpr::create(offset,
                             ConstantExpr::create(6,
                                                  kMachinePointerType))),
       read8(AddExpr::create(offset,
                             ConstantExpr::create(7,
                                                  kMachinePointerType))));
  } else {
    return ConcatExpr::create8
      (read8(AddExpr::create(offset,
                             ConstantExpr::create(7,
                                                  kMachinePointerType))),
       read8(AddExpr::create(offset,
                             ConstantExpr::create(6,
                                                  kMachinePointerType))),
       read8(AddExpr::create(offset,
                             ConstantExpr::create(5,
                                                  kMachinePointerType))),
       read8(AddExpr::create(offset,
                             ConstantExpr::create(4,
                                                  kMachinePointerType))),
       read8(AddExpr::create(offset,
                             ConstantExpr::create(3,
                                                  kMachinePointerType))),
       read8(AddExpr::create(offset,
                             ConstantExpr::create(2,
                                                  kMachinePointerType))),
       read8(AddExpr::create(offset,
                             ConstantExpr::create(1,
                                                  kMachinePointerType))),
       read8(AddExpr::create(offset,
                             ConstantExpr::create(0,
                                                  kMachinePointerType))));
  }
}

void ObjectState::write(ref<Expr> offset, ref<Expr> value) {
  Expr::Width w = value->getWidth();
  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(offset)) {
    write(CE->getConstantValue(), value);
  } else {
    switch(w) {
    case  Expr::Bool:  write1(offset, value); break;
    case  Expr::Int8:  write8(offset, value); break;
    case Expr::Int16: write16(offset, value); break;
    case Expr::Int32: write32(offset, value); break;
    case Expr::Int64: write64(offset, value); break;
    default: assert(0 && "invalid number of bytes in write");
    }
  }
}

void ObjectState::write(unsigned offset, ref<Expr> value) {
  Expr::Width w = value->getWidth();
  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(value)) {
    uint64_t val = CE->getConstantValue();
    switch(w) {
    case  Expr::Bool:
    case  Expr::Int8:  write8(offset, val); break;
    case Expr::Int16: write16(offset, val); break;
    case Expr::Int32: write32(offset, val); break;
    case Expr::Int64: write64(offset, val); break;
    default: assert(0 && "invalid number of bytes in write");
    }
  } else {
    switch(w) {
    case  Expr::Bool:  write1(offset, value); break;
    case  Expr::Int8:  write8(offset, value); break;
    case Expr::Int16: write16(offset, value); break;
    case Expr::Int32: write32(offset, value); break;
    case Expr::Int64: write64(offset, value); break;
    default: assert(0 && "invalid number of bytes in write");
    }
  }
}

void ObjectState::write1(unsigned offset, ref<Expr> value) {
  write8(offset, ZExtExpr::create(value, Expr::Int8));
}

void ObjectState::write1(ref<Expr> offset, ref<Expr> value) {
  write8(offset, ZExtExpr::create(value, Expr::Int8));
}

void ObjectState::write16(unsigned offset, uint16_t value) {
  if (kMachineByteOrder == machine::MSB) {
    write8(offset+0, (uint8_t) (value >>  8));
    write8(offset+1, (uint8_t) (value >>  0));
  } else {
    write8(offset+1, (uint8_t) (value >>  8));
    write8(offset+0, (uint8_t) (value >>  0));
  }
}

void ObjectState::write16(unsigned offset, ref<Expr> value) {
  if (kMachineByteOrder == machine::MSB) {
    write8(offset+0, ExtractExpr::createByteOff(value, 1));
    write8(offset+1, ExtractExpr::createByteOff(value, 0));
  } else {
    write8(offset+1, ExtractExpr::createByteOff(value, 1));
    write8(offset+0, ExtractExpr::createByteOff(value, 0));
  }
}


void ObjectState::write16(ref<Expr> offset, ref<Expr> value) {
  if (kMachineByteOrder == machine::MSB) {
    write8(AddExpr::create(offset,
                           ConstantExpr::create(0, kMachinePointerType)), 
           ExtractExpr::createByteOff(value,1));
    write8(AddExpr::create(offset,
                           ConstantExpr::create(0, kMachinePointerType)), 
           ExtractExpr::createByteOff(value,0));
  } else {
    write8(AddExpr::create(offset,
                           ConstantExpr::create(1, kMachinePointerType)), 
           ExtractExpr::createByteOff(value,1));
    write8(AddExpr::create(offset,
                           ConstantExpr::create(0, kMachinePointerType)), 
           ExtractExpr::createByteOff(value,0));
  }
}

void ObjectState::write32(unsigned offset, uint32_t value) {
  if (kMachineByteOrder == machine::MSB) {
    write8(offset+0, (uint8_t) (value >>  24));
    write8(offset+1, (uint8_t) (value >>  16));
    write8(offset+2, (uint8_t) (value >>  8));
    write8(offset+3, (uint8_t) (value >>  0));
  } else {
    write8(offset+3, (uint8_t) (value >>  24));
    write8(offset+2, (uint8_t) (value >>  16));
    write8(offset+1, (uint8_t) (value >>  8));
    write8(offset+0, (uint8_t) (value >>  0));
  }
}

void ObjectState::write32(unsigned offset, ref<Expr> value) {
  if (kMachineByteOrder == machine::MSB) {
    write8(offset+0, ExtractExpr::createByteOff(value, 3));
    write8(offset+1, ExtractExpr::createByteOff(value, 2));
    write8(offset+2, ExtractExpr::createByteOff(value, 1));
    write8(offset+3, ExtractExpr::createByteOff(value, 0));
  } else {
    write8(offset+3, ExtractExpr::createByteOff(value, 3));
    write8(offset+2, ExtractExpr::createByteOff(value, 2));
    write8(offset+1, ExtractExpr::createByteOff(value, 1));
    write8(offset+0, ExtractExpr::createByteOff(value, 0));
  }
}

void ObjectState::write32(ref<Expr> offset, ref<Expr> value) {
  if (kMachineByteOrder == machine::MSB) {
    write8(AddExpr::create(offset,
                           ConstantExpr::create(0, kMachinePointerType)), 
           ExtractExpr::createByteOff(value,3));
    write8(AddExpr::create(offset,
                           ConstantExpr::create(1, kMachinePointerType)), 
           ExtractExpr::createByteOff(value,2));
    write8(AddExpr::create(offset,
                           ConstantExpr::create(2, kMachinePointerType)), 
           ExtractExpr::createByteOff(value,1));
    write8(AddExpr::create(offset,
                           ConstantExpr::create(3, kMachinePointerType)), 
           ExtractExpr::createByteOff(value,0));
  } else {
    write8(AddExpr::create(offset,
                           ConstantExpr::create(3, kMachinePointerType)), 
           ExtractExpr::createByteOff(value,3));
    write8(AddExpr::create(offset,
                           ConstantExpr::create(2, kMachinePointerType)), 
           ExtractExpr::createByteOff(value,2));
    write8(AddExpr::create(offset,
                           ConstantExpr::create(1, kMachinePointerType)), 
           ExtractExpr::createByteOff(value,1));
    write8(AddExpr::create(offset,
                           ConstantExpr::create(0, kMachinePointerType)), 
           ExtractExpr::createByteOff(value,0));
  }
}

void ObjectState::write64(unsigned offset, uint64_t value) {
  if (kMachineByteOrder == machine::MSB) {
    write8(offset+0, (uint8_t) (value >>  56));
    write8(offset+1, (uint8_t) (value >>  48));
    write8(offset+2, (uint8_t) (value >>  40));
    write8(offset+3, (uint8_t) (value >>  32));
    write8(offset+4, (uint8_t) (value >>  24));
    write8(offset+5, (uint8_t) (value >>  16));
    write8(offset+6, (uint8_t) (value >>  8));
    write8(offset+7, (uint8_t) (value >>  0));
  } else {
    write8(offset+7, (uint8_t) (value >>  56));
    write8(offset+6, (uint8_t) (value >>  48));
    write8(offset+5, (uint8_t) (value >>  40));
    write8(offset+4, (uint8_t) (value >>  32));
    write8(offset+3, (uint8_t) (value >>  24));
    write8(offset+2, (uint8_t) (value >>  16));
    write8(offset+1, (uint8_t) (value >>  8));
    write8(offset+0, (uint8_t) (value >>  0));
  }
}

void ObjectState::write64(unsigned offset, ref<Expr> value) {
  if (kMachineByteOrder == machine::MSB) {
    write8(offset+0, ExtractExpr::createByteOff(value, 7));
    write8(offset+1, ExtractExpr::createByteOff(value, 6));
    write8(offset+2, ExtractExpr::createByteOff(value, 5));
    write8(offset+3, ExtractExpr::createByteOff(value, 4));
    write8(offset+4, ExtractExpr::createByteOff(value, 3));
    write8(offset+5, ExtractExpr::createByteOff(value, 2));
    write8(offset+6, ExtractExpr::createByteOff(value, 1));
    write8(offset+7, ExtractExpr::createByteOff(value, 0));
  } else {
    write8(offset+7, ExtractExpr::createByteOff(value, 7));
    write8(offset+6, ExtractExpr::createByteOff(value, 6));
    write8(offset+5, ExtractExpr::createByteOff(value, 5));
    write8(offset+4, ExtractExpr::createByteOff(value, 4));
    write8(offset+3, ExtractExpr::createByteOff(value, 3));
    write8(offset+2, ExtractExpr::createByteOff(value, 2));
    write8(offset+1, ExtractExpr::createByteOff(value, 1));
    write8(offset+0, ExtractExpr::createByteOff(value, 0));
  }
}

void ObjectState::write64(ref<Expr> offset, ref<Expr> value) {
  if (kMachineByteOrder == machine::MSB) {
    write8(AddExpr::create(offset,
                           ConstantExpr::create(0, kMachinePointerType)), 
           ExtractExpr::createByteOff(value,7));
    write8(AddExpr::create(offset,
                           ConstantExpr::create(1, kMachinePointerType)), 
           ExtractExpr::createByteOff(value,6));
    write8(AddExpr::create(offset,
                           ConstantExpr::create(2, kMachinePointerType)), 
           ExtractExpr::createByteOff(value,5));
    write8(AddExpr::create(offset,
                           ConstantExpr::create(3, kMachinePointerType)), 
           ExtractExpr::createByteOff(value,4));
    write8(AddExpr::create(offset,
                           ConstantExpr::create(4, kMachinePointerType)), 
           ExtractExpr::createByteOff(value,3));
    write8(AddExpr::create(offset,
                           ConstantExpr::create(5, kMachinePointerType)), 
           ExtractExpr::createByteOff(value,2));
    write8(AddExpr::create(offset,
                           ConstantExpr::create(6, kMachinePointerType)), 
           ExtractExpr::createByteOff(value,1));
    write8(AddExpr::create(offset,
                           ConstantExpr::create(7, kMachinePointerType)), 
           ExtractExpr::createByteOff(value,0));
  } else {
    write8(AddExpr::create(offset,
                           ConstantExpr::create(7, kMachinePointerType)), 
           ExtractExpr::createByteOff(value,7));
    write8(AddExpr::create(offset,
                           ConstantExpr::create(6, kMachinePointerType)), 
           ExtractExpr::createByteOff(value,6));
    write8(AddExpr::create(offset,
                           ConstantExpr::create(5, kMachinePointerType)), 
           ExtractExpr::createByteOff(value,5));
    write8(AddExpr::create(offset,
                           ConstantExpr::create(4, kMachinePointerType)), 
           ExtractExpr::createByteOff(value,4));
    write8(AddExpr::create(offset,
                           ConstantExpr::create(3, kMachinePointerType)), 
           ExtractExpr::createByteOff(value,3));
    write8(AddExpr::create(offset,
                           ConstantExpr::create(2, kMachinePointerType)), 
           ExtractExpr::createByteOff(value,2));
    write8(AddExpr::create(offset,
                           ConstantExpr::create(1, kMachinePointerType)), 
           ExtractExpr::createByteOff(value,1));
    write8(AddExpr::create(offset,
                           ConstantExpr::create(0, kMachinePointerType)), 
           ExtractExpr::createByteOff(value,0));
  }
}

void ObjectState::print() {
  llvm::cerr << "-- ObjectState --\n";
  llvm::cerr << "\tMemoryObject ID: " << object->id << "\n";
  llvm::cerr << "\tRoot Object: " << updates.root << "\n";
  llvm::cerr << "\tIs Rooted? " << updates.isRooted << "\n";
  llvm::cerr << "\tSize: " << size << "\n";

  llvm::cerr << "\tBytes:\n";
  for (unsigned i=0; i<size; i++) {
    llvm::cerr << "\t\t["<<i<<"]"
               << " concrete? " << isByteConcrete(i)
               << " known-sym? " << isByteKnownSymbolic(i)
               << " flushed? " << isByteFlushed(i) << " = ";
    ref<Expr> e = read8(i);
    llvm::cerr << e << "\n";
  }

  llvm::cerr << "\tUpdates:\n";
  for (const UpdateNode *un=updates.head; un; un=un->next) {
    llvm::cerr << "\t\t[" << un->index << "] = " << un->value << "\n";
  }
}
