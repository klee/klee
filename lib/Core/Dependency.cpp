//===-- Dependency.cpp - Memory location dependency -------------*- C++ -*-===//
//
//               The Tracer-X KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the implementation of the dependency analysis to
/// compute the locations upon which the unsatisfiability core depends,
/// which is used in computing the interpolant.
///
//===----------------------------------------------------------------------===//

#include "Dependency.h"
#include "ShadowArray.h"
#include "TxPrintUtil.h"

#include "klee/CommandLine.h"
#include "klee/Internal/Support/ErrorHandling.h"

#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 5)
#include <llvm/IR/DebugInfo.h>
#elif LLVM_VERSION_CODE >= LLVM_VERSION(3, 2)
#include <llvm/DebugInfo.h>
#else
#include <llvm/Analysis/DebugInfo.h>
#endif

#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 3)
#include <llvm/IR/Constants.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/Intrinsics.h>
#else
#include <llvm/Constants.h>
#include <llvm/DataLayout.h>
#include <llvm/Intrinsics.h>
#endif

using namespace klee;

namespace klee {

void StoredValue::init(ref<VersionedValue> vvalue,
                       std::set<const Array *> &replacements,
                       const std::set<std::string> &_coreReasons,
                       bool shadowing) {

  refCount = 0;
  id = reinterpret_cast<uintptr_t>(this);
  expr = shadowing ? ShadowArray::getShadowExpression(vvalue->getExpression(),
                                                      replacements)
                   : vvalue->getExpression();
  value = vvalue->getValue();

  doNotUseBound = !(vvalue->canInterpolateBound());

  coreReasons = _coreReasons;

  if (doNotUseBound)
    return;

  const std::set<ref<MemoryLocation> > locations = vvalue->getLocations();

  if (!locations.empty()) {
    // Here we compute memory bounds for checking pointer values. The memory
    // bound is the size of the allocation minus the offset; this is the weakest
    // precondition (interpolant) of memory bound checks done by KLEE.
    for (std::set<ref<MemoryLocation> >::const_iterator it = locations.begin(),
                                                        ie = locations.end();
         it != ie; ++it) {
      llvm::Value *v = (*it)->getValue(); // The allocation site

      // Concrete bound
      uint64_t concreteBound = (*it)->getConcreteOffsetBound();
      std::set<ref<Expr> > newBounds;

      if (concreteBound > 0)
        allocationBounds[v].insert(Expr::createPointer(concreteBound));

      // Symbolic bounds
      std::set<ref<Expr> > bounds = (*it)->getSymbolicOffsetBounds();

      if (shadowing) {
        std::set<ref<Expr> > shadowBounds;
        for (std::set<ref<Expr> >::iterator it1 = bounds.begin(),
                                            ie1 = bounds.end();
             it1 != ie1; ++it1) {
          shadowBounds.insert(
              ShadowArray::getShadowExpression(*it1, replacements));
        }
        bounds = shadowBounds;
      }

      if (!bounds.empty())
        allocationBounds[v].insert(bounds.begin(), bounds.end());

      ref<Expr> offset = shadowing ? ShadowArray::getShadowExpression(
                                         (*it)->getOffset(), replacements)
                                   : (*it)->getOffset();

      // We next build the offsets to be compared against stored allocation
      // offset bounds
      ConstantExpr *oe = llvm::dyn_cast<ConstantExpr>(offset);
      if (oe && !allocationOffsets[v].empty()) {
        // Here we check if smaller offset exists, in which case we replace it
        // with the new offset; as we want the greater offset to possibly
        // violate an offset bound.
        std::set<ref<Expr> > res;
        uint64_t offsetInt = oe->getZExtValue();
        for (std::set<ref<Expr> >::iterator it1 = allocationOffsets[v].begin(),
                                            ie1 = allocationOffsets[v].end();
             it1 != ie1; ++it1) {
          if (ConstantExpr *ce = llvm::dyn_cast<ConstantExpr>(*it1)) {
            uint64_t c = ce->getZExtValue();
            if (offsetInt > c) {
              res.insert(offset);
              continue;
            }
          }
          res.insert(*it1);
        }
        allocationOffsets[v] = res;
      } else {
        allocationOffsets[v].insert(offset);
      }
    }
  }
}

ref<Expr> StoredValue::getBoundsCheck(ref<StoredValue> stateValue,
                                      std::set<ref<Expr> > &bounds,
                                      int debugSubsumptionLevel) const {
  ref<Expr> res;
#ifdef ENABLE_Z3

  // In principle, for a state to be subsumed, the subsuming state must be
  // weaker, which in this case means that it should specify less allocations,
  // so all allocations in the subsuming (this), should be specified by the
  // subsumed (the stateValue argument), and we iterate over allocation of
  // the current object and for each such allocation, retrieve the
  // information from the argument object; in this way resulting in
  // less iterations compared to doing it the other way around.
  bool matchFound = false;
  for (std::map<llvm::Value *, std::set<ref<Expr> > >::const_iterator
           it = allocationBounds.begin(),
           ie = allocationBounds.end();
       it != ie; ++it) {
    std::set<ref<Expr> > tabledBounds = it->second;
    std::map<llvm::Value *, std::set<ref<Expr> > >::iterator iter =
        stateValue->allocationOffsets.find(it->first);
    if (iter == stateValue->allocationOffsets.end()) {
      continue;
    }
    matchFound = true;

    std::set<ref<Expr> > stateOffsets = iter->second;

    assert(!tabledBounds.empty() && "tabled bounds empty");

    if (stateOffsets.empty()) {
      if (debugSubsumptionLevel >= 3) {
        std::string msg;
        llvm::raw_string_ostream stream(msg);
        it->first->print(stream);
        stream.flush();
        klee_message("No offset defined in state for %s", msg.c_str());
      }
      return ConstantExpr::create(0, Expr::Bool);
    }

    for (std::set<ref<Expr> >::const_iterator it1 = stateOffsets.begin(),
                                              ie1 = stateOffsets.end();
         it1 != ie1; ++it1) {
      for (std::set<ref<Expr> >::const_iterator it2 = tabledBounds.begin(),
                                                ie2 = tabledBounds.end();
           it2 != ie2; ++it2) {
        if (ConstantExpr *tabledBound = llvm::dyn_cast<ConstantExpr>(*it2)) {
          uint64_t tabledBoundInt = tabledBound->getZExtValue();
          if (ConstantExpr *stateOffset = llvm::dyn_cast<ConstantExpr>(*it1)) {
            if (tabledBoundInt > 0) {
              uint64_t stateOffsetInt = stateOffset->getZExtValue();
              if (stateOffsetInt >= tabledBoundInt) {
                if (debugSubsumptionLevel >= 3) {
                  std::string msg;
                  llvm::raw_string_ostream stream(msg);
                  it->first->print(stream);
                  stream.flush();
                  klee_message("Offset %lu out of bound %lu for %s",
                               stateOffsetInt, tabledBoundInt, msg.c_str());
                }
                return ConstantExpr::create(0, Expr::Bool);
              }
            }
          }

          if (tabledBoundInt > 0) {
            // Symbolic state offset, but concrete tabled bound. Here the bound
            // is known (non-zero), so we create constraints
            if (res.isNull()) {
              res = UltExpr::create(*it1, *it2);
            } else {
              res = AndExpr::create(UltExpr::create(*it1, *it2), res);
            }
            bounds.insert(*it2);
          }
          continue;
        }
        // Create constraints for symbolic bounds
        if (res.isNull()) {
          res = UltExpr::create(*it1, *it2);
        } else {
          res = AndExpr::create(UltExpr::create(*it1, *it2), res);
        }
        bounds.insert(*it2);
      }
    }
  }

  // Bounds check successful if no constraints added
  if (res.isNull()) {
    if (matchFound)
      return ConstantExpr::create(1, Expr::Bool);
    else
      return ConstantExpr::create(0, Expr::Bool);
  }
#endif // ENABLE_Z3
  return res;
}

void StoredValue::print(llvm::raw_ostream &stream) const { print(stream, ""); }

void StoredValue::print(llvm::raw_ostream &stream,
                        const std::string &prefix) const {
  std::string nextTabs = appendTab(prefix);

  if (!doNotUseBound && !allocationBounds.empty()) {
    stream << prefix << "BOUNDS:";
    for (std::map<llvm::Value *, std::set<ref<Expr> > >::const_iterator
             it = allocationBounds.begin(),
             ie = allocationBounds.end();
         it != ie; ++it) {
      std::set<ref<Expr> > boundsSet = it->second;
      stream << "\n";
      stream << prefix << "[";
      it->first->print(stream);
      stream << "<={";
      for (std::set<ref<Expr> >::const_iterator it1 = it->second.begin(),
                                                is1 = it1,
                                                ie1 = it->second.end();
           it1 != ie1; ++it1) {
        if (it1 != is1)
          stream << ",";
        (*it1)->print(stream);
      }
      stream << "}]";
    }

    if (!allocationOffsets.empty()) {
      stream << "\n";
      stream << prefix << "OFFSETS:";
      for (std::map<llvm::Value *, std::set<ref<Expr> > >::const_iterator
               it = allocationOffsets.begin(),
               ie = allocationOffsets.end();
           it != ie; ++it) {
        std::set<ref<Expr> > boundsSet = it->second;
        stream << "\n";
        stream << prefix << "[";
        it->first->print(stream);
        stream << "=={";
        for (std::set<ref<Expr> >::const_iterator it1 = it->second.begin(),
                                                  is1 = it1,
                                                  ie1 = it->second.end();
             it1 != ie1; ++it1) {
          if (it1 != is1)
            stream << ",";
          (*it1)->print(stream);
        }
        stream << "}]";
      }
    }
  } else {
    stream << prefix;
    expr->print(stream);
  }

  if (!coreReasons.empty()) {
    stream << "\n";
    stream << prefix << "reason(s) for storage:\n";
    for (std::set<std::string>::const_iterator is = coreReasons.begin(),
                                               ie = coreReasons.end(), it = is;
         it != ie; ++it) {
      if (it != is)
        stream << "\n";
      stream << nextTabs << *it;
    }
  }
}

/**/

bool Dependency::isMainArgument(llvm::Value *loc) {
  llvm::Argument *vArg = llvm::dyn_cast<llvm::Argument>(loc);

  // FIXME: We need a more precise way to detect main argument
  if (vArg && vArg->getParent() &&
      (vArg->getParent()->getName().equals("main") ||
       vArg->getParent()->getName().equals("__user_main"))) {
    return true;
  }
  return false;
}

ref<VersionedValue>
Dependency::registerNewVersionedValue(llvm::Value *value,
                                      ref<VersionedValue> vvalue) {
  valuesMap[value].push_back(vvalue);
  return vvalue;
}

std::pair<Dependency::ConcreteStore, Dependency::SymbolicStore>
Dependency::getStoredExpressions(const std::vector<llvm::Instruction *> &stack,
                                 std::set<const Array *> &replacements,
                                 bool coreOnly) {
  ConcreteStore concreteStore;
  SymbolicStore symbolicStore;

  for (std::map<ref<MemoryLocation>,
                std::pair<ref<VersionedValue>, ref<VersionedValue> > >::iterator
           it = concretelyAddressedStore.begin(),
           ie = concretelyAddressedStore.end();
       it != ie; ++it) {
    if (!it->first->contextIsPrefixOf(stack))
      continue;
    if (it->second.second.isNull())
      continue;

    if (!coreOnly) {
      llvm::Value *base = it->first->getValue();
      concreteStore[base][it->first] = StoredValue::create(it->second.second);
    } else if (it->second.second->isCore()) {
      // An address is in the core if it stores a value that is in the core
      llvm::Value *base = it->first->getValue();
#ifdef ENABLE_Z3
      if (!NoExistential) {
        concreteStore[base][it->first] =
            StoredValue::create(it->second.second, replacements);
      } else
#endif
        concreteStore[base][it->first] = StoredValue::create(it->second.second);
    }
  }

  for (std::map<ref<MemoryLocation>,
                std::pair<ref<VersionedValue>, ref<VersionedValue> > >::iterator
           it = symbolicallyAddressedStore.begin(),
           ie = symbolicallyAddressedStore.end();
       it != ie; ++it) {
    if (!it->first->contextIsPrefixOf(stack))
      continue;

    if (it->second.second.isNull())
      continue;

    if (!coreOnly) {
      llvm::Value *base = it->first->getValue();
      symbolicStore[base].push_back(
          AddressValuePair(it->first, StoredValue::create(it->second.second)));
    } else if (it->second.second->isCore()) {
      // An address is in the core if it stores a value that is in the core
      llvm::Value *base = it->first->getValue();
#ifdef ENABLE_Z3
      if (!NoExistential) {
        symbolicStore[base].push_back(AddressValuePair(
            MemoryLocation::create(it->first, replacements),
            StoredValue::create(it->second.second, replacements)));
      } else
#endif
        symbolicStore[base].push_back(AddressValuePair(
            it->first, StoredValue::create(it->second.second)));
      }
  }

  return std::pair<ConcreteStore, SymbolicStore>(concreteStore, symbolicStore);
}

ref<VersionedValue>
Dependency::getLatestValue(llvm::Value *value,
                           const std::vector<llvm::Instruction *> &stack,
                           ref<Expr> valueExpr, bool constraint) {
  assert(value && !valueExpr.isNull() && "value cannot be null");
  if (llvm::isa<llvm::ConstantExpr>(value)) {
    llvm::Instruction *asInstruction =
        llvm::dyn_cast<llvm::ConstantExpr>(value)->getAsInstruction();
    if (llvm::GetElementPtrInst *gi =
            llvm::dyn_cast<llvm::GetElementPtrInst>(asInstruction)) {
      uint64_t offset = 0;
      uint64_t size = 0;

      // getelementptr may be cascading, so we loop
      while (gi) {
        if (gi->getNumOperands() >= 2) {
          if (llvm::ConstantInt *idx =
                  llvm::dyn_cast<llvm::ConstantInt>(gi->getOperand(1))) {
            offset += idx->getLimitedValue();
          }
        }
        llvm::Value *ptrOp = gi->getPointerOperand();
        llvm::Type *pointerElementType =
            ptrOp->getType()->getPointerElementType();

        size = pointerElementType->isSized()
                   ? targetData->getTypeStoreSize(pointerElementType)
                   : 0;

        if (llvm::isa<llvm::ConstantExpr>(ptrOp)) {
          llvm::Instruction *asInstruction =
              llvm::dyn_cast<llvm::ConstantExpr>(ptrOp)->getAsInstruction();
          gi = llvm::dyn_cast<llvm::GetElementPtrInst>(asInstruction);
          continue;
        }

        gi = 0;
      }

      return getNewPointerValue(value, stack, valueExpr, size);
    } else if (llvm::isa<llvm::IntToPtrInst>(asInstruction)) {
	// 0 signifies unknown size
      return getNewPointerValue(value, stack, valueExpr, 0);
    }
  }

  // A global value is a constant: Its value is constant throughout execution,
  // but
  // indeterministic. In case this was a non-global-value (normal) constant, we
  // immediately return with a versioned value, as dependencies are not
  // important. However, the dependencies of global values should be searched
  // for in the ancestors (later) as they need to be consistent in an execution.
  if (llvm::isa<llvm::Constant>(value) && !llvm::isa<llvm::GlobalValue>(value))
    return getNewVersionedValue(value, stack, valueExpr);

  if (valuesMap.find(value) != valuesMap.end()) {
    // Slight complication here that the latest version of an LLVM
    // value may not be at the end of the vector; it is possible other
    // values in a call stack has been appended to the vector, before
    // the function returned, so the end part of the vector contains
    // local values in a call already returned. To resolve this issue,
    // here we naively search for values with equivalent expression.
    std::vector<ref<VersionedValue> > allValues = valuesMap[value];

    // In case this was for adding constraints, simply assume the
    // latest value is the one. This is due to the difficulty in
    // that the constraint in valueExpr is already processed into
    // a different syntax.
    if (constraint)
      return allValues.back();

    for (std::vector<ref<VersionedValue> >::reverse_iterator
             it = allValues.rbegin(),
             ie = allValues.rend();
         it != ie; ++it) {
      ref<Expr> e = (*it)->getExpression();
      if (e == valueExpr)
        return *it;
    }
  }

  ref<VersionedValue> ret = 0;
  if (parent)
    ret = parent->getLatestValue(value, stack, valueExpr, constraint);

  if (ret.isNull()) {
    if (llvm::GlobalValue *gv = llvm::dyn_cast<llvm::GlobalValue>(value)) {
      // We could not find the global value: we register it anew.
      if (gv->getType()->isPointerTy()) {
        uint64_t size = 0;
        if (gv->getType()->getPointerElementType()->isSized())
          size = targetData->getTypeStoreSize(
              gv->getType()->getPointerElementType());
        ret = getNewPointerValue(value, stack, valueExpr, size);
      } else {
        ret = getNewVersionedValue(value, stack, valueExpr);
      }
    } else {
      llvm::StringRef name(value->getName());
      if (name.str() == "argc") {
        ret = getNewVersionedValue(value, stack, valueExpr);
      } else if (name.str() == "this" && value->getType()->isPointerTy()) {
        // For C++ "this" variable that is not found
        if (value->getType()->getPointerElementType()->isSized()) {
          uint64_t size = targetData->getTypeStoreSize(
              value->getType()->getPointerElementType());
          ret = getNewPointerValue(value, stack, valueExpr, size);
        }
      }
    }
  }
  return ret;
}

ref<VersionedValue>
Dependency::getLatestValueNoConstantCheck(llvm::Value *value,
                                          ref<Expr> valueExpr) {
  assert(value && "value cannot be null");

  if (valuesMap.find(value) != valuesMap.end()) {
    if (!valueExpr.isNull()) {
      // Slight complication here that the latest version of an LLVM
      // value may not be at the end of the vector; it is possible other
      // values in a call stack has been appended to the vector, before
      // the function returned, so the end part of the vector contains
      // local values in a call already returned. To resolve this issue,
      // here we naively search for values with equivalent expression.
      std::vector<ref<VersionedValue> > allValues = valuesMap[value];

      for (std::vector<ref<VersionedValue> >::reverse_iterator
               it = allValues.rbegin(),
               ie = allValues.rend();
           it != ie; ++it) {
        ref<Expr> e = (*it)->getExpression();
        if (e == valueExpr)
          return *it;
      }
    } else {
      return valuesMap[value].back();
    }
  }

  if (parent)
    return parent->getLatestValueNoConstantCheck(value, valueExpr);

  return 0;
}

ref<VersionedValue> Dependency::getLatestValueForMarking(llvm::Value *val,
                                                         ref<Expr> expr) {
  ref<VersionedValue> value = getLatestValueNoConstantCheck(val, expr);

  // Right now we simply ignore the __dso_handle values. They are due
  // to library / linking errors caused by missing options (-shared) in the
  // compilation involving shared library.
  if (value.isNull()) {
    if (llvm::ConstantExpr *cVal = llvm::dyn_cast<llvm::ConstantExpr>(val)) {
      for (unsigned i = 0; i < cVal->getNumOperands(); ++i) {
        if (cVal->getOperand(i)->getName().equals("__dso_handle")) {
          return value;
        }
      }
    }

    if (llvm::isa<llvm::Constant>(val))
      return value;

    assert(!"unknown value");
  }
  return value;
}

void Dependency::updateStore(ref<MemoryLocation> loc,
                             ref<VersionedValue> address,
                             ref<VersionedValue> value) {
  if (loc->hasConstantAddress())
    concretelyAddressedStore[loc] =
        std::pair<ref<VersionedValue>, ref<VersionedValue> >(address, value);
  else
    symbolicallyAddressedStore[loc] =
        std::pair<ref<VersionedValue>, ref<VersionedValue> >(address, value);
}

void Dependency::addDependency(ref<VersionedValue> source,
                               ref<VersionedValue> target,
                               bool multiLocationsCheck) {
  if (source.isNull() || target.isNull())
    return;

  assert((!multiLocationsCheck || target->getLocations().empty()) &&
         "should not add new location");

  addDependencyIntToPtr(source, target);
}

void Dependency::addDependencyIntToPtr(ref<VersionedValue> source,
                                       ref<VersionedValue> target) {
  ref<MemoryLocation> nullLocation;

  if (source.isNull() || target.isNull())
    return;

  std::set<ref<MemoryLocation> > locations = source->getLocations();
  ref<Expr> targetExpr(ZExtExpr::create(target->getExpression(),
                                        Expr::createPointer(0)->getWidth()));
  for (std::set<ref<MemoryLocation> >::iterator it = locations.begin(),
                                                ie = locations.end();
       it != ie; ++it) {
    ref<Expr> sourceBase((*it)->getBase());
    ref<Expr> offsetDelta(SubExpr::create(
        SubExpr::create(targetExpr, sourceBase), (*it)->getOffset()));
    target->addLocation(MemoryLocation::create(*it, targetExpr, offsetDelta));
  }
  target->addDependency(source, nullLocation);
}

void Dependency::addDependencyWithOffset(ref<VersionedValue> source,
                                         ref<VersionedValue> target,
                                         ref<Expr> offsetDelta) {
  ref<MemoryLocation> nullLocation;

  if (source.isNull() || target.isNull())
    return;

  std::set<ref<MemoryLocation> > locations = source->getLocations();
  ref<Expr> targetExpr(target->getExpression());

  ConstantExpr *ce = llvm::dyn_cast<ConstantExpr>(targetExpr);
  uint64_t a = ce ? ce->getZExtValue() : 0;

  ConstantExpr *de = llvm::dyn_cast<ConstantExpr>(offsetDelta);
  uint64_t d = de ? de->getZExtValue() : 0;

  uint64_t nLocations = locations.size();
  uint64_t i = 0;
  bool locationAdded = false;

  for (std::set<ref<MemoryLocation> >::iterator it = locations.begin(),
                                                ie = locations.end();
       it != ie; ++it) {
    ++i;

    ConstantExpr *be = llvm::dyn_cast<ConstantExpr>((*it)->getBase());
    uint64_t b = be ? be->getZExtValue() : 0;

    ConstantExpr *oe = llvm::dyn_cast<ConstantExpr>((*it)->getOffset());
    uint64_t o = (oe ? oe->getZExtValue() : 0) + d;

    // The following if conditional implements a mechanism to
    // only add memory locations that make sense; that is, when
    // the offset is address minus base
    if (ce && de && be && oe) {
      if (o != (a - b) && (b != 0) && (locationAdded || i < nLocations))
        continue;
    }
    target->addLocation(MemoryLocation::create(*it, targetExpr, offsetDelta));
    locationAdded = true;
  }
  target->addDependency(source, nullLocation);
}

void Dependency::addDependencyViaLocation(ref<VersionedValue> source,
                                          ref<VersionedValue> target,
                                          ref<MemoryLocation> via) {
  if (source.isNull() || target.isNull())
    return;

  std::set<ref<MemoryLocation> > locations = source->getLocations();
  for (std::set<ref<MemoryLocation> >::iterator it = locations.begin(),
                                                ie = locations.end();
       it != ie; ++it) {
    target->addLocation(*it);
  }
  target->addDependency(source, via);
}

void Dependency::addDependencyViaExternalFunction(
    const std::vector<llvm::Instruction *> &stack, ref<VersionedValue> source,
    ref<VersionedValue> target) {
  if (source.isNull() || target.isNull())
    return;

#ifdef ENABLE_Z3
  if (!NoBoundInterpolation) {
    std::set<ref<MemoryLocation> > locations = source->getLocations();
    if (!locations.empty()) {
      std::string reason = "";
      if (debugSubsumptionLevel >= 1) {
        llvm::raw_string_ostream stream(reason);
        stream << "parameter [";
        source->getValue()->print(stream);
        stream << "] of external call [";
        target->getValue()->print(stream);
        stream << "]";
        stream.flush();
      }
      markPointerFlow(source, source, reason);
    }
  }
#endif

  // Add new location to the target in case of pointer return value
  llvm::Type *t = target->getValue()->getType();
  if (t->isPointerTy() && target->getLocations().size() == 0) {
    uint64_t size = 0;
    ref<Expr> address(target->getExpression());

    llvm::Type *elementType = t->getPointerElementType();
    if (elementType->isSized()) {
      size = targetData->getTypeStoreSize(elementType);
    }

    target->addLocation(
        MemoryLocation::create(target->getValue(), stack, address, size));
  }

  addDependencyToNonPointer(source, target);
}

void Dependency::addDependencyToNonPointer(ref<VersionedValue> source,
                                           ref<VersionedValue> target) {
  if (source.isNull() || target.isNull())
    return;

  ref<MemoryLocation> nullLocation;
  target->addDependency(source, nullLocation);
}

std::vector<ref<VersionedValue> >
Dependency::directFlowSources(ref<VersionedValue> target) const {
  std::vector<ref<VersionedValue> > ret;
  std::map<ref<VersionedValue>, ref<MemoryLocation> > sources =
      target->getSources();
  ref<VersionedValue> loadAddress = target->getLoadAddress(),
                      storeAddress = target->getStoreAddress();

  for (std::map<ref<VersionedValue>, ref<MemoryLocation> >::iterator it =
           sources.begin();
       it != sources.end(); ++it) {
    ret.push_back(it->first);
  }

  if (!loadAddress.isNull()) {
    ret.push_back(loadAddress);
    if (!storeAddress.isNull() && storeAddress != loadAddress) {
      ret.push_back(storeAddress);
    }
  } else if (!storeAddress.isNull()) {
    ret.push_back(storeAddress);
  }

  return ret;
}

void Dependency::markFlow(ref<VersionedValue> target,
                          const std::string &reason) const {
  if (target.isNull() || (target->isCore() && !target->canInterpolateBound()))
    return;

  target->setAsCore(reason);
  target->disableBoundInterpolation();

  std::vector<ref<VersionedValue> > stepSources = directFlowSources(target);
  for (std::vector<ref<VersionedValue> >::iterator it = stepSources.begin(),
                                                   ie = stepSources.end();
       it != ie; ++it) {
    markFlow(*it, reason);
  }
}

void Dependency::markPointerFlow(ref<VersionedValue> target,
                                 ref<VersionedValue> checkedAddress,
                                 std::set<ref<Expr> > &bounds,
                                 const std::string &reason) const {
  if (target.isNull())
    return;

  if (target->canInterpolateBound()) {
    //  checkedAddress->dump();
    std::set<ref<MemoryLocation> > locations = target->getLocations();
    for (std::set<ref<MemoryLocation> >::iterator it = locations.begin(),
                                                  ie = locations.end();
         it != ie; ++it) {
      (*it)->adjustOffsetBound(checkedAddress, bounds);
    }
  }
  target->setAsCore(reason);

  // Compute the direct pointer flow dependency
  std::map<ref<VersionedValue>, ref<MemoryLocation> > sources =
      target->getSources();

  for (std::map<ref<VersionedValue>, ref<MemoryLocation> >::iterator
           it = sources.begin(),
           ie = sources.end();
       it != ie; ++it) {
    markPointerFlow(it->first, checkedAddress, bounds, reason);
  }

  // We use normal marking with markFlow for load/store addresses
  markFlow(target->getLoadAddress(), reason);
  markFlow(target->getStoreAddress(), reason);
}

std::vector<ref<VersionedValue> > Dependency::populateArgumentValuesList(
    llvm::CallInst *site, const std::vector<llvm::Instruction *> &stack,
    std::vector<ref<Expr> > &arguments) {
  unsigned numArgs = site->getCalledFunction()->arg_size();
  std::vector<ref<VersionedValue> > argumentValuesList;
  for (unsigned i = numArgs; i > 0;) {
    llvm::Value *argOperand = site->getArgOperand(--i);
    ref<VersionedValue> latestValue =
        getLatestValue(argOperand, stack, arguments.at(i));

    if (!latestValue.isNull())
      argumentValuesList.push_back(latestValue);
    else {
      // This is for the case when latestValue was NULL, which means there is
      // no source dependency information for this node, e.g., a constant.
      argumentValuesList.push_back(
          VersionedValue::create(argOperand, stack, arguments[i]));
    }
  }
  return argumentValuesList;
}

Dependency::Dependency(Dependency *parent, llvm::DataLayout *_targetData)
    : parent(parent), targetData(_targetData) {
  if (parent) {
    concretelyAddressedStore = parent->concretelyAddressedStore;
    symbolicallyAddressedStore = parent->symbolicallyAddressedStore;
    debugSubsumptionLevel = parent->debugSubsumptionLevel;
    debugState = parent->debugState;
  } else {
#ifdef ENABLE_Z3
    debugSubsumptionLevel = DebugSubsumption;
    debugState = DebugState;
#else
    debugSubsumptionLevel = 0;
    debugState = false;
#endif
  }
}

Dependency::~Dependency() {
  // Delete the locally-constructed relations
  concretelyAddressedStore.clear();
  symbolicallyAddressedStore.clear();

  // Delete valuesMap
  for (std::map<llvm::Value *, std::vector<ref<VersionedValue> > >::iterator
           it = valuesMap.begin(),
           ie = valuesMap.end();
       it != ie; ++it) {
    it->second.clear();
  }
  valuesMap.clear();
}

Dependency *Dependency::cdr() const { return parent; }

void Dependency::execute(llvm::Instruction *instr,
                         const std::vector<llvm::Instruction *> &stack,
                         std::vector<ref<Expr> > &args,
                         bool symbolicExecutionError) {
  // The basic design principle that we need to be careful here
  // is that we should not store quadratic-sized structures in
  // the database of computed relations, e.g., not storing the
  // result of traversals of the graph. We keep the
  // quadratic blow up for only when querying the database.

  if (llvm::isa<llvm::CallInst>(instr)) {
    llvm::CallInst *callInst = llvm::dyn_cast<llvm::CallInst>(instr);
    llvm::Function *f = callInst->getCalledFunction();

    if (!f) {
	// Handles the case when the callee is wrapped within another expression
	llvm::ConstantExpr *calledValue = llvm::dyn_cast<llvm::ConstantExpr>(callInst->getCalledValue());
	if (calledValue && calledValue->getOperand(0)) {
	    f = llvm::dyn_cast<llvm::Function>(calledValue->getOperand(0));
	}
    }

    if (f && f->getIntrinsicID() == llvm::Intrinsic::not_intrinsic) {
      llvm::StringRef calleeName = f->getName();
      // FIXME: We need a more precise way to determine invoked method
      // rather than just using the name.
      std::string getValuePrefix("klee_get_value");

      if (calleeName.equals("_Znwm") || calleeName.equals("_Znam")) {
        ConstantExpr *sizeExpr = llvm::dyn_cast<ConstantExpr>(args.at(1));
        getNewPointerValue(instr, stack, args.at(0), sizeExpr->getZExtValue());
      } else if ((calleeName.equals("getpagesize") && args.size() == 1) ||
                 (calleeName.equals("ioctl") && args.size() == 4) ||
                 (calleeName.equals("__ctype_b_loc") && args.size() == 1) ||
                 (calleeName.equals("__ctype_b_locargs") && args.size() == 1) ||
                 calleeName.equals("puts") || calleeName.equals("fflush") ||
                 calleeName.equals("strcmp") || calleeName.equals("strncmp") ||
                 (calleeName.equals("__errno_location") && args.size() == 1) ||
                 (calleeName.equals("geteuid") && args.size() == 1)) {
        getNewVersionedValue(instr, stack, args.at(0));
      } else if (calleeName.equals("_ZNSi5seekgElSt12_Ios_Seekdir") &&
                 args.size() == 4) {
        ref<VersionedValue> returnValue =
            getNewVersionedValue(instr, stack, args.at(0));
        for (unsigned i = 0; i < 3; ++i) {
          addDependencyViaExternalFunction(
              stack,
              getLatestValue(instr->getOperand(i), stack, args.at(i + 1)),
              returnValue);
        }
      } else if (calleeName.equals(
                     "_ZNSt13basic_fstreamIcSt11char_traitsIcEE7is_openEv") &&
                 args.size() == 2) {
        addDependencyViaExternalFunction(
            stack, getLatestValue(instr->getOperand(0), stack, args.at(1)),
            getNewVersionedValue(instr, stack, args.at(0)));
      } else if (calleeName.equals("_ZNSi5tellgEv") && args.size() == 2) {
        addDependencyViaExternalFunction(
            stack, getLatestValue(instr->getOperand(0), stack, args.at(1)),
            getNewVersionedValue(instr, stack, args.at(0)));
      } else if ((calleeName.equals("powl") && args.size() == 3) ||
                 (calleeName.equals("gettimeofday") && args.size() == 3)) {
        ref<VersionedValue> returnValue =
            getNewVersionedValue(instr, stack, args.at(0));
        for (unsigned i = 0; i < 2; ++i) {
          addDependencyViaExternalFunction(
              stack,
              getLatestValue(instr->getOperand(i), stack, args.at(i + 1)),
              returnValue);
        }
      } else if (calleeName.equals("malloc") && args.size() == 1) {
        // malloc is an location-type instruction. This is for the case when the
        // allocation size is unknown (0), so the
        // single argument here is the return address, for which KLEE provides
        // 0.
        getNewPointerValue(instr, stack, args.at(0), 0);
      } else if (calleeName.equals("malloc") && args.size() == 2) {
        // malloc is an location-type instruction. This is the case when it has
        // a determined size
        uint64_t size = 0;
        if (ConstantExpr *ce = llvm::dyn_cast<ConstantExpr>(args.at(1)))
          size = ce->getZExtValue();
        getNewPointerValue(instr, stack, args.at(0), size);
      } else if (calleeName.equals("realloc") && args.size() == 1) {
        // realloc is an location-type instruction: its single argument is the
        // return address.
        addDependency(getLatestValue(instr->getOperand(0), stack, args.at(0)),
                      getNewVersionedValue(instr, stack, args.at(0)));
      } else if (calleeName.equals("calloc") && args.size() == 1) {
        // calloc is a location-type instruction: its single argument is the
        // return address. We assume its allocation size is unknown
        getNewPointerValue(instr, stack, args.at(0), 0);
      } else if (calleeName.equals("syscall") && args.size() >= 2) {
        ref<VersionedValue> returnValue =
            getNewVersionedValue(instr, stack, args.at(0));
        for (unsigned i = 0; i + 1 < args.size(); ++i) {
          addDependencyViaExternalFunction(
              stack,
              getLatestValue(instr->getOperand(i), stack, args.at(i + 1)),
              returnValue);
        }
      } else if (std::mismatch(getValuePrefix.begin(), getValuePrefix.end(),
                               calleeName.begin()).first ==
                     getValuePrefix.end() &&
                 args.size() == 2) {
        addDependencyViaExternalFunction(
            stack, getLatestValue(instr->getOperand(0), stack, args.at(1)),
            getNewVersionedValue(instr, stack, args.at(0)));
      } else if (calleeName.equals("getenv") && args.size() == 2) {
        // We assume getenv has unknown allocation size
        getNewPointerValue(instr, stack, args.at(0), 0);
      } else if (calleeName.equals("printf") && args.size() >= 2) {
        ref<VersionedValue> returnValue =
            getNewVersionedValue(instr, stack, args.at(0));
        addDependencyViaExternalFunction(
            stack, getLatestValue(instr->getOperand(0), stack, args.at(1)),
            returnValue);
        for (unsigned i = 2, argsNum = args.size(); i < argsNum; ++i) {
          addDependencyViaExternalFunction(
              stack,
              getLatestValue(instr->getOperand(i - 1), stack, args.at(i)),
              returnValue);
        }
      } else if (calleeName.equals("vprintf") && args.size() == 3) {
        ref<VersionedValue> returnValue =
            getNewVersionedValue(instr, stack, args.at(0));
        addDependencyViaExternalFunction(
            stack, getLatestValue(instr->getOperand(0), stack, args.at(1)),
            returnValue);
        addDependencyViaExternalFunction(
            stack, getLatestValue(instr->getOperand(1), stack, args.at(2)),
            returnValue);
      } else if (((calleeName.equals("fchmodat") && args.size() == 5)) ||
                 (calleeName.equals("fchownat") && args.size() == 6)) {
        ref<VersionedValue> returnValue =
            getNewVersionedValue(instr, stack, args.at(0));
        for (unsigned i = 0; i < 2; ++i) {
          addDependencyViaExternalFunction(
              stack,
              getLatestValue(instr->getOperand(i), stack, args.at(i + 1)),
              returnValue);
        }
      } else {
        // Default external function handler: We ignore functions that return
        // void, and we DO NOT build dependency of return value to the
        // arguments.
        if (!instr->getType()->isVoidTy()) {
          assert(args.size() && "non-void call missing return expression");
          klee_warning("using default handler for external function %s",
                       calleeName.str().c_str());
          getNewVersionedValue(instr, stack, args.at(0));
        }
      }
    }
    return;
  }

  switch (args.size()) {
  case 0: {
    switch (instr->getOpcode()) {
    case llvm::Instruction::Br: {
      llvm::BranchInst *binst = llvm::dyn_cast<llvm::BranchInst>(instr);
      if (binst && binst->isConditional()) {
        ref<Expr> unknownExpression;
        std::string reason = "";
        if (debugSubsumptionLevel >= 1) {
          llvm::raw_string_ostream stream(reason);
          stream << "branch instruction [";
          if (binst->getParent()->getParent()) {
            stream << binst->getParent()->getParent()->getName().str() << ": ";
          }
          if (llvm::MDNode *n = binst->getMetadata("dbg")) {
            llvm::DILocation loc(n);
            stream << "Line " << loc.getLineNumber();
          } else {
            binst->print(stream);
          }
          stream << "]";
          stream.flush();
        }
        markAllValues(binst->getCondition(), unknownExpression, reason);
      }
      break;
    }
    default:
      break;
    }
    return;
  }
  case 1: {
    ref<Expr> argExpr = args.at(0);

    switch (instr->getOpcode()) {
    case llvm::Instruction::BitCast: {
      ref<VersionedValue> val =
          getLatestValue(instr->getOperand(0), stack, argExpr);

      if (!val.isNull()) {
        addDependency(val, getNewVersionedValue(instr, stack, argExpr));
      } else if (!llvm::isa<llvm::Constant>(instr->getOperand(0)))
          // Constants would kill dependencies, the remaining is for
          // cases that may actually require dependencies.
      {
        if (instr->getOperand(0)->getType()->isPointerTy()) {
          uint64_t size = targetData->getTypeStoreSize(
              instr->getOperand(0)->getType()->getPointerElementType());
          addDependency(
              getNewPointerValue(instr->getOperand(0), stack, argExpr, size),
              getNewVersionedValue(instr, stack, argExpr));
        } else if (llvm::isa<llvm::Argument>(instr->getOperand(0)) ||
                   llvm::isa<llvm::CallInst>(instr->getOperand(0)) ||
                   symbolicExecutionError) {
          addDependency(
              getNewVersionedValue(instr->getOperand(0), stack, argExpr),
              getNewVersionedValue(instr, stack, argExpr));
        } else {
          assert(!"operand not found");
        }
      }
      break;
    }
    default: { assert(!"unhandled unary instruction"); }
    }
    return;
  }
  case 2: {
    ref<Expr> valueExpr = args.at(0);
    ref<Expr> address = args.at(1);

    switch (instr->getOpcode()) {
    case llvm::Instruction::Alloca: {
      // In case of alloca, the valueExpr is the address, and address is the
      // allocation size.
      uint64_t size = 0;
      if (ConstantExpr *ce = llvm::dyn_cast<ConstantExpr>(address)) {
        size = ce->getZExtValue();
      }
      getNewPointerValue(instr, stack, valueExpr, size);
      break;
    }
    case llvm::Instruction::Load: {
      ref<VersionedValue> addressValue =
          getLatestValue(instr->getOperand(0), stack, address);
      llvm::Type *loadedType =
          instr->getOperand(0)->getType()->getPointerElementType();

      if (!addressValue.isNull()) {
        std::set<ref<MemoryLocation> > locations = addressValue->getLocations();
        if (locations.empty()) {
          // The size of the allocation is unknown here as the memory region
          // might have been allocated by the environment
          ref<MemoryLocation> loc =
              MemoryLocation::create(instr->getOperand(0), stack, address, 0);
          addressValue->addLocation(loc);

          // Build the loaded value
          ref<VersionedValue> loadedValue =
              loadedType->isPointerTy()
                  ? getNewPointerValue(instr, stack, valueExpr, 0)
                  : getNewVersionedValue(instr, stack, valueExpr);

          updateStore(loc, addressValue, loadedValue);
          break;
        } else if (locations.size() == 1) {
          ref<MemoryLocation> loc = *(locations.begin());
          if (isMainArgument(loc->getValue())) {
            // The load corresponding to a load of the main function's argument
            // that was never allocated within this program.

            // Build the loaded value
            ref<VersionedValue> loadedValue =
                loadedType->isPointerTy()
                    ? getNewPointerValue(instr, stack, valueExpr, 0)
                    : getNewVersionedValue(instr, stack, valueExpr);

            updateStore(loc, addressValue, loadedValue);
            break;
          }
        }
      } else {
        // assert(!"loaded allocation size must not be zero");
        addressValue =
            getNewPointerValue(instr->getOperand(0), stack, address, 0);

        if (llvm::isa<llvm::GlobalVariable>(instr->getOperand(0))) {
          // The value not found was a global variable, record it here.
          std::set<ref<MemoryLocation> > locations =
              addressValue->getLocations();

          // Build the loaded value
          ref<VersionedValue> loadedValue =
              loadedType->isPointerTy()
                  ? getNewPointerValue(instr, stack, valueExpr, 0)
                  : getNewVersionedValue(instr, stack, valueExpr);

          updateStore(*(locations.begin()), addressValue, loadedValue);
          break;
        }
      }

      std::set<ref<MemoryLocation> > locations = addressValue->getLocations();

      for (std::set<ref<MemoryLocation> >::iterator li = locations.begin(),
                                                    le = locations.end();
           li != le; ++li) {
        std::pair<ref<VersionedValue>, ref<VersionedValue> > addressValuePair;

        if ((*li)->hasConstantAddress()) {
          if (concretelyAddressedStore.count(*li) > 0) {
            addressValuePair = concretelyAddressedStore[*li];
          }
        } else if (symbolicallyAddressedStore.count(*li) > 0) {
          // FIXME: Here we assume that the expressions have to exactly be the
          // same expression object. More properly, this should instead add an
          // ite constraint onto the path condition.
          addressValuePair = concretelyAddressedStore[*li];
        }

        // Build the loaded value
        ref<VersionedValue> loadedValue =
            (addressValuePair.second.isNull() ||
             addressValuePair.second->getLocations().empty()) &&
                    loadedType->isPointerTy()
                ? getNewPointerValue(instr, stack, valueExpr, 0)
                : getNewVersionedValue(instr, stack, valueExpr);

        if (addressValuePair.second.isNull() ||
            loadedValue->getExpression() !=
                addressValuePair.second->getExpression()) {
          // We could not find the stored value, create a new one.
          updateStore(*li, addressValue, loadedValue);
          loadedValue->setLoadAddress(addressValue);
        } else {
          addDependencyViaLocation(addressValuePair.second, loadedValue, *li);
          loadedValue->setLoadAddress(addressValue);
          loadedValue->setStoreAddress(addressValuePair.first);
        }
      }
      break;
    }
    case llvm::Instruction::Store: {
      ref<VersionedValue> storedValue =
          getLatestValue(instr->getOperand(0), stack, valueExpr);
      ref<VersionedValue> addressValue =
          getLatestValue(instr->getOperand(1), stack, address);

      // If there was no dependency found, we should create
      // a new value
      if (storedValue.isNull())
        storedValue =
            getNewVersionedValue(instr->getOperand(0), stack, valueExpr);

      if (addressValue.isNull()) {
        // assert(!"null address");
        addressValue =
            getNewPointerValue(instr->getOperand(1), stack, address, 0);
      } else if (addressValue->getLocations().size() == 0) {
        if (instr->getOperand(1)->getType()->isPointerTy()) {
          addressValue->addLocation(
              MemoryLocation::create(instr->getOperand(1), stack, address, 0));
        } else {
          assert(!"address is not a pointer");
        }
      }

      std::set<ref<MemoryLocation> > locations = addressValue->getLocations();

      for (std::set<ref<MemoryLocation> >::iterator it = locations.begin(),
                                                    ie = locations.end();
           it != ie; ++it) {
        updateStore(*it, addressValue, storedValue);
      }
      break;
    }
    case llvm::Instruction::Trunc:
    case llvm::Instruction::ZExt:
    case llvm::Instruction::FPTrunc:
    case llvm::Instruction::FPExt:
    case llvm::Instruction::FPToUI:
    case llvm::Instruction::FPToSI:
    case llvm::Instruction::UIToFP:
    case llvm::Instruction::SIToFP:
    case llvm::Instruction::IntToPtr:
    case llvm::Instruction::PtrToInt:
    case llvm::Instruction::SExt:
    case llvm::Instruction::ExtractValue: {
      ref<Expr> result = args.at(0);
      ref<Expr> argExpr = args.at(1);

      ref<VersionedValue> val =
          getLatestValue(instr->getOperand(0), stack, argExpr);

      if (!val.isNull()) {
        if (llvm::isa<llvm::IntToPtrInst>(instr)) {
          if (val->getLocations().size() == 0) {
            // 0 signifies unknown allocation size
            addDependencyToNonPointer(
                val, getNewPointerValue(instr, stack, result, 0));
          } else {
            addDependencyIntToPtr(val,
                                  getNewVersionedValue(instr, stack, result));
          }
        } else {
          addDependency(val, getNewVersionedValue(instr, stack, result));
        }
      } else if (!llvm::isa<llvm::Constant>(instr->getOperand(0)))
          // Constants would kill dependencies, the remaining is for
          // cases that may actually require dependencies.
      {
        if (instr->getOperand(0)->getType()->isPointerTy()) {
          uint64_t size = targetData->getTypeStoreSize(
              instr->getOperand(0)->getType()->getPointerElementType());
          // Here we create normal non-pointer value for the
          // dependency target as it will be properly made a
          // pointer value by addDependency.
          addDependency(
              getNewPointerValue(instr->getOperand(0), stack, argExpr, size),
              getNewVersionedValue(instr, stack, result));
        } else if (llvm::isa<llvm::Argument>(instr->getOperand(0)) ||
                   llvm::isa<llvm::CallInst>(instr->getOperand(0)) ||
                   symbolicExecutionError) {
          if (llvm::isa<llvm::IntToPtrInst>(instr)) {
            // 0 signifies unknown allocation size
            addDependency(
                getNewVersionedValue(instr->getOperand(0), stack, argExpr),
                getNewPointerValue(instr, stack, result, 0));
          } else {
            addDependency(
                getNewVersionedValue(instr->getOperand(0), stack, argExpr),
                getNewVersionedValue(instr, stack, result));
          }
        } else {
          assert(!"operand not found");
        }
      }
      break;
    }
    default: { assert(!"unhandled binary instruction"); }
    }
    return;
  }
  case 3: {
    ref<Expr> result = args.at(0);
    ref<Expr> op1Expr = args.at(1);
    ref<Expr> op2Expr = args.at(2);

    switch (instr->getOpcode()) {
    case llvm::Instruction::Select: {
      ref<VersionedValue> op1 =
          getLatestValue(instr->getOperand(1), stack, op1Expr);
      ref<VersionedValue> op2 =
          getLatestValue(instr->getOperand(2), stack, op2Expr);
      ref<VersionedValue> newValue = getNewVersionedValue(instr, stack, result);

      if (result == op1Expr) {
        addDependency(op1, newValue);
      } else if (result == op2Expr) {
        addDependency(op2, newValue);
      } else {
        addDependency(op1, newValue);
        // We do not require that the locations set is empty
        addDependency(op2, newValue, false);
      }
      break;
    }

    case llvm::Instruction::Add:
    case llvm::Instruction::Sub:
    case llvm::Instruction::Mul:
    case llvm::Instruction::UDiv:
    case llvm::Instruction::SDiv:
    case llvm::Instruction::URem:
    case llvm::Instruction::SRem:
    case llvm::Instruction::And:
    case llvm::Instruction::Or:
    case llvm::Instruction::Xor:
    case llvm::Instruction::Shl:
    case llvm::Instruction::LShr:
    case llvm::Instruction::AShr:
    case llvm::Instruction::ICmp:
    case llvm::Instruction::FAdd:
    case llvm::Instruction::FSub:
    case llvm::Instruction::FMul:
    case llvm::Instruction::FDiv:
    case llvm::Instruction::FRem:
    case llvm::Instruction::FCmp:
    case llvm::Instruction::InsertValue: {
      ref<VersionedValue> op1 =
          getLatestValue(instr->getOperand(0), stack, op1Expr);
      ref<VersionedValue> op2 =
          getLatestValue(instr->getOperand(1), stack, op2Expr);
      ref<VersionedValue> newValue;

      if (op1.isNull() &&
          (instr->getParent()->getParent()->getName().equals("klee_range") &&
           instr->getOperand(0)->getName().equals("start"))) {
        op1 = getNewVersionedValue(instr->getOperand(0), stack, op1Expr);
      }
      if (op2.isNull() &&
          (instr->getParent()->getParent()->getName().equals("klee_range") &&
           instr->getOperand(1)->getName().equals("end"))) {
        op2 = getNewVersionedValue(instr->getOperand(1), stack, op2Expr);
      }

      if (!op1.isNull() || !op2.isNull()) {
        newValue = getNewVersionedValue(instr, stack, result);
        if (instr->getOpcode() == llvm::Instruction::ICmp ||
            instr->getOpcode() == llvm::Instruction::FCmp) {
          addDependencyToNonPointer(op1, newValue);
          addDependencyToNonPointer(op2, newValue);
        } else {
          addDependency(op1, newValue);
          // We do not require that the locations set is empty
          addDependency(op2, newValue, false);
        }
      }
      break;
    }
    case llvm::Instruction::GetElementPtr: {
      ref<Expr> resultAddress = args.at(0);
      ref<Expr> inputAddress = args.at(1);
      ref<Expr> inputOffset = args.at(2);

      ref<VersionedValue> addressValue =
          getLatestValue(instr->getOperand(0), stack, inputAddress);
      if (addressValue.isNull()) {
        // assert(!"null address");
        addressValue =
            getNewPointerValue(instr->getOperand(0), stack, inputAddress, 0);
      } else if (addressValue->getLocations().size() == 0) {
        // Note that the allocation has unknown size here (0).
        addressValue->addLocation(MemoryLocation::create(
            instr->getOperand(0), stack, inputAddress, 0));
      }

      addDependencyWithOffset(addressValue,
                              getNewVersionedValue(instr, stack, resultAddress),
                              inputOffset);
      break;
    }
    default:
      assert(!"unhandled ternary instruction");
    }
    return;
  }
  default:
    break;
  }
  assert(!"unhandled instruction arguments number");
}

void Dependency::executePHI(llvm::Instruction *instr,
                            unsigned int incomingBlock,
                            const std::vector<llvm::Instruction *> &stack,
                            ref<Expr> valueExpr, bool symbolicExecutionError) {
  llvm::PHINode *node = llvm::dyn_cast<llvm::PHINode>(instr);
  llvm::Value *llvmArgValue = node->getIncomingValue(incomingBlock);
  ref<VersionedValue> val = getLatestValue(llvmArgValue, stack, valueExpr);
  if (!val.isNull()) {
    addDependency(val, getNewVersionedValue(instr, stack, valueExpr));
  } else if (llvm::isa<llvm::Constant>(llvmArgValue) ||
             llvm::isa<llvm::Argument>(llvmArgValue) ||
             symbolicExecutionError) {
    getNewVersionedValue(instr, stack, valueExpr);
  } else {
    assert(!"operand not found");
  }
}

void Dependency::executeMemoryOperation(
    llvm::Instruction *instr, const std::vector<llvm::Instruction *> &stack,
    std::vector<ref<Expr> > &args, bool boundsCheck,
    bool symbolicExecutionError) {
  execute(instr, stack, args, symbolicExecutionError);
#ifdef ENABLE_Z3
  if (!NoBoundInterpolation && boundsCheck) {
    // The bounds check has been proven valid, we keep the dependency on the
    // address. Calling va_start within a variadic function also triggers memory
    // operation, but we ignored it here as this method is only called when load
    // / store instruction is processed.
    llvm::Value *addressOperand;
    ref<Expr> address(args.at(1));
    switch (instr->getOpcode()) {
    case llvm::Instruction::Load: {
      addressOperand = instr->getOperand(0);
      break;
    }
    case llvm::Instruction::Store: {
      addressOperand = instr->getOperand(1);
      break;
    }
    default: {
      assert(!"unknown memory operation");
      break;
    }
    }

    if (SpecialFunctionBoundInterpolation) {
      // Limit interpolation to only within function tracerx_check
      ref<VersionedValue> val(
          getLatestValueForMarking(addressOperand, address));
      if (llvm::isa<llvm::LoadInst>(instr) && !val->getLocations().empty()) {
        if (instr->getParent()->getParent()->getName().str() ==
            "tracerx_check") {
          std::set<ref<MemoryLocation> > locations(val->getLocations());
          for (std::set<ref<MemoryLocation> >::iterator it = locations.begin(),
                                                        ie = locations.end();
               it != ie; ++it) {
            if (llvm::ConstantExpr *ce =
                    llvm::dyn_cast<llvm::ConstantExpr>((*it)->getValue())) {
              if (llvm::isa<llvm::GetElementPtrInst>(ce->getAsInstruction())) {
                std::string reason = "";
                if (debugSubsumptionLevel >= 1) {
                  llvm::raw_string_ostream stream(reason);
                  stream << "pointer use [";
                  if (instr->getParent()->getParent()) {
                    stream << instr->getParent()->getParent()->getName().str()
                           << ": ";
                  }
                  if (llvm::MDNode *n = instr->getMetadata("dbg")) {
                    llvm::DILocation loc(n);
                    stream << "Line " << loc.getLineNumber();
                  }
                  stream << "]";
                  stream.flush();
                }
                if (ExactAddressInterpolant) {
                  markAllValues(addressOperand, address, reason);
                } else {
                  markAllPointerValues(addressOperand, address, reason);
                }
                break;
              }
            }
          }
        }
      }
    } else {
      std::string reason = "";
      if (debugSubsumptionLevel >= 1) {
        llvm::raw_string_ostream stream(reason);
        stream << "pointer use [";
        if (instr->getParent()->getParent()) {
          stream << instr->getParent()->getParent()->getName().str() << ": ";
        }
        if (llvm::MDNode *n = instr->getMetadata("dbg")) {
          llvm::DILocation loc(n);
          stream << "Line " << loc.getLineNumber();
        }
        stream << "]";
        stream.flush();
      }
      if (ExactAddressInterpolant) {
        markAllValues(addressOperand, address, reason);
      } else {
        markAllPointerValues(addressOperand, address, reason);
      }
    }
  }
#endif
}

void Dependency::bindCallArguments(llvm::Instruction *i,
                                   std::vector<llvm::Instruction *> &stack,
                                   std::vector<ref<Expr> > &arguments) {
  llvm::CallInst *site = llvm::dyn_cast<llvm::CallInst>(i);

  if (!site)
    return;

  llvm::Function *callee = site->getCalledFunction();

  // Sometimes the callee information is missing, in which case
  // the callee is not to be symbolically tracked.
  if (!callee)
    return;

  argumentValuesList = populateArgumentValuesList(site, stack, arguments);

  unsigned index = 0;
  stack.push_back(i);
  for (llvm::Function::ArgumentListType::iterator
           it = callee->getArgumentList().begin(),
           ie = callee->getArgumentList().end();
       it != ie; ++it) {
    if (!argumentValuesList.back().isNull()) {

      addDependency(argumentValuesList.back(),
                    getNewVersionedValue(
                        it, stack, argumentValuesList.back()->getExpression()));
    }
    argumentValuesList.pop_back();
    ++index;
  }
}

void Dependency::bindReturnValue(llvm::CallInst *site,
                                 std::vector<llvm::Instruction *> &stack,
                                 llvm::Instruction *i, ref<Expr> returnValue) {
  llvm::ReturnInst *retInst = llvm::dyn_cast<llvm::ReturnInst>(i);
  if (site && retInst &&
      retInst->getReturnValue() // For functions returning void
      ) {
    ref<VersionedValue> value =
        getLatestValue(retInst->getReturnValue(), stack, returnValue);
    if (!stack.empty()) {
      stack.pop_back();
    }
    if (!value.isNull())
      addDependency(value, getNewVersionedValue(site, stack, returnValue));
  }
}

void Dependency::markAllValues(ref<VersionedValue> value,
                               const std::string &reason) {
  markFlow(value, reason);
}

void Dependency::markAllValues(llvm::Value *val, ref<Expr> expr,
                               const std::string &reason) {
  ref<VersionedValue> value = getLatestValueForMarking(val, expr);

  if (value.isNull())
    return;

  markFlow(value, reason);
}

void Dependency::markAllPointerValues(llvm::Value *val, ref<Expr> address,
                                      std::set<ref<Expr> > &bounds,
                                      const std::string &reason) {
  ref<VersionedValue> value = getLatestValueForMarking(val, address);

  if (value.isNull())
    return;

  markPointerFlow(value, value, bounds, reason);
}

void Dependency::print(llvm::raw_ostream &stream) const {
  this->print(stream, 0);
}

void Dependency::print(llvm::raw_ostream &stream,
                       const unsigned paddingAmount) const {
  std::string tabs = makeTabs(paddingAmount);
  std::string tabsNext = appendTab(tabs);
  std::string tabsNextNext = appendTab(tabsNext);

  if (concretelyAddressedStore.empty()) {
    stream << tabs << "concrete store = []\n";
  } else {
    stream << tabs << "concrete store = [\n";
    for (std::map<ref<MemoryLocation>,
                  std::pair<ref<VersionedValue>,
                            ref<VersionedValue> > >::const_iterator
             is = concretelyAddressedStore.begin(),
             ie = concretelyAddressedStore.end(), it = is;
         it != ie; ++it) {
      if (it != is)
        stream << tabsNext << "------------------------------------------\n";
      stream << tabsNext << "address:\n";
      it->first->print(stream, tabsNextNext);
      stream << "\n";
      stream << tabsNext << "content:\n";
      it->second.second->print(stream, tabsNextNext);
      stream << "\n";
    }
    stream << tabs << "]\n";
  }

  if (symbolicallyAddressedStore.empty()) {
    stream << tabs << "symbolic store = []\n";
  } else {
    stream << tabs << "symbolic store = [\n";
    for (std::map<ref<MemoryLocation>,
                  std::pair<ref<VersionedValue>,
                            ref<VersionedValue> > >::const_iterator
             is = symbolicallyAddressedStore.begin(),
             ie = symbolicallyAddressedStore.end(), it = is;
         it != ie; ++it) {
      if (it != is)
        stream << tabsNext << "------------------------------------------\n";
      stream << tabsNext << "address:\n";
      it->first->print(stream, tabsNextNext);
      stream << "\n";
      stream << tabsNext << "content:\n";
      it->second.second->print(stream, tabsNextNext);
      stream << "\n";
    }
    stream << "]\n";
  }

  if (parent) {
    stream << tabs << "--------- Parent Dependencies ----------\n";
    parent->print(stream, paddingAmount);
  }
}

}
