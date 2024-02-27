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
#include "klee/Statistics/TimerStatIncrementer.h"

#include "CoreStats.h"

using namespace klee;

///

void AddressSpace::bindObject(const MemoryObject *mo, ObjectState *os) {
  assert(os->copyOnWriteOwner==0 && "object already has owner");
  os->copyOnWriteOwner = cowKey;
  objects = objects.replace(std::make_pair(mo, os));
}

void AddressSpace::unbindObject(const MemoryObject *mo) {
  objects = objects.remove(mo);
}

const ObjectState *AddressSpace::findObject(const MemoryObject *mo) const {
  const auto res = objects.lookup(mo);
  return res ? res->second.get() : nullptr;
}

ObjectState *AddressSpace::getWriteable(const MemoryObject *mo,
                                        const ObjectState *os) {
  assert(!os->readOnly);

  // If this address space owns they object, return it
  if (cowKey == os->copyOnWriteOwner)
    return const_cast<ObjectState*>(os);

  // Add a copy of this object state that can be updated
  ref<ObjectState> newObjectState(new ObjectState(*os));
  newObjectState->copyOnWriteOwner = cowKey;
  objects = objects.replace(std::make_pair(mo, newObjectState));
  return newObjectState.get();
}

/// 

bool AddressSpace::resolveOne(const ref<ConstantExpr> &addr, 
                              ObjectPair &result) const {
  uint64_t address = addr->getZExtValue();
  MemoryObject hack(address);

  if (const auto res = objects.lookup_previous(&hack)) {
    const auto &mo = res->first;
    // Check if the provided address is between start and end of the object
    // [mo->address, mo->address + mo->size) or the object is a 0-sized object.
    if ((mo->size==0 && address==mo->address) ||
        (address - mo->address < mo->size)) {
      result.first = res->first;
      result.second = res->second.get();
      return true;
    }
  }

  return false;
}

bool AddressSpace::resolveOne(ExecutionState &state,
                              TimingSolver *solver,
                              ref<Expr> address,
                              ObjectPair &result,
                              bool &success) const {
  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(address)) {
    success = resolveOne(CE, result);
    return true;
  } else {
    TimerStatIncrementer timer(stats::resolveTime);

    // try cheap search, will succeed for any inbounds pointer

    ref<ConstantExpr> cex;
    if (!solver->getValue(state.constraints, address, cex, state.queryMetaData))
      return false;
    uint64_t example = cex->getZExtValue();
    MemoryObject hack(example);
    const auto res = objects.lookup_previous(&hack);

    if (res) {
      const MemoryObject *mo = res->first;
      if (example - mo->address < mo->size) {
        result.first = res->first;
        result.second = res->second.get();
        success = true;
        return true;
      }
    }

    // didn't work, now we have to search
       
    MemoryMap::iterator oi = objects.upper_bound(&hack);
    MemoryMap::iterator begin = objects.begin();
    MemoryMap::iterator end = objects.end();
      
    MemoryMap::iterator start = oi;
    while (oi!=begin) {
      --oi;
      const auto &mo = oi->first;

      bool mayBeTrue;
      if (!solver->mayBeTrue(state.constraints,
                             mo->getBoundsCheckPointer(address), mayBeTrue,
                             state.queryMetaData))
        return false;
      if (mayBeTrue) {
        result.first = oi->first;
        result.second = oi->second.get();
        success = true;
        return true;
      } else {
        bool mustBeTrue;
        if (!solver->mustBeTrue(state.constraints,
                                UgeExpr::create(address, mo->getBaseExpr()),
                                mustBeTrue, state.queryMetaData))
          return false;
        if (mustBeTrue)
          break;
      }
    }

    // search forwards
    for (oi=start; oi!=end; ++oi) {
      const auto &mo = oi->first;

      bool mustBeTrue;
      if (!solver->mustBeTrue(state.constraints,
                              UltExpr::create(address, mo->getBaseExpr()),
                              mustBeTrue, state.queryMetaData))
        return false;
      if (mustBeTrue) {
        break;
      } else {
        bool mayBeTrue;

        if (!solver->mayBeTrue(state.constraints,
                               mo->getBoundsCheckPointer(address), mayBeTrue,
                               state.queryMetaData))
          return false;
        if (mayBeTrue) {
          result.first = oi->first;
          result.second = oi->second.get();
          success = true;
          return true;
        }
      }
    }

    success = false;
    return true;
  }
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
    rl.push_back(op);

    // fast path check
    auto size = rl.size();
    if (size == 1) {
      bool mustBeTrue;
      if (!solver->mustBeTrue(state.constraints, inBounds, mustBeTrue,
                              state.queryMetaData))
        return 1;
      if (mustBeTrue)
        return 0;
    }
    else
      if (size == maxResolutions)
        return 1;
  }

  return 2;
}

bool AddressSpace::resolve(ExecutionState &state, TimingSolver *solver,
                           ref<Expr> p, ResolutionList &rl,
                           unsigned maxResolutions, time::Span timeout) const {
  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(p)) {
    ObjectPair res;
    if (resolveOne(CE, res))
      rl.push_back(res);
    return false;
  } else {
    TimerStatIncrementer timer(stats::resolveTime);

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

    ref<ConstantExpr> cex;
    if (!solver->getValue(state.constraints, p, cex, state.queryMetaData))
      return true;
    uint64_t example = cex->getZExtValue();
    MemoryObject hack(example);

    MemoryMap::iterator oi = objects.upper_bound(&hack);
    MemoryMap::iterator begin = objects.begin();
    MemoryMap::iterator end = objects.end();

    MemoryMap::iterator start = oi;
    // search backwards, start with one minus because this
    // is the object that p *should* be within, which means we
    // get write off the end with 4 queries
    while (oi != begin) {
      --oi;
      const MemoryObject *mo = oi->first;
      if (timeout && timeout < timer.delta())
        return true;

      auto op = std::make_pair<>(mo, oi->second.get());

      int incomplete =
          checkPointerInObject(state, solver, p, op, rl, maxResolutions);
      if (incomplete != 2)
        return incomplete ? true : false;

      bool mustBeTrue;
      if (!solver->mustBeTrue(state.constraints,
                              UgeExpr::create(p, mo->getBaseExpr()), mustBeTrue,
                              state.queryMetaData))
        return true;
      if (mustBeTrue)
        break;
    }

    // search forwards
    for (oi = start; oi != end; ++oi) {
      const MemoryObject *mo = oi->first;
      if (timeout && timeout < timer.delta())
        return true;

      bool mustBeTrue;
      if (!solver->mustBeTrue(state.constraints,
                              UltExpr::create(p, mo->getBaseExpr()), mustBeTrue,
                              state.queryMetaData))
        return true;
      if (mustBeTrue)
        break;
      auto op = std::make_pair<>(mo, oi->second.get());

      int incomplete =
          checkPointerInObject(state, solver, p, op, rl, maxResolutions);
      if (incomplete != 2)
        return incomplete ? true : false;
    }
  }

  return false;
}

// These two are pretty big hack so we can sort of pass memory back
// and forth to externals. They work by abusing the concrete cache
// store inside of the object states, which allows them to
// transparently avoid screwing up symbolics (if the byte is symbolic
// then its concrete cache byte isn't being used) but is just a hack.

std::size_t AddressSpace::copyOutConcretes() {
  std::size_t numPages{};
  for (const auto &object : objects) {
    auto &mo = object.first;
    auto &os = object.second;
    if (!mo->isUserSpecified && !os->readOnly && os->size != 0) {
      auto size = std::max(os->size, mo->alignment);
      numPages +=
          (size + MemoryManager::pageSize - 1) / MemoryManager::pageSize;
      copyOutConcrete(mo, os.get());
    }
  }
  return numPages;
}

void AddressSpace::copyOutConcrete(const MemoryObject *mo,
                                   const ObjectState *os) const {
  auto address = reinterpret_cast<std::uint8_t *>(mo->address);
  std::memcpy(address, os->concreteStore, mo->size);
}

bool AddressSpace::copyInConcretes(bool concretize) {
  for (auto &obj : objects) {
    const MemoryObject *mo = obj.first;

    if (!mo->isUserSpecified) {
      const auto &os = obj.second;

      if (!copyInConcrete(mo, os.get(), mo->address, concretize))
        return false;
    }
  }

  return true;
}

bool AddressSpace::copyInConcrete(const MemoryObject *mo, const ObjectState *os,
                                  uint64_t src_address, bool concretize) {
  auto address = reinterpret_cast<std::uint8_t*>(src_address);

  // Don't do anything if the underlying representation has not been changed
  // externally.
  if (std::memcmp(address, os->concreteStore, mo->size) == 0)
    return true;

  // External object representation has been changed

  // Return `false` if the object is marked as read-only
  if (os->readOnly)
    return false;

  ObjectState *wos = getWriteable(mo, os);
  // Check if the object is fully concrete object. If so, use the fast
  // path and `memcpy` the new values from the external object to the internal
  // representation
  if (!wos->unflushedMask) {
    std::memcpy(wos->concreteStore, address, mo->size);
    return true;
  }

  // Check if object should be concretized
  if (concretize) {
    wos->makeConcrete();
    std::memcpy(wos->concreteStore, address, mo->size);
  } else {
    // The object is partially symbolic, it needs to be updated byte-by-byte
    // via object state's `write` function
    for (size_t i = 0, ie = mo->size; i < ie; ++i) {
      u_int8_t external_byte_value = *(address + i);
      if (external_byte_value != wos->concreteStore[i])
        wos->write8(i, external_byte_value);
    }
  }
  return true;
}

/***/

bool MemoryObjectLT::operator()(const MemoryObject *a, const MemoryObject *b) const {
  return a->address < b->address;
}

