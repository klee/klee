//===-- Memory.cpp --------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "Memory.h"

#include "ConstructStorage.h"
#include "ExecutionState.h"
#include "MemoryManager.h"
#include "klee/ADT/Bits.h"
#include "klee/ADT/Ref.h"
#include "klee/ADT/SparseStorage.h"
#include "klee/Core/Context.h"

#include "CodeLocation.h"
#include "klee/Expr/Assignment.h"
#include "klee/Expr/Expr.h"
#include "klee/Module/KType.h"
#include "klee/Solver/Solver.h"
#include "klee/Support/ErrorHandling.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"

#include <algorithm>
#include <cassert>
#include <cstddef>

namespace klee {
llvm::cl::opt<MemoryType> MemoryBackend(
    "memory-backend",
    llvm::cl::desc("Type of data structures to use for memory modelling "
                   "(default=fixed)"),
    llvm::cl::values(clEnumValN(MemoryType::Fixed, "fixed",
                                "Use fixed size data structure (default) "),
                     clEnumValN(MemoryType::Dynamic, "dynamic",
                                "Use dynamic size data structure"),
                     clEnumValN(MemoryType::Persistent, "persistent",
                                "Use persistent data structures"),
                     clEnumValN(MemoryType::Mixed, "mixed",
                                "Use according to the conditions")),
    llvm::cl::init(MemoryType::Fixed));

llvm::cl::opt<unsigned long> MaxFixedSizeStructureSize(
    "max-fixed-size-structures-size",
    llvm::cl::desc("Maximum available size to use dense structures for memory "
                   "(default 10Mb)"),
    llvm::cl::init(10ll << 10));
} // namespace klee

using namespace llvm;
using namespace klee;

/***/

IDType MemoryObject::counter = 1;
int MemoryObject::time = 0;

MemoryObject::~MemoryObject() {
  if (parent)
    parent->markFreed(this);
}

std::string MemoryObject::getAllocInfo() const {
  std::string result;
  llvm::raw_string_ostream info(result);

  info << "MO" << id << "[";
  sizeExpr->print(info);
  info << "]";

  if (allocSite && allocSite->source) {
    const llvm::Value *allocSiteSource = allocSite->source->unwrap();
    info << " allocated at ";
    if (const Instruction *i = dyn_cast<Instruction>(allocSiteSource)) {
      info << i->getParent()->getParent()->getName() << "():";
      info << *i;
    } else if (const GlobalValue *gv = dyn_cast<GlobalValue>(allocSiteSource)) {
      info << "global:" << gv->getName();
    } else {
      info << "value:" << *allocSiteSource;
    }
  } else {
    info << " (no allocation info)";
  }

  info.flush();
  return result;
}

/***/

ObjectState::ObjectState(const MemoryObject *mo, const Array *array, KType *dt)
    : copyOnWriteOwner(0), object(mo), valueOS(ObjectStage(array, nullptr)),
      baseOS(ObjectStage(array->size, Expr::createPointer(0), false,
                         Context::get().getPointerWidth())),
      lastUpdate(nullptr), size(array->size), dynamicType(dt), readOnly(false) {
  baseOS.initializeToZero();
}

ObjectState::ObjectState(const MemoryObject *mo, KType *dt)
    : copyOnWriteOwner(0), object(mo),
      valueOS(ObjectStage(mo->getSizeExpr(), nullptr)),
      baseOS(ObjectStage(mo->getSizeExpr(), Expr::createPointer(0), false,
                         Context::get().getPointerWidth())),
      lastUpdate(nullptr), size(mo->getSizeExpr()), dynamicType(dt),
      readOnly(false) {
  baseOS.initializeToZero();
}

ObjectState::ObjectState(const ObjectState &os)
    : copyOnWriteOwner(0), object(os.object), valueOS(os.valueOS),
      baseOS(os.baseOS), lastUpdate(os.lastUpdate), size(os.size),
      dynamicType(os.dynamicType), readOnly(os.readOnly),
      wasWritten(os.wasWritten) {}

/***/

void ObjectState::initializeToZero() {
  valueOS.initializeToZero();
  baseOS.initializeToZero();
}

ref<Expr> ObjectState::read8(unsigned offset) const {
  ref<Expr> val = valueOS.readWidth(offset);
  ref<Expr> base = baseOS.readWidth(offset);
  if (base->isZero()) {
    return val;
  } else {
    return PointerExpr::create(base, val);
  }
}

ref<Expr> ObjectState::readValue8(unsigned offset) const {
  return valueOS.readWidth(offset);
}

ref<Expr> ObjectState::readBase8(unsigned offset) const {
  return baseOS.readWidth(offset);
}

ref<Expr> ObjectState::read8(ref<Expr> offset) const {
  assert(!isa<ConstantExpr>(offset) &&
         "constant offset passed to symbolic read8");

  if (object) {
    if (ref<ConstantExpr> sizeExpr =
            dyn_cast<ConstantExpr>(object->getSizeExpr())) {
      auto moSize = sizeExpr->getZExtValue();
      if (object && moSize > 4096) {
        std::string allocInfo = object->getAllocInfo();
        klee_warning_once(nullptr,
                          "Symbolic memory access will send the following "
                          "array of %" PRId64 " bytes to "
                          "the constraint solver -- large symbolic arrays may "
                          "cause significant "
                          "performance issues: %s",
                          moSize, allocInfo.c_str());
      }
    }
  }
  ref<Expr> val = valueOS.readWidth(offset);
  ref<Expr> base = baseOS.readWidth(offset);
  if (base->isZero()) {
    return val;
  } else {
    return PointerExpr::create(base, val);
  }
}

ref<Expr> ObjectState::readValue8(ref<Expr> offset) const {
  assert(!isa<ConstantExpr>(offset) &&
         "constant offset passed to symbolic read8");

  if (object) {
    if (ref<ConstantExpr> sizeExpr =
            dyn_cast<ConstantExpr>(object->getSizeExpr())) {
      auto moSize = sizeExpr->getZExtValue();
      if (object && moSize > 4096) {
        std::string allocInfo = object->getAllocInfo();
        klee_warning_once(nullptr,
                          "Symbolic memory access will send the following "
                          "array of %" PRId64 " bytes to "
                          "the constraint solver -- large symbolic arrays may "
                          "cause significant "
                          "performance issues: %s",
                          moSize, allocInfo.c_str());
      }
    }
  }
  return valueOS.readWidth(offset);
}

ref<Expr> ObjectState::readBase8(ref<Expr> offset) const {
  assert(!isa<ConstantExpr>(offset) &&
         "constant offset passed to symbolic read8");

  if (object) {
    if (ref<ConstantExpr> sizeExpr =
            dyn_cast<ConstantExpr>(object->getSizeExpr())) {
      auto moSize = sizeExpr->getZExtValue();
      if (object && moSize > 4096) {
        std::string allocInfo = object->getAllocInfo();
        klee_warning_once(nullptr,
                          "Symbolic memory access will send the following "
                          "array of %" PRId64 " bytes to "
                          "the constraint solver -- large symbolic arrays may "
                          "cause significant "
                          "performance issues: %s",
                          moSize, allocInfo.c_str());
      }
    }
  }
  return baseOS.readWidth(offset);
}

void ObjectState::write8(unsigned offset, uint8_t value) {
  valueOS.writeWidth(offset, value);
  baseOS.writeWidth(offset,
                    ConstantExpr::create(0, Context::get().getPointerWidth()));
}

void ObjectState::write8(unsigned offset, ref<Expr> value) {
  wasWritten = true;
  if (auto pointer = dyn_cast<PointerExpr>(value)) {
    valueOS.writeWidth(offset, pointer->getValue());
    baseOS.writeWidth(offset, pointer->getBase());
  } else {
    valueOS.writeWidth(offset, value);
    baseOS.writeWidth(
        offset, ConstantExpr::create(0, Context::get().getPointerWidth()));
  }
}

void ObjectState::write8(ref<Expr> offset, ref<Expr> value) {
  wasWritten = true;

  assert(!isa<ConstantExpr>(offset) &&
         "constant offset passed to symbolic write8");

  if (ref<ConstantExpr> sizeExpr =
          dyn_cast<ConstantExpr>(object->getSizeExpr())) {
    auto moSize = sizeExpr->getZExtValue();
    if (object && moSize > 4096) {
      std::string allocInfo = object->getAllocInfo();
      klee_warning_once(nullptr,
                        "Symbolic memory access will send the following array "
                        "array of %" PRId64 " bytes to "
                        "the constraint solver -- large symbolic arrays may "
                        "cause significant "
                        "performance issues: %s",
                        moSize, allocInfo.c_str());
    }
  }

  if (auto pointer = dyn_cast<PointerExpr>(value)) {
    valueOS.writeWidth(offset, pointer->getValue());
    baseOS.writeWidth(offset, pointer->getBase());
  } else {
    valueOS.writeWidth(offset, value);
    baseOS.writeWidth(
        offset, ConstantExpr::create(0, Context::get().getPointerWidth()));
  }
}

void ObjectState::write(ref<const ObjectState> os) {
  wasWritten = true;
  valueOS.write(os->valueOS);
  baseOS.write(os->baseOS);
  lastUpdate = os->lastUpdate;
}

/***/

ref<Expr> ObjectState::read(ref<Expr> offset, Expr::Width width) const {
  if (lastUpdate && lastUpdate->index == offset &&
      lastUpdate->value->getWidth() == width)
    return lastUpdate->value;

  ref<Expr> val = readValue(offset, width);
  ref<Expr> base = readBase(offset, width);
  if (base->isZero()) {
    return val;
  } else {
    return PointerExpr::create(base, val);
  }
}

ref<Expr> ObjectState::read(unsigned offset, Expr::Width width) const {
  ref<Expr> val = readValue(offset, width);
  ref<Expr> base = readBase(offset, width);
  if (base->isZero()) {
    return val;
  } else {
    return PointerExpr::create(base, val);
  }
}

ref<Expr> ObjectState::readValue(ref<Expr> offset, Expr::Width width) const {
  // Truncate offset to 32-bits.
  offset = ZExtExpr::create(offset, Expr::Int32);

  // Check for reads at constant offsets.
  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(offset))
    return readValue(CE->getZExtValue(32), width);

  // Treat bool specially, it is the only non-byte sized write we allow.
  if (width == Expr::Bool)
    return ExtractExpr::create(readValue8(offset), 0, Expr::Bool);

  // Otherwise, follow the slow general case.
  unsigned NumBytes = width / 8;
  assert(width == NumBytes * 8 && "Invalid read size!");
  ref<Expr> Res(0);
  for (unsigned i = 0; i != NumBytes; ++i) {
    unsigned idx = Context::get().isLittleEndian() ? i : (NumBytes - i - 1);
    ref<Expr> Byte = readValue8(
        AddExpr::create(offset, ConstantExpr::create(idx, Expr::Int32)));
    Res = i ? ConcatExpr::create(Byte, Res) : Byte;
  }

  return Res;
}

ref<Expr> ObjectState::readValue(unsigned offset, Expr::Width width) const {
  // Treat bool specially, it is the only non-byte sized write we allow.
  if (width == Expr::Bool)
    return ExtractExpr::create(readValue8(offset), 0, Expr::Bool);

  // Otherwise, follow the slow general case.
  unsigned NumBytes = width / 8;
  assert(width == NumBytes * 8 && "Invalid width for read size!");
  ref<Expr> Res(0);
  for (unsigned i = 0; i != NumBytes; ++i) {
    unsigned idx = Context::get().isLittleEndian() ? i : (NumBytes - i - 1);
    ref<Expr> Byte = readValue8(offset + idx);
    Res = i ? ConcatExpr::create(Byte, Res) : Byte;
  }

  return Res;
}

ref<Expr> ObjectState::readBase(ref<Expr> offset, Expr::Width width) const {
  // Truncate offset to 32-bits.
  offset = ZExtExpr::create(offset, Expr::Int32);

  // Check for reads at constant offsets.
  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(offset))
    return readBase(CE->getZExtValue(32), width);

  // Treat bool specially, it is the only non-byte sized write we allow.
  if (width == Expr::Bool)
    return ExtractExpr::create(readBase8(offset), 0, Expr::Bool);

  // Treat bool specially, it is the only non-byte sized write we allow.
  if (width == Expr::Bool)
    return ExtractExpr::create(readBase8(offset), 0, Expr::Bool);

  // Otherwise, follow the slow general case.
  unsigned NumBytes = width / 8;
  assert(width == NumBytes * 8 && "Invalid read size!");
  ref<Expr> Res(0);
  ref<Expr> null = Expr::createPointer(0);
  for (unsigned i = 0; i != NumBytes; ++i) {
    unsigned idx = Context::get().isLittleEndian() ? i : (NumBytes - i - 1);
    ref<Expr> Byte = readBase8(
        AddExpr::create(offset, ConstantExpr::create(idx, Expr::Int32)));
    if (i) {
      ref<Expr> eqBytes = EqExpr::create(Byte, Res);
      Res = SelectExpr::create(eqBytes, Byte, null);
    } else {
      Res = Byte;
    }
  }

  return Res;
}

ref<Expr> ObjectState::readBase(unsigned offset, Expr::Width width) const {
  // Treat bool specially, it is the only non-byte sized write we allow.
  if (width == Expr::Bool)
    return ExtractExpr::create(readBase8(offset), 0, Expr::Bool);

  // Otherwise, follow the slow general case.
  unsigned NumBytes = width / 8;
  assert(width == NumBytes * 8 && "Invalid width for read size!");
  ref<Expr> Res(0);
  ref<Expr> null = Expr::createPointer(0);
  for (unsigned i = 0; i != NumBytes; ++i) {
    unsigned idx = Context::get().isLittleEndian() ? i : (NumBytes - i - 1);
    ref<Expr> Byte = readBase8(offset + idx);
    if (i) {
      ref<Expr> eqBytes = EqExpr::create(Byte, Res);
      Res = SelectExpr::create(eqBytes, Byte, null);
    } else {
      Res = Byte;
    }
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
  llvm::errs() << "\tBase ObjectStage:\n";
  valueOS.print();
  llvm::errs() << "\tOffset ObjectStage:\n";
  baseOS.print();
}

KType *ObjectState::getDynamicType() const { return dynamicType; }

bool ObjectState::isAccessableFrom(KType *accessingType) const {
  return !UseTypeBasedAliasAnalysis ||
         dynamicType->isAccessableFrom(accessingType);
}

/***/

ObjectStage::ObjectStage(const Array *array, ref<Expr> defaultValue, bool safe,
                         Expr::Width width)
    : updates(array, nullptr), size(array->size), safeRead(safe), width(width) {
  knownSymbolics.reset(constructStorage<ref<Expr>, OptionalRefEq<Expr>>(
      array->getSize(), defaultValue, MaxFixedSizeStructureSize));
  unflushedMask.reset(
      constructStorage(array->getSize(), false, MaxFixedSizeStructureSize));
}

ObjectStage::ObjectStage(ref<Expr> size, ref<Expr> defaultValue, bool safe,
                         Expr::Width width)
    : updates(nullptr, nullptr), size(size), safeRead(safe), width(width) {
  knownSymbolics.reset(constructStorage<ref<Expr>, OptionalRefEq<Expr>>(
      size, defaultValue, MaxFixedSizeStructureSize));
  unflushedMask.reset(constructStorage(size, false, MaxFixedSizeStructureSize));
}

ObjectStage::ObjectStage(const ObjectStage &os)
    : knownSymbolics(os.knownSymbolics->clone()),
      unflushedMask(os.unflushedMask->clone()), updates(os.updates),
      size(os.size), safeRead(os.safeRead), width(os.width) {}

/***/

const UpdateList &ObjectStage::getUpdates() const {
  if (auto sizeExpr = dyn_cast<ConstantExpr>(size)) {
    auto size = sizeExpr->getZExtValue();
    if (knownSymbolics->storage().size() == size) {
      std::unique_ptr<SparseStorage<ref<ConstantExpr>>> values(constructStorage(
          sizeExpr, ConstantExpr::create(0, width), MaxFixedSizeStructureSize));
      UpdateList symbolicUpdates = UpdateList(nullptr, nullptr);
      for (unsigned i = 0; i < size; i++) {
        auto value = knownSymbolics->load(i);
        assert(value);
        if (isa<ConstantExpr>(value)) {
          values->store(i, cast<ConstantExpr>(value));
        } else {
          symbolicUpdates.extend(ConstantExpr::create(i, Expr::Int32), value);
        }
      }
      auto array =
          Array::create(sizeExpr, SourceBuilder::constant(values->clone()),
                        Expr::Int32, width);
      updates = UpdateList(array, symbolicUpdates.head);
      knownSymbolics->reset(nullptr);
      unflushedMask->reset();
    }
  }

  if (!updates.root) {
    auto array = Array::create(
        size,
        SourceBuilder::constant(constructStorage(
            size, ConstantExpr::create(0, width), MaxFixedSizeStructureSize)),
        Expr::Int32, width);
    updates = UpdateList(array, updates.head);
  }

  assert(updates.root);

  return updates;
}

void ObjectStage::initializeToZero() {
  auto array = Array::create(
      size,
      SourceBuilder::constant(constructStorage(
          size, ConstantExpr::create(0, width), MaxFixedSizeStructureSize)),
      Expr::Int32, width);
  updates = UpdateList(array, nullptr);
  knownSymbolics->reset();
  unflushedMask->reset();
}

void ObjectStage::flushForRead() const {
  for (const auto &unflushed : unflushedMask->storage()) {
    auto offset = unflushed.first;
    auto value = knownSymbolics->load(offset);
    assert(value);
    updates.extend(ConstantExpr::create(offset, Expr::Int32), value);
  }
  unflushedMask->reset(false);
}

void ObjectStage::flushForWrite() {
  flushForRead();
  // The write is symbolic offset and might overwrite any byte
  knownSymbolics->reset(nullptr);
}

/***/

ref<Expr> ObjectStage::readWidth(unsigned offset) const {
  if (auto byte = knownSymbolics->load(offset)) {
    return byte;
  } else {
    assert(!unflushedMask->load(offset) &&
           "unflushed byte without cache value");
    return ReadExpr::create(
        getUpdates(), ConstantExpr::create(offset, Expr::Int32), safeRead);
  }
}

ref<Expr> ObjectStage::readWidth(ref<Expr> offset) const {
  assert(!isa<ConstantExpr>(offset) &&
         "constant offset passed to symbolic read8");
  flushForRead();

  return ReadExpr::create(getUpdates(), ZExtExpr::create(offset, Expr::Int32),
                          safeRead);
}

void ObjectStage::writeWidth(unsigned offset, uint64_t value) {
  auto byte = knownSymbolics->load(offset);
  if (byte) {
    auto ce = dyn_cast<ConstantExpr>(byte);
    if (ce && ce->getZExtValue(width) == value) {
      return;
    }
  }
  knownSymbolics->store(offset, ConstantExpr::create(value, width));
  unflushedMask->store(offset, true);
}

void ObjectStage::writeWidth(unsigned offset, ref<Expr> value) {
  // can happen when ExtractExpr special cases
  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(value)) {
    writeWidth(offset, CE->getZExtValue(width));
  } else {
    auto byte = knownSymbolics->load(offset);
    if (byte && byte == value) {
      return;
    }
    knownSymbolics->store(offset, value);
    unflushedMask->store(offset, true);
  }
}

void ObjectStage::writeWidth(ref<Expr> offset, ref<Expr> value) {
  assert(!isa<ConstantExpr>(offset) &&
         "constant offset passed to symbolic write8");

  if (knownSymbolics->defaultV() && knownSymbolics->defaultV() == value &&
      knownSymbolics->storage().size() == 0 && updates.getSize() == 0) {
    return;
  }

  flushForWrite();

  updates.extend(ZExtExpr::create(offset, Expr::Int32), value);
}

void ObjectStage::write(const ObjectStage &os) {
  auto constSize = dyn_cast<ConstantExpr>(size);
  auto osConstSize = dyn_cast<ConstantExpr>(os.size);
  if (constSize || osConstSize) {
    size_t bound = 0;
    if (osConstSize && constSize) {
      bound = std::min(constSize->getZExtValue(), osConstSize->getZExtValue());
    } else if (constSize) {
      bound = constSize->getZExtValue();
    } else {
      bound = osConstSize->getZExtValue();
    }
    for (size_t i = 0; i < bound; ++i) {
      knownSymbolics->store(i, os.knownSymbolics->load(i));
      unflushedMask->store(i, os.unflushedMask->load(i));
    }
  } else {
    knownSymbolics.reset(os.knownSymbolics->clone());
    unflushedMask.reset(os.unflushedMask->clone());
  }
  updates = UpdateList(updates.root, os.updates.head);
}

/***/

void ObjectStage::print() const {
  llvm::errs() << "-- ObjectStage --\n";
  llvm::errs() << "\tRoot Object: " << updates.root << "\n";
  llvm::errs() << "\tSize: " << size << "\n";

  llvm::errs() << "\tBytes:\n";

  for (auto [index, value] : knownSymbolics->storage()) {
    llvm::errs() << "\t\t[" << index << "]";
    llvm::errs() << value << "\n";
  }

  llvm::errs() << "\tUpdates:\n";
  for (const auto *un = updates.head.get(); un; un = un->next.get()) {
    llvm::errs() << "\t\t[" << un->index << "] = " << un->value << "\n";
  }
}
