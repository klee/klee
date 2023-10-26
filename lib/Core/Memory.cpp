//===-- Memory.cpp --------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "Memory.h"

#include "ExecutionState.h"
#include "MemoryManager.h"
#include "klee/Core/Context.h"

#include "klee/ADT/BitArray.h"
#include "klee/Expr/ArrayCache.h"
#include "klee/Expr/Assignment.h"
#include "klee/Expr/Expr.h"
#include "klee/Module/KType.h"
#include "klee/Solver/Solver.h"
#include "klee/Support/ErrorHandling.h"
#include "klee/Support/OptionCategories.h"

#include "klee/Support/CompilerWarning.h"
DISABLE_WARNING_PUSH
DISABLE_WARNING_DEPRECATED_DECLARATIONS
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"
DISABLE_WARNING_POP

#include <cassert>
#include <sstream>

using namespace llvm;
using namespace klee;

/***/

IDType MemoryObject::counter = 1;
int MemoryObject::time = 0;

MemoryObject::~MemoryObject() {
  if (parent)
    parent->markFreed(this);
}

void MemoryObject::getAllocInfo(std::string &result) const {
  llvm::raw_string_ostream info(result);

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

  info.flush();
}

/***/

ObjectState::ObjectState(const MemoryObject *mo, const Array *array, KType *dt)
    : copyOnWriteOwner(0), object(mo), knownSymbolics(nullptr),
      unflushedMask(false), updates(array, nullptr), lastUpdate(nullptr),
      size(array->size), dynamicType(dt), readOnly(false) {}

ObjectState::ObjectState(const MemoryObject *mo, KType *dt)
    : copyOnWriteOwner(0), object(mo), knownSymbolics(nullptr),
      unflushedMask(false), updates(nullptr, nullptr), lastUpdate(nullptr),
      size(mo->getSizeExpr()), dynamicType(dt), readOnly(false) {}

ObjectState::ObjectState(const ObjectState &os)
    : copyOnWriteOwner(0), object(os.object), knownSymbolics(os.knownSymbolics),
      unflushedMask(os.unflushedMask), updates(os.updates),
      lastUpdate(os.lastUpdate), size(os.size), dynamicType(os.dynamicType),
      readOnly(os.readOnly) {}

ArrayCache *ObjectState::getArrayCache() const {
  assert(object && "object was NULL");
  return object->parent->getArrayCache();
}

/***/

const UpdateList &ObjectState::getUpdates() const {
  // Constant arrays are created lazily.

  if (auto sizeExpr = dyn_cast<ConstantExpr>(size)) {
    auto size = sizeExpr->getZExtValue();
    if (knownSymbolics.storage().size() == size) {
      SparseStorage<ref<ConstantExpr>> values(
          ConstantExpr::create(0, Expr::Int8));
      UpdateList symbolicUpdates = UpdateList(nullptr, nullptr);
      for (unsigned i = 0; i < size; i++) {
        auto value = knownSymbolics.load(i);
        assert(value);
        if (isa<ConstantExpr>(value)) {
          values.store(i, cast<ConstantExpr>(value));
        } else {
          symbolicUpdates.extend(ConstantExpr::create(i, Expr::Int32), value);
        }
      }
      auto array = getArrayCache()->CreateArray(
          sizeExpr, SourceBuilder::constant(values));
      updates = UpdateList(array, symbolicUpdates.head);
      knownSymbolics.reset();
      unflushedMask.reset();
    }
  }

  if (!updates.root) {
    SparseStorage<ref<ConstantExpr>> values(
        ConstantExpr::create(0, Expr::Int8));
    auto array =
        getArrayCache()->CreateArray(size, SourceBuilder::constant(values));
    updates = UpdateList(array, updates.head);
  }

  assert(updates.root);

  return updates;
}

void ObjectState::flushForRead() const {
  for (const auto &unflushed : unflushedMask.storage()) {
    auto offset = unflushed.first;
    auto value = knownSymbolics.load(offset);
    assert(value);
    updates.extend(ConstantExpr::create(offset, Expr::Int32), value);
  }
  unflushedMask.reset(false);
}

void ObjectState::flushForWrite() {
  flushForRead();
  // The write is symbolic offset and might overwrite any byte
  knownSymbolics.reset(nullptr);
}

/***/

ref<Expr> ObjectState::read8(unsigned offset) const {
  if (auto byte = knownSymbolics.load(offset)) {
    return byte;
  } else {
    assert(!unflushedMask.load(offset) && "unflushed byte without cache value");
    return ReadExpr::create(getUpdates(),
                            ConstantExpr::create(offset, Expr::Int32));
  }
}

ref<Expr> ObjectState::read8(ref<Expr> offset) const {
  assert(!isa<ConstantExpr>(offset) &&
         "constant offset passed to symbolic read8");
  flushForRead();

  if (object && object->size > 4096) {
    std::string allocInfo;
    object->getAllocInfo(allocInfo);
    klee_warning_once(
        nullptr,
        "Symbolic memory access will send the following array of %d bytes to "
        "the constraint solver -- large symbolic arrays may cause significant "
        "performance issues: %s",
        object->size, allocInfo.c_str());
  }

  return ReadExpr::create(getUpdates(), ZExtExpr::create(offset, Expr::Int32));
}

void ObjectState::write8(unsigned offset, uint8_t value) {
  knownSymbolics.store(offset, ConstantExpr::create(value, Expr::Int8));
  unflushedMask.store(offset, true);
}

void ObjectState::write8(unsigned offset, ref<Expr> value) {
  // can happen when ExtractExpr special cases
  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(value)) {
    write8(offset, (uint8_t)CE->getZExtValue(8));
  } else {
    knownSymbolics.store(offset, value);
    unflushedMask.store(offset, true);
  }
}

void ObjectState::write8(ref<Expr> offset, ref<Expr> value) {
  assert(!isa<ConstantExpr>(offset) &&
         "constant offset passed to symbolic write8");
  flushForWrite();

  if (object && object->size > 4096) {
    std::string allocInfo;
    object->getAllocInfo(allocInfo);
    klee_warning_once(
        nullptr,
        "Symbolic memory access will send the following array of %d bytes to "
        "the constraint solver -- large symbolic arrays may cause significant "
        "performance issues: %s",
        object->size, allocInfo.c_str());
  }

  updates.extend(ZExtExpr::create(offset, Expr::Int32), value);
}

void ObjectState::write(ref<const ObjectState> os) {
  knownSymbolics = os->knownSymbolics;
  unflushedMask = os->unflushedMask;
  updates = UpdateList(updates.root, os->updates.head);
  lastUpdate = os->lastUpdate;
}

/***/

ref<Expr> ObjectState::read(ref<Expr> offset, Expr::Width width) const {
  // Truncate offset to 32-bits.
  offset = ZExtExpr::create(offset, Expr::Int32);

  // Check for reads at constant offsets.
  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(offset))
    return read(CE->getZExtValue(32), width);

  // Treat bool specially, it is the only non-byte sized write we allow.
  if (width == Expr::Bool)
    return ExtractExpr::create(read8(offset), 0, Expr::Bool);

  if (lastUpdate && lastUpdate->index == offset &&
      lastUpdate->value->getWidth() == width)
    return lastUpdate->value;

  // Otherwise, follow the slow general case.
  unsigned NumBytes = width / 8;
  assert(width == NumBytes * 8 && "Invalid read size!");
  ref<Expr> Res(0);
  for (unsigned i = 0; i != NumBytes; ++i) {
    unsigned idx = Context::get().isLittleEndian() ? i : (NumBytes - i - 1);
    ref<Expr> Byte =
        read8(AddExpr::create(offset, ConstantExpr::create(idx, Expr::Int32)));
    Res = i ? ConcatExpr::create(Byte, Res) : Byte;
  }

  return Res;
}

ref<Expr> ObjectState::read(unsigned offset, Expr::Width width) const {
  // Treat bool specially, it is the only non-byte sized write we allow.
  if (width == Expr::Bool)
    return ExtractExpr::create(read8(offset), 0, Expr::Bool);

  // Otherwise, follow the slow general case.
  unsigned NumBytes = width / 8;
  assert(width == NumBytes * 8 && "Invalid width for read size!");
  ref<Expr> Res(0);
  for (unsigned i = 0; i != NumBytes; ++i) {
    unsigned idx = Context::get().isLittleEndian() ? i : (NumBytes - i - 1);
    ref<Expr> Byte = read8(offset + idx);
    Res = i ? ConcatExpr::create(Byte, Res) : Byte;
  }

  return Res;
}

void ObjectState::write(ref<Expr> offset, ref<Expr> value) {
  // Truncate offset to 32-bits.
  offset = ZExtExpr::create(offset, Expr::Int32);

  // Check for writes at constant offsets.
  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(offset)) {
    write(CE->getZExtValue(32), value);
    return;
  }

  // Treat bool specially, it is the only non-byte sized write we allow.
  Expr::Width w = value->getWidth();
  if (w == Expr::Bool) {
    write8(offset, ZExtExpr::create(value, Expr::Int8));
    return;
  }

  // Otherwise, follow the slow general case.
  unsigned NumBytes = w / 8;
  assert(w == NumBytes * 8 && "Invalid write size!");
  for (unsigned i = 0; i != NumBytes; ++i) {
    unsigned idx = Context::get().isLittleEndian() ? i : (NumBytes - i - 1);
    write8(AddExpr::create(offset, ConstantExpr::create(idx, Expr::Int32)),
           ExtractExpr::create(value, 8 * i, Expr::Int8));
  }
  lastUpdate = new UpdateNode(nullptr, offset, value);
}

void ObjectState::write(unsigned offset, ref<Expr> value) {
  // Check for writes of constant values.
  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(value)) {
    Expr::Width w = CE->getWidth();
    if (w <= 64 && klee::bits64::isPowerOfTwo(w)) {
      uint64_t val = CE->getZExtValue();
      switch (w) {
      default:
        assert(0 && "Invalid write size!");
      case Expr::Bool:
      case Expr::Int8:
        write8(offset, val);
        return;
      case Expr::Int16:
        write16(offset, val);
        return;
      case Expr::Int32:
        write32(offset, val);
        return;
      case Expr::Int64:
        write64(offset, val);
        return;
      }
    }
  }

  // Treat bool specially, it is the only non-byte sized write we allow.
  Expr::Width w = value->getWidth();
  if (w == Expr::Bool) {
    write8(offset, ZExtExpr::create(value, Expr::Int8));
    return;
  }

  // Otherwise, follow the slow general case.
  unsigned NumBytes = w / 8;
  assert(w == NumBytes * 8 && "Invalid write size!");
  for (unsigned i = 0; i != NumBytes; ++i) {
    unsigned idx = Context::get().isLittleEndian() ? i : (NumBytes - i - 1);
    write8(offset + idx, ExtractExpr::create(value, 8 * i, Expr::Int8));
  }
}

void ObjectState::write16(unsigned offset, uint16_t value) {
  unsigned NumBytes = 2;
  for (unsigned i = 0; i != NumBytes; ++i) {
    unsigned idx = Context::get().isLittleEndian() ? i : (NumBytes - i - 1);
    write8(offset + idx, (uint8_t)(value >> (8 * i)));
  }
}

void ObjectState::write32(unsigned offset, uint32_t value) {
  unsigned NumBytes = 4;
  for (unsigned i = 0; i != NumBytes; ++i) {
    unsigned idx = Context::get().isLittleEndian() ? i : (NumBytes - i - 1);
    write8(offset + idx, (uint8_t)(value >> (8 * i)));
  }
}

void ObjectState::write64(unsigned offset, uint64_t value) {
  unsigned NumBytes = 8;
  for (unsigned i = 0; i != NumBytes; ++i) {
    unsigned idx = Context::get().isLittleEndian() ? i : (NumBytes - i - 1);
    write8(offset + idx, (uint8_t)(value >> (8 * i)));
  }
}

void ObjectState::print() const {
  llvm::errs() << "-- ObjectState --\n";
  llvm::errs() << "\tMemoryObject ID: " << object->id << "\n";
  llvm::errs() << "\tRoot Object: " << updates.root << "\n";
  llvm::errs() << "\tSize: " << object->size << "\n";

  llvm::errs() << "\tBytes:\n";
  for (unsigned i = 0; i < object->size; i++) {
    llvm::errs() << "\t\t[" << i << "]"
                 << " known? " << !knownSymbolics.load(i).isNull()
                 << " unflushed? " << unflushedMask.load(i) << " = ";
    ref<Expr> e = read8(i);
    llvm::errs() << e << "\n";
  }

  llvm::errs() << "\tUpdates:\n";
  for (const auto *un = updates.head.get(); un; un = un->next.get()) {
    llvm::errs() << "\t\t[" << un->index << "] = " << un->value << "\n";
  }
}

KType *ObjectState::getDynamicType() const { return dynamicType; }

bool ObjectState::isAccessableFrom(KType *accessingType) const {
  return !UseTypeBasedAliasAnalysis ||
         dynamicType->isAccessableFrom(accessingType);
}
