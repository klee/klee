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
  idToObjects = idToObjects.replace(std::make_pair(mo->id, mo));
}

void AddressSpace::unbindObject(const MemoryObject *mo) {
  idToObjects = idToObjects.remove(mo->id);
  objects = objects.remove(mo);
}

ObjectPair AddressSpace::findObject(const MemoryObject *mo) const {
  const auto res = objects.lookup(mo);
  return res ? ObjectPair(res->first, res->second.get())
             : ObjectPair(nullptr, nullptr);
}

ObjectPair AddressSpace::findObject(IDType id) const {
  const auto res = idToObjects.lookup(id);
  return res ? findObject(res->second) : ObjectPair(nullptr, nullptr);
}

ObjectState *AddressSpace::getWriteable(const MemoryObject *mo,
                                        const ObjectState *os) {
  // If this address space owns they object, return it
  if (cowKey == os->copyOnWriteOwner)
    return const_cast<ObjectState *>(os);

  // Add a copy of this object state that can be updated
  ref<ObjectState> newObjectState(new ObjectState(*os));
  newObjectState->copyOnWriteOwner = cowKey;
  objects = objects.replace(std::make_pair(mo, newObjectState));
  idToObjects = idToObjects.replace(std::make_pair(mo->id, mo));
  return newObjectState.get();
}

///

bool AddressSpace::resolveOne(const ref<ConstantExpr> &addr, KType *objectType,
                              IDType &result) const {
  uint64_t address = addr->getZExtValue();
  MemoryObject hack(address);

  if (const auto res = objects.lookup_previous(&hack)) {
    const auto &mo = res->first;
    if (ref<ConstantExpr> arrayConstantSize =
            dyn_cast<ConstantExpr>(mo->getSizeExpr())) {
      // Check if the provided address is between start and end of the object
      // [mo->address, mo->address + mo->size) or the object is a 0-sized
      // object.
      uint64_t size = arrayConstantSize->getZExtValue();
      if ((size == 0 && address == mo->address) ||
          (address - mo->address < size)) {
        result = mo->id;
        return true;
      }
    }
  }

  return false;
}

bool AddressSpace::resolveOneIfUnique(ExecutionState &state,
                                      TimingSolver *solver, ref<Expr> address,
                                      KType *objectType, IDType &result,
                                      bool &success) const {
  ref<Expr> base =
      state.isGEPExpr(address) ? state.gepExprBases.at(address).first : address;
  ref<Expr> uniqueAddress;
  if (!solver->tryGetUnique(state.constraints, base, uniqueAddress,
                            state.queryMetaData)) {
    return false;
  }

  success = false;
  if (ref<ConstantExpr> CE = dyn_cast<ConstantExpr>(uniqueAddress)) {
    MemoryObject hack(CE->getZExtValue());
    if (const auto *res = objects.lookup_previous(&hack)) {
      const MemoryObject *mo = res->first;
      ref<Expr> inBounds = mo->getBoundsCheckPointer(address);

      if (!solver->mayBeTrue(state.constraints, inBounds, success,
                             state.queryMetaData)) {
        return false;
      }
      if (success) {
        result = mo->id;
      }
    }
  }

  return true;
}

bool AddressSpace::resolveOne(ExecutionState &state, TimingSolver *solver,
                              ref<Expr> address, KType *objectType,
                              IDType &result, MOPredicate predicate,
                              bool &success) const {
  if (ref<ConstantExpr> CE = dyn_cast<ConstantExpr>(address)) {
    if (resolveOne(CE, objectType, result)) {
      success = true;
      return true;
    }
  }

  TimerStatIncrementer timer(stats::resolveTime);

  if (!resolveOneIfUnique(state, solver, address, objectType, result,
                          success)) {
    return false;
  }
  if (success) {
    return true;
  }

  // try cheap search, will succeed for any inbounds pointer

  ref<ConstantExpr> cex;
  if (!solver->getValue(state.constraints, address, cex, state.queryMetaData))
    return false;
  uint64_t example = cex->getZExtValue();
  MemoryObject hack(example);
  const auto res = objects.lookup_previous(&hack);

  if (res && predicate(res->first)) {
    const MemoryObject *mo = res->first;
    if (ref<ConstantExpr> arrayConstantSize =
            dyn_cast<ConstantExpr>(mo->getSizeExpr())) {
      if (example - mo->address < arrayConstantSize->getZExtValue() &&
          res->second->isAccessableFrom(objectType)) {
        result = mo->id;
        success = true;
        return true;
      }
    }
  }

  // didn't work, now we have to search

  for (MemoryMap::iterator oi = objects.begin(), oe = objects.end(); oi != oe;
       ++oi) {
    const auto &mo = oi->first;
    if (!predicate(mo))
      continue;

    if (!oi->second->isAccessableFrom(objectType)) {
      continue;
    }

    bool mayBeTrue;
    if (!solver->mayBeTrue(state.constraints,
                           mo->getBoundsCheckPointer(address), mayBeTrue,
                           state.queryMetaData))
      return false;
    if (mayBeTrue) {
      result = oi->first->id;
      success = true;
      return true;
    }
  }

  success = false;
  return true;
}

bool AddressSpace::resolveOne(ExecutionState &state, TimingSolver *solver,
                              ref<Expr> address, KType *objectType,
                              IDType &result, bool &success) const {
  MOPredicate predicate([](const MemoryObject *mo) { return true; });
  if (UseTimestamps) {
    ref<Expr> base =
        state.isGEPExpr(address) ? state.gepExprBases[address].first : address;
    unsigned timestamp = UINT_MAX;
    if (!isa<ConstantExpr>(address)) {
      std::pair<ref<const MemoryObject>, ref<Expr>> moBasePair;
      if (state.getBase(base, moBasePair)) {
        timestamp = moBasePair.first->timestamp;
      }
    }
    predicate = [timestamp, predicate](const MemoryObject *mo) {
      return predicate(mo) && mo->timestamp <= timestamp;
    };
  }
  if (SkipNotSymbolicObjects) {
    predicate = [&state, predicate](const MemoryObject *mo) {
      return predicate(mo) && state.inSymbolics(mo);
    };
  }
  if (SkipNotLazyInitialized) {
    predicate = [predicate](const MemoryObject *mo) {
      return predicate(mo) && mo->isLazyInitialized();
    };
  }

  return resolveOne(state, solver, address, objectType, result, predicate,
                    success);
}

int AddressSpace::checkPointerInObject(ExecutionState &state,
                                       TimingSolver *solver, ref<Expr> p,
                                       const ObjectPair &op, ResolutionList &rl,
                                       unsigned maxResolutions) const {
  // XXX in the common case we can save one query if we ask
  // mustBeTrue before mayBeTrue for the first result. easy
  // to add I just want to have a nice symbolic test case first.
  const MemoryObject *mo = op.first;
  ref<Expr> inBounds = mo->getBoundsCheckPointer(p);
  bool mayBeTrue;
  if (!solver->mayBeTrue(state.constraints, inBounds, mayBeTrue,
                         state.queryMetaData)) {
    return 1;
  }

  if (mayBeTrue) {
    rl.push_back(mo->id);

    // fast path check
    auto size = rl.size();
    if (size == 1) {
      bool mustBeTrue;
      if (!solver->mustBeTrue(state.constraints, inBounds, mustBeTrue,
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
                           ref<Expr> p, KType *objectType, ResolutionList &rl,
                           ResolutionList &rlSkipped, MOPredicate predicate,
                           unsigned maxResolutions, time::Span timeout) const {
  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(p)) {
    IDType res;
    if (resolveOne(CE, objectType, res)) {
      rl.push_back(res);
      return false;
    }
  }
  TimerStatIncrementer timer(stats::resolveTime);

  IDType fastPathObjectID;
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
    if (!predicate(mo))
      continue;
    if (!oi->second->isAccessableFrom(objectType)) {
      rlSkipped.push_back(mo->id);
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

bool AddressSpace::resolve(ExecutionState &state, TimingSolver *solver,
                           ref<Expr> p, KType *objectType, ResolutionList &rl,
                           ResolutionList &rlSkipped, unsigned maxResolutions,
                           time::Span timeout) const {
  MOPredicate predicate([](const MemoryObject *mo) { return true; });
  if (UseTimestamps) {
    ref<Expr> base = state.isGEPExpr(p) ? state.gepExprBases[p].first : p;
    unsigned timestamp = UINT_MAX;
    if (!isa<ConstantExpr>(p)) {
      std::pair<ref<const MemoryObject>, ref<Expr>> moBasePair;
      if (state.getBase(base, moBasePair)) {
        timestamp = moBasePair.first->timestamp;
      }
    }
    predicate = [timestamp, predicate](const MemoryObject *mo) {
      return predicate(mo) && mo->timestamp <= timestamp;
    };
  }
  if (SkipNotSymbolicObjects) {
    predicate = [&state, predicate](const MemoryObject *mo) {
      return predicate(mo) && state.inSymbolics(mo);
    };
  }
  if (SkipNotLazyInitialized) {
    predicate = [predicate](const MemoryObject *mo) {
      return predicate(mo) && mo->isLazyInitialized();
    };
  }

  return resolve(state, solver, p, objectType, rl, rlSkipped, predicate,
                 maxResolutions, timeout);
}

// These two are pretty big hack so we can sort of pass memory back
// and forth to externals. They work by abusing the concrete cache
// store inside of the object states, which allows them to
// transparently avoid screwing up symbolics (if the byte is symbolic
// then its concrete cache byte isn't being used) but is just a hack.

void AddressSpace::copyOutConcretes() {
  for (MemoryMap::iterator it = objects.begin(), ie = objects.end(); it != ie;
       ++it) {
    const MemoryObject *mo = it->first;

    if (!mo->isUserSpecified) {
      const auto &os = it->second;
      auto address = reinterpret_cast<std::uint8_t *>(mo->address);

      if (!os->readOnly)
        memcpy(address, os->concreteStore, mo->size);
    }
  }
}

bool AddressSpace::copyInConcretes() {
  for (auto &obj : objects) {
    const MemoryObject *mo = obj.first;

    if (!mo->isUserSpecified) {
      const auto &os = obj.second;

      if (!copyInConcrete(mo, os.get(), mo->address))
        return false;
    }
  }

  return true;
}

bool AddressSpace::copyInConcrete(const MemoryObject *mo, const ObjectState *os,
                                  uint64_t src_address) {
  auto address = reinterpret_cast<std::uint8_t *>(src_address);
  if (memcmp(address, os->concreteStore, mo->size) != 0) {
    if (os->readOnly) {
      return false;
    } else {
      ObjectState *wos = getWriteable(mo, os);
      memcpy(wos->concreteStore, address, mo->size);
    }
  }
  return true;
}

/***/

bool MemoryObjectLT::operator()(const MemoryObject *a,
                                const MemoryObject *b) const {
  return a->address < b->address;
}
