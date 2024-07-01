//===-- AddressSpace.cpp --------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "AddressSpace.h"

#include "ExecutionState.h"
#include "Memory.h"
#include "TimingSolver.h"

#include "klee/Expr/ArrayExprVisitor.h"
#include "klee/Expr/Expr.h"
#include "klee/Module/KType.h"
#include "klee/Statistics/TimerStatIncrementer.h"

#include "CoreStats.h"

namespace klee {
llvm::cl::OptionCategory
    PointerResolvingCat("Pointer resolving options",
                        "These options impact pointer resolving.");

llvm::cl::opt<bool> SkipNotSymbolicObjects(
    "skip-not-symbolic-objects", llvm::cl::init(false),
    llvm::cl::desc("Set pointers only on symbolic objects, "
                   "use only with timestamps (default=false)"),
    llvm::cl::cat(PointerResolvingCat));

llvm::cl::opt<bool> SkipNotLazyInitialized(
    "skip-not-lazy-initialized", llvm::cl::init(false),
    llvm::cl::desc("Set pointers only on lazy initialized objects, "
                   "use only with timestamps (default=false)"),
    llvm::cl::cat(PointerResolvingCat));

llvm::cl::opt<bool> SkipLocal(
    "skip-local", llvm::cl::init(false),
    llvm::cl::desc(
        "Do not set symbolic pointers on local objects (default=false)"),
    llvm::cl::cat(PointerResolvingCat));

llvm::cl::opt<bool> SkipGlobal(
    "skip-global", llvm::cl::init(false),
    llvm::cl::desc(
        "Do not set symbolic pointers on global objects (default=false)"),
    llvm::cl::cat(PointerResolvingCat));

llvm::cl::opt<bool> UseTimestamps(
    "use-timestamps", llvm::cl::init(true),
    llvm::cl::desc("Set symbolic pointers only to objects created before those "
                   "pointers were created (default=true)"),
    llvm::cl::cat(PointerResolvingCat));
} // namespace klee

using namespace klee;

///

void AddressSpace::bindObject(const MemoryObject *mo, ObjectState *os) {
  assert(os->copyOnWriteOwner == 0 && "object already has owner");
  os->copyOnWriteOwner = cowKey;
  objects = objects.replace(std::make_pair(mo, os));
}

void AddressSpace::bindObject(const MemoryObject *mo, const ObjectState *os) {
  ref<ObjectState> newObjectState(new ObjectState(*os));
  bindObject(mo, newObjectState.get());
}

void AddressSpace::unbindObject(const MemoryObject *mo) {
  objects = objects.remove(mo);
}

ObjectPair AddressSpace::findObject(const MemoryObject *mo) const {
  const auto res = objects.lookup(mo);
  return res ? ObjectPair(res->first, res->second.get())
             : ObjectPair(nullptr, nullptr);
}

RefObjectPair AddressSpace::lazyInitializeObject(const MemoryObject *mo) const {
  return RefObjectPair(mo, new ObjectState(mo, mo->content, mo->type));
}

RefObjectPair
AddressSpace::findOrLazyInitializeObject(const MemoryObject *mo) const {
  ObjectPair op = findObject(mo);
  if (op.first && op.second) {
    return RefObjectPair(op.first, op.second);
  }
  assert(!op.first && !op.second);
  if (mo->isLazyInitialized) {
    return lazyInitializeObject(mo);
  }
  return RefObjectPair(nullptr, nullptr);
}

ObjectState *AddressSpace::getWriteable(const MemoryObject *mo,
                                        const ObjectState *os) {
  // If this address space owns they object, return it
  if (cowKey == os->copyOnWriteOwner)
    return const_cast<ObjectState *>(os);

  // Add a copy of this object state that can be updated
  ref<ObjectState> newObjectState(new ObjectState(*os));
  bindObject(mo, newObjectState.get());
  return newObjectState.get();
}

///

bool AddressSpace::resolveOne(ref<ConstantPointerExpr> address,
                              KType *objectType, ObjectPair &result) const {
  uint64_t baseConst = address->getConstantBase()->getZExtValue();
  uint64_t addressConst = address->getConstantValue()->getZExtValue();
  MemoryObject hack(baseConst);

  if (const auto res = objects.lookup_previous(&hack)) {
    const auto &mo = res->first;
    if (ref<ConstantExpr> arrayConstantAddress =
            dyn_cast<ConstantExpr>(mo->getBaseExpr())) {
      if (ref<ConstantExpr> arrayConstantSize =
              dyn_cast<ConstantExpr>(mo->getSizeExpr())) {
        // Check if the provided address is between start and end of the object
        // [mo->address, mo->address + mo->size) or the object is a 0-sized
        // object.
        uint64_t moSize = arrayConstantSize->getZExtValue();
        uint64_t moAddress = arrayConstantAddress->getZExtValue();
        if ((moSize == 0 && addressConst == moAddress) ||
            (addressConst - moAddress < moSize)) {
          result = findObject(mo);
          return true;
        }
      }
    }
  }

  return false;
}

bool AddressSpace::resolveOneIfUnique(ExecutionState &state,
                                      TimingSolver *solver,
                                      ref<PointerExpr> address,
                                      KType *objectType, ObjectPair &result,
                                      bool &success) const {
  ref<Expr> base = address->getBase();
  ref<Expr> uniqueAddress = base;
  if (!isa<ConstantExpr>(uniqueAddress) &&
      !solver->tryGetUnique(state.constraints.cs(), base, uniqueAddress,
                            state.queryMetaData)) {
    return false;
  }

  success = false;
  if (ref<ConstantExpr> CE = dyn_cast<ConstantExpr>(uniqueAddress)) {
    MemoryObject hack(CE->getZExtValue());
    if (const auto *res = objects.lookup_previous(&hack)) {
      const MemoryObject *mo = res->first;
      ref<Expr> inBounds = mo->getBoundsCheckPointer(address);

      if (!solver->mayBeTrue(state.constraints.cs(), inBounds, success,
                             state.queryMetaData)) {
        return false;
      }
      if (success) {
        result = findObject(mo);
      }
    }
  }

  return true;
}

class ResolvePredicate {
  bool useTimestamps;
  bool skipNotSymbolicObjects;
  bool skipNotLazyInitialized;
  bool skipLocal;
  bool skipGlobal;
  unsigned timestamp;
  ExecutionState *state;
  KType *objectType;
  bool complete;

public:
  explicit ResolvePredicate(ExecutionState &state, ref<PointerExpr> address,
                            KType *objectType, bool complete)
      : useTimestamps(UseTimestamps),
        skipNotSymbolicObjects(SkipNotSymbolicObjects),
        skipNotLazyInitialized(SkipNotLazyInitialized), skipLocal(SkipLocal),
        skipGlobal(SkipGlobal), timestamp(UINT_MAX), state(&state),
        objectType(objectType), complete(complete) {
    ref<Expr> base = address->getBase();
    if (!isa<ConstantExpr>(base)) {
      std::pair<ref<const MemoryObject>, ref<Expr>> moBasePair;
      if (state.getBase(base, moBasePair)) {
        timestamp = moBasePair.first->timestamp;
      }
    }
    // This is hack to deal with the poineters, which are represented as
    // select expressions from constant pointers with symbolic condition.
    // in such case we allow to resolve in all objects in the address space,
    // but should forbid lazy initilization.
    if (isa<SelectExpr>(base)) {
      std::vector<ref<Expr>> alternatives;
      ArrayExprHelper::collectAlternatives(cast<SelectExpr>(*base),
                                           alternatives);
      if (std::find_if_not(alternatives.begin(), alternatives.end(),
                           [](ref<Expr> expr) {
                             return isa<ConstantExpr>(expr);
                           }) == alternatives.end()) {
        skipNotSymbolicObjects = false;
        skipNotLazyInitialized = false;
        skipLocal = false;
        skipGlobal = false;
      }
    }
  }

  bool operator()(const MemoryObject *mo, const ObjectState *os) const {
    bool result = !useTimestamps || mo->timestamp <= timestamp;
    result = result && (!skipNotSymbolicObjects || state->inSymbolics(mo));
    result = result && (!skipNotLazyInitialized || mo->isLazyInitialized);
    result = result && (!skipLocal || !mo->isLocal);
    result = result && (!skipGlobal || !mo->isGlobal);
    result = result && os->isAccessableFrom(objectType);
    result = result && (!complete || os->wasWritten);
    return result;
  }
};

bool AddressSpace::resolveOne(ExecutionState &state, TimingSolver *solver,
                              ref<PointerExpr> address, KType *objectType,
                              ObjectPair &result, bool &success,
                              const std::atomic_bool &haltExecution) const {
  ResolvePredicate predicate(state, address, objectType, complete);
  ref<Expr> addressExpr = address->getValue();
  if (ref<ConstantPointerExpr> CP = dyn_cast<ConstantPointerExpr>(address)) {
    if (resolveOne(CP, objectType, result)) {
      success = true;
      return true;
    }
  }

  TimerStatIncrementer timer(stats::resolveTime);

  // try cheap search, will succeed for any inbounds pointer

  ref<ConstantPointerExpr> addressCex;
  if (!solver->getValue(state.constraints.cs(), address, addressCex,
                        state.queryMetaData))
    return false;

  if (resolveOne(addressCex, objectType, result)) {
    success = true;
    return true;
  }

  // didn't work, now we have to search

  for (MemoryMap::iterator oi = objects.begin(), oe = objects.end(); oi != oe;
       ++oi) {
    const auto &mo = oi->first;
    if (!predicate(mo, oi->second.get())) {
      continue;
    }

    if (haltExecution) {
      break;
    }

    bool mayBeTrue;
    if (!solver->mayBeTrue(state.constraints.cs(),
                           mo->getBoundsCheckPointer(address), mayBeTrue,
                           state.queryMetaData))
      return false;
    if (mayBeTrue) {
      result = {oi->first, oi->second.get()};
      success = true;
      return true;
    }
  }

  success = false;
  return true;
}

int AddressSpace::checkPointerInObject(ExecutionState &state,
                                       TimingSolver *solver, ref<PointerExpr> p,
                                       const ObjectPair &op, ResolutionList &rl,
                                       unsigned maxResolutions) const {
  // XXX in the common case we can save one query if we ask
  // mustBeTrue before mayBeTrue for the first result. easy
  // to add I just want to have a nice symbolic test case first.
  const MemoryObject *mo = op.first;
  ref<Expr> inBounds = mo->getBoundsCheckPointer(p);

  bool mayBeTrue;
  if (!solver->mayBeTrue(state.constraints.cs(), inBounds, mayBeTrue,
                         state.queryMetaData)) {
    return 1;
  }

  if (mayBeTrue) {
    rl.push_back(op);

    // fast path check
    auto size = rl.size();
    if (size == 1) {
      bool mustBeTrue;
      if (!solver->mustBeTrue(state.constraints.cs(), inBounds, mustBeTrue,
                              state.queryMetaData))
        return 1;
      if (mustBeTrue)
        return 0;
    } else if (size == maxResolutions)
      return 1;
  }

  return 2;
}

bool AddressSpace::resolve(ExecutionState &state, TimingSolver *solver,
                           ref<PointerExpr> p, KType *objectType,
                           ResolutionList &rl, ResolutionList &rlSkipped,
                           unsigned maxResolutions, time::Span timeout) const {
  ResolvePredicate predicate(state, p, objectType, complete);
  if (ref<ConstantPointerExpr> CP = dyn_cast<ConstantPointerExpr>(p)) {
    ObjectPair res;
    if (resolveOne(CP, objectType, res)) {
      rl.push_back(res);
      return false;
    }
  }
  TimerStatIncrementer timer(stats::resolveTime);

  ObjectPair fastPathObjectID;
  bool fastPathSuccess;
  if (!resolveOneIfUnique(state, solver, p, objectType, fastPathObjectID,
                          fastPathSuccess)) {
    return true;
  }
  if (fastPathSuccess) {
    rl.push_back(fastPathObjectID);
    return false;
  }

  // XXX in general this isn't exactly what we want... for
  // a multiple resolution case (or for example, a \in {b,c,0})
  // we want to find the first object, find a cex assuming
  // not the first, find a cex assuming not the second...
  // etc.

  // XXX how do we smartly amortize the cost of checking to
  // see if we need to keep searching up/down, in bad cases?
  // maybe we don't care?

  // XXX we really just need a smart place to start (although
  // if its a known solution then the code below is guaranteed
  // to hit the fast path with exactly 2 queries). we could also
  // just get this by inspection of the expr.

  for (MemoryMap::iterator oi = objects.begin(), oe = objects.end(); oi != oe;
       ++oi) {
    const MemoryObject *mo = oi->first;
    if (!predicate(mo, oi->second.get())) {
      continue;
    }

    if (timeout && timeout < timer.delta())
      return true;

    auto op = std::make_pair<>(mo, oi->second.get());

    int incomplete =
        checkPointerInObject(state, solver, p, op, rl, maxResolutions);
    if (incomplete != 2)
      return incomplete ? true : false;
  }
  return false;
}

// These two are pretty big hack so we can sort of pass memory back
// and forth to externals. They work by abusing the concrete cache
// store inside of the object states, which allows them to
// transparently avoid screwing up symbolics (if the byte is symbolic
// then its concrete cache byte isn't being used) but is just a hack.

void AddressSpace::copyOutConcretes(const Assignment &assignment) {
  for (const auto &object : objects) {
    auto &mo = object.first;
    auto &os = object.second;
    if (ref<ConstantExpr> sizeExpr =
            dyn_cast<ConstantExpr>(mo->getSizeExpr())) {
      if (!mo->isUserSpecified && !os->readOnly &&
          sizeExpr->getZExtValue() != 0) {
        copyOutConcrete(mo, os.get(), assignment);
      }
    }
  }
}

ref<ConstantExpr> toConstantExpr(ref<Expr> expr) {
  if (ref<ConstantPointerExpr> pointer = dyn_cast<ConstantPointerExpr>(expr)) {
    return pointer->getConstantValue();
  } else {
    return cast<ConstantExpr>(expr);
  }
}

void AddressSpace::copyOutConcrete(const MemoryObject *mo,
                                   const ObjectState *os,
                                   const Assignment &assignment) const {
  if (ref<ConstantExpr> addressExpr =
          dyn_cast<ConstantExpr>(mo->getBaseExpr())) {
    auto address =
        reinterpret_cast<std::uint8_t *>(addressExpr->getZExtValue());
    AssignmentEvaluator evaluator(assignment, false);
    if (ref<ConstantExpr> sizeExpr =
            dyn_cast<ConstantExpr>(mo->getSizeExpr())) {
      size_t moSize = sizeExpr->getZExtValue();
      std::vector<uint8_t> concreteStore(moSize);
      for (size_t i = 0; i < moSize; i++) {
        auto byte = evaluator.visit(os->readValue8(i));
        concreteStore[i] = cast<ConstantExpr>(byte)->getZExtValue(Expr::Int8);
      }
      std::memcpy(address, concreteStore.data(), moSize);
    }
  }
}

bool AddressSpace::copyInConcretes(const Assignment &assignment) {
  for (auto &obj : objects) {
    const MemoryObject *mo = obj.first;

    if (!mo->isUserSpecified) {
      const auto &os = obj.second;

      if (ref<ConstantExpr> arrayConstantAddress =
              dyn_cast<ConstantExpr>(mo->getBaseExpr())) {
        if (!copyInConcrete(mo, os.get(), arrayConstantAddress->getZExtValue(),
                            assignment))
          return false;
      }
    }
  }

  return true;
}

bool AddressSpace::copyInConcrete(const MemoryObject *mo, const ObjectState *os,
                                  uint64_t src_address,
                                  const Assignment &assignment) {
  AssignmentEvaluator evaluator(assignment, false);
  auto address = reinterpret_cast<std::uint8_t *>(src_address);
  size_t moSize =
      cast<ConstantExpr>(evaluator.visit(mo->getSizeExpr()))->getZExtValue();
  std::vector<uint8_t> concreteStore(moSize);
  for (size_t i = 0; i < moSize; i++) {
    auto byte = evaluator.visit(os->readValue8(i));
    concreteStore[i] = cast<ConstantExpr>(byte)->getZExtValue(8);
  }
  if (memcmp(address, concreteStore.data(), moSize) != 0) {
    if (os->readOnly) {
      return false;
    } else {
      ObjectState *wos = getWriteable(mo, os);
      for (size_t i = 0; i < moSize; i++) {
        wos->write(i, ConstantExpr::create(address[i], Expr::Int8));
      }
    }
  }
  return true;
}

/***/

bool MemoryObjectLT::operator()(const MemoryObject *a,
                                const MemoryObject *b) const {
  if (a->address.has_value() && b->address.has_value()) {
    return a->address < b->address;
  } else if (a->address.has_value()) {
    return true;
  } else if (b->address.has_value()) {
    return false;
  }
  return a->getBaseExpr() < b->getBaseExpr();
}
