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

void Dependency::getConcreteStore(
    const std::vector<llvm::Instruction *> &callHistory,
    const std::map<ref<TxStateAddress>,
                   std::pair<ref<VersionedValue>, ref<VersionedValue> > > &
        store,
    std::set<const Array *> &replacements, bool coreOnly,
    Dependency::ConcreteStore &concreteStore) const {

  for (std::map<ref<TxStateAddress>,
                std::pair<ref<VersionedValue>,
                          ref<VersionedValue> > >::const_iterator
           it = store.begin(),
           ie = store.end();
       it != ie; ++it) {
    if (!it->first->contextIsPrefixOf(callHistory))
      continue;
    if (it->second.second.isNull())
      continue;

    if (!coreOnly) {
      const llvm::Value *base = it->first->getContext()->getValue();
      concreteStore[base][it->first->getInterpolantStyleAddress()] =
          TxInterpolantValue::create(it->second.second);
    } else if (it->second.second->isCore()) {
      // An address is in the core if it stores a value that is in the core
      const llvm::Value *base = it->first->getContext()->getValue();
#ifdef ENABLE_Z3
      if (!NoExistential) {
        concreteStore[base][it->first->getInterpolantStyleAddress()] =
            TxInterpolantValue::create(it->second.second, replacements);
      } else
#endif
        concreteStore[base][it->first->getInterpolantStyleAddress()] =
            TxInterpolantValue::create(it->second.second);
    }
  }
}

void Dependency::getSymbolicStore(
    const std::vector<llvm::Instruction *> &callHistory,
    const std::map<ref<TxStateAddress>,
                   std::pair<ref<VersionedValue>, ref<VersionedValue> > > &
        store,
    std::set<const Array *> &replacements, bool coreOnly,
    Dependency::SymbolicStore &symbolicStore) const {
  for (std::map<ref<TxStateAddress>,
                std::pair<ref<VersionedValue>,
                          ref<VersionedValue> > >::const_iterator
           it = store.begin(),
           ie = store.end();
       it != ie; ++it) {
    if (!it->first->contextIsPrefixOf(callHistory))
      continue;

    if (it->second.second.isNull())
      continue;

    if (!coreOnly) {
      llvm::Value *base = it->first->getContext()->getValue();
      symbolicStore[base].push_back(Dependency::AddressValuePair(
          it->first->getInterpolantStyleAddress(),
          TxInterpolantValue::create(it->second.second)));
    } else if (it->second.second->isCore()) {
      // An address is in the core if it stores a value that is in the core
      llvm::Value *base = it->first->getContext()->getValue();
#ifdef ENABLE_Z3
      if (!NoExistential) {
        symbolicStore[base].push_back(Dependency::AddressValuePair(
            TxStateAddress::create(it->first, replacements)
                ->getInterpolantStyleAddress(),
            TxInterpolantValue::create(it->second.second, replacements)));
      } else
#endif
        symbolicStore[base].push_back(Dependency::AddressValuePair(
            it->first->getInterpolantStyleAddress(),
            TxInterpolantValue::create(it->second.second)));
    }
  }
}

bool Dependency::isMainArgument(const llvm::Value *loc) {
  const llvm::Argument *vArg = llvm::dyn_cast<llvm::Argument>(loc);

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

void Dependency::getStoredExpressions(
    const std::vector<llvm::Instruction *> &callHistory,
    std::set<const Array *> &replacements, bool coreOnly,
    Dependency::ConcreteStore &_concretelyAddressedStore,
    Dependency::SymbolicStore &_symbolicallyAddressedStore) {
  getConcreteStore(callHistory, concretelyAddressedStore, replacements,
                   coreOnly, _concretelyAddressedStore);
  getSymbolicStore(callHistory, symbolicallyAddressedStore, replacements,
                   coreOnly, _symbolicallyAddressedStore);
}

ref<VersionedValue>
Dependency::getLatestValue(llvm::Value *value,
                           const std::vector<llvm::Instruction *> &callHistory,
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

      return getNewPointerValue(value, callHistory, valueExpr, size);
    } else if (llvm::isa<llvm::IntToPtrInst>(asInstruction)) {
	// 0 signifies unknown size
      return getNewPointerValue(value, callHistory, valueExpr, 0);
    } else if (llvm::BitCastInst *bci =
                   llvm::dyn_cast<llvm::BitCastInst>(asInstruction)) {
      return getLatestValue(bci->getOperand(0), callHistory, valueExpr,
                            constraint);
    }
  }

  // A global value is a constant: Its value is constant throughout execution,
  // but
  // indeterministic. In case this was a non-global-value (normal) constant, we
  // immediately return with a versioned value, as dependencies are not
  // important. However, the dependencies of global values should be searched
  // for in the ancestors (later) as they need to be consistent in an execution.
  if (llvm::isa<llvm::Constant>(value) && !llvm::isa<llvm::GlobalValue>(value))
    return getNewVersionedValue(value, callHistory, valueExpr);

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
    ret = parent->getLatestValue(value, callHistory, valueExpr, constraint);

  if (ret.isNull()) {
    if (llvm::GlobalValue *gv = llvm::dyn_cast<llvm::GlobalValue>(value)) {
      // We could not find the global value: we register it anew.
      if (gv->getType()->isPointerTy()) {
        uint64_t size = 0;
        if (gv->getType()->getPointerElementType()->isSized())
          size = targetData->getTypeStoreSize(
              gv->getType()->getPointerElementType());
        ret = getNewPointerValue(value, callHistory, valueExpr, size);
      } else {
        ret = getNewVersionedValue(value, callHistory, valueExpr);
      }
    } else {
      llvm::StringRef name(value->getName());
      if (name.str() == "argc") {
        ret = getNewVersionedValue(value, callHistory, valueExpr);
      } else if (name.str() == "this" && value->getType()->isPointerTy()) {
        // For C++ "this" variable that is not found
        if (value->getType()->getPointerElementType()->isSized()) {
          uint64_t size = targetData->getTypeStoreSize(
              value->getType()->getPointerElementType());
          ret = getNewPointerValue(value, callHistory, valueExpr, size);
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

void Dependency::updateStore(ref<TxStateAddress> loc,
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
  ref<TxStateAddress> nullLocation;

  if (source.isNull() || target.isNull())
    return;

  std::set<ref<TxStateAddress> > locations = source->getLocations();
  ref<Expr> targetExpr(ZExtExpr::create(target->getExpression(),
                                        Expr::createPointer(0)->getWidth()));
  for (std::set<ref<TxStateAddress> >::iterator it = locations.begin(),
                                                ie = locations.end();
       it != ie; ++it) {
    ref<Expr> sourceBase((*it)->getBase());
    ref<Expr> offsetDelta(SubExpr::create(
        SubExpr::create(targetExpr, sourceBase), (*it)->getOffset()));
    target->addLocation(TxStateAddress::create(*it, targetExpr, offsetDelta));
  }
  target->addDependency(source, nullLocation);
}

void Dependency::addDependencyWithOffset(ref<VersionedValue> source,
                                         ref<VersionedValue> target,
                                         ref<Expr> offsetDelta) {
  ref<TxStateAddress> nullLocation;

  if (source.isNull() || target.isNull())
    return;

  std::set<ref<TxStateAddress> > locations = source->getLocations();
  ref<Expr> targetExpr(target->getExpression());

  ConstantExpr *ce = llvm::dyn_cast<ConstantExpr>(targetExpr);
  uint64_t a = ce ? ce->getZExtValue() : 0;

  ConstantExpr *de = llvm::dyn_cast<ConstantExpr>(offsetDelta);
  uint64_t d = de ? de->getZExtValue() : 0;

  uint64_t nLocations = locations.size();
  uint64_t i = 0;
  bool locationAdded = false;

  for (std::set<ref<TxStateAddress> >::iterator it = locations.begin(),
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
    target->addLocation(TxStateAddress::create(*it, targetExpr, offsetDelta));
    locationAdded = true;
  }
  target->addDependency(source, nullLocation);
}

void Dependency::addDependencyViaLocation(ref<VersionedValue> source,
                                          ref<VersionedValue> target,
                                          ref<TxStateAddress> via) {
  if (source.isNull() || target.isNull())
    return;

  std::set<ref<TxStateAddress> > locations = source->getLocations();
  for (std::set<ref<TxStateAddress> >::iterator it = locations.begin(),
                                                ie = locations.end();
       it != ie; ++it) {
    target->addLocation(*it);
  }
  target->addDependency(source, via);
}

void Dependency::addDependencyViaExternalFunction(
    const std::vector<llvm::Instruction *> &callHistory,
    ref<VersionedValue> source, ref<VersionedValue> target) {
  if (source.isNull() || target.isNull())
    return;

#ifdef ENABLE_Z3
  if (!NoBoundInterpolation) {
    std::set<ref<TxStateAddress> > locations = source->getLocations();
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
        TxStateAddress::create(target->getValue(), callHistory, address, size));
  }

  addDependencyToNonPointer(source, target);
}

void Dependency::addDependencyToNonPointer(ref<VersionedValue> source,
                                           ref<VersionedValue> target) {
  if (source.isNull() || target.isNull())
    return;

  ref<TxStateAddress> nullLocation;
  target->addDependency(source, nullLocation);
}

std::vector<ref<VersionedValue> >
Dependency::directFlowSources(ref<VersionedValue> target) const {
  std::vector<ref<VersionedValue> > ret;
  std::map<ref<VersionedValue>, ref<TxStateAddress> > sources =
      target->getSources();
  ref<VersionedValue> loadAddress = target->getLoadAddress(),
                      storeAddress = target->getStoreAddress();

  for (std::map<ref<VersionedValue>, ref<TxStateAddress> >::iterator it =
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
    std::set<ref<TxStateAddress> > locations = target->getLocations();
    for (std::set<ref<TxStateAddress> >::iterator it = locations.begin(),
                                                  ie = locations.end();
         it != ie; ++it) {
      (*it)->adjustOffsetBound(checkedAddress, bounds);
    }
  }
  target->setAsCore(reason);

  // Compute the direct pointer flow dependency
  std::map<ref<VersionedValue>, ref<TxStateAddress> > sources =
      target->getSources();

  for (std::map<ref<VersionedValue>, ref<TxStateAddress> >::iterator
           it = sources.begin(),
           ie = sources.end();
       it != ie; ++it) {
    markPointerFlow(it->first, checkedAddress, bounds, reason);
  }

  // We use normal marking with markFlow for load/store addresses
  markFlow(target->getLoadAddress(), reason);
  markFlow(target->getStoreAddress(), reason);
}

void Dependency::populateArgumentValuesList(
    llvm::CallInst *site, const std::vector<llvm::Instruction *> &callHistory,
    std::vector<ref<Expr> > &arguments,
    std::vector<ref<VersionedValue> > &argumentValuesList) {
  unsigned numArgs = site->getCalledFunction()->arg_size();
  for (unsigned i = numArgs; i > 0;) {
    llvm::Value *argOperand = site->getArgOperand(--i);
    ref<VersionedValue> latestValue =
        getLatestValue(argOperand, callHistory, arguments.at(i));

    if (!latestValue.isNull())
      argumentValuesList.push_back(latestValue);
    else {
      // This is for the case when latestValue was NULL, which means there is
      // no source dependency information for this node, e.g., a constant.
      argumentValuesList.push_back(
          VersionedValue::create(argOperand, callHistory, arguments[i]));
    }
  }
}

Dependency::Dependency(Dependency *parent, llvm::DataLayout *_targetData)
    : parent(parent), targetData(_targetData) {
  if (parent) {
    concretelyAddressedStore = parent->concretelyAddressedStore;
    symbolicallyAddressedStore = parent->symbolicallyAddressedStore;
    debugSubsumptionLevel = parent->debugSubsumptionLevel;
    debugStateLevel = parent->debugStateLevel;
  } else {
#ifdef ENABLE_Z3
    debugSubsumptionLevel = DebugSubsumption;
    debugStateLevel = DebugState;
#else
    debugSubsumptionLevel = 0;
    debugStateLevel = 0;
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
                         const std::vector<llvm::Instruction *> &callHistory,
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
        getNewPointerValue(instr, callHistory, args.at(0),
                           sizeExpr->getZExtValue());
      } else if ((calleeName.equals("getpagesize") && args.size() == 1) ||
                 (calleeName.equals("ioctl") && args.size() == 4) ||
                 (calleeName.equals("__ctype_b_loc") && args.size() == 1) ||
                 (calleeName.equals("__ctype_b_locargs") && args.size() == 1) ||
                 calleeName.equals("puts") || calleeName.equals("fflush") ||
                 calleeName.equals("strcmp") || calleeName.equals("strncmp") ||
                 (calleeName.equals("__errno_location") && args.size() == 1) ||
                 (calleeName.equals("geteuid") && args.size() == 1)) {
        getNewVersionedValue(instr, callHistory, args.at(0));
      } else if (calleeName.equals("_ZNSi5seekgElSt12_Ios_Seekdir") &&
                 args.size() == 4) {
        ref<VersionedValue> returnValue =
            getNewVersionedValue(instr, callHistory, args.at(0));
        for (unsigned i = 0; i < 3; ++i) {
          addDependencyViaExternalFunction(
              callHistory,
              getLatestValue(instr->getOperand(i), callHistory, args.at(i + 1)),
              returnValue);
        }
      } else if (calleeName.equals(
                     "_ZNSt13basic_fstreamIcSt11char_traitsIcEE7is_openEv") &&
                 args.size() == 2) {
        addDependencyViaExternalFunction(
            callHistory,
            getLatestValue(instr->getOperand(0), callHistory, args.at(1)),
            getNewVersionedValue(instr, callHistory, args.at(0)));
      } else if (calleeName.equals("_ZNSi5tellgEv") && args.size() == 2) {
        addDependencyViaExternalFunction(
            callHistory,
            getLatestValue(instr->getOperand(0), callHistory, args.at(1)),
            getNewVersionedValue(instr, callHistory, args.at(0)));
      } else if ((calleeName.equals("powl") && args.size() == 3) ||
                 (calleeName.equals("gettimeofday") && args.size() == 3)) {
        ref<VersionedValue> returnValue =
            getNewVersionedValue(instr, callHistory, args.at(0));
        for (unsigned i = 0; i < 2; ++i) {
          addDependencyViaExternalFunction(
              callHistory,
              getLatestValue(instr->getOperand(i), callHistory, args.at(i + 1)),
              returnValue);
        }
      } else if (calleeName.equals("malloc") && args.size() == 1) {
        // malloc is an location-type instruction. This is for the case when the
        // allocation size is unknown (0), so the
        // single argument here is the return address, for which KLEE provides
        // 0.
        getNewPointerValue(instr, callHistory, args.at(0), 0);
      } else if (calleeName.equals("malloc") && args.size() == 2) {
        // malloc is an location-type instruction. This is the case when it has
        // a determined size
        uint64_t size = 0;
        if (ConstantExpr *ce = llvm::dyn_cast<ConstantExpr>(args.at(1)))
          size = ce->getZExtValue();
        getNewPointerValue(instr, callHistory, args.at(0), size);
      } else if (calleeName.equals("realloc") && args.size() == 1) {
        // realloc is an location-type instruction: its single argument is the
        // return address.
        addDependency(
            getLatestValue(instr->getOperand(0), callHistory, args.at(0)),
            getNewVersionedValue(instr, callHistory, args.at(0)));
      } else if (calleeName.equals("calloc") && args.size() == 1) {
        // calloc is a location-type instruction: its single argument is the
        // return address. We assume its allocation size is unknown
        getNewPointerValue(instr, callHistory, args.at(0), 0);
      } else if (calleeName.equals("syscall") && args.size() >= 2) {
        ref<VersionedValue> returnValue =
            getNewVersionedValue(instr, callHistory, args.at(0));
        for (unsigned i = 0; i + 1 < args.size(); ++i) {
          addDependencyViaExternalFunction(
              callHistory,
              getLatestValue(instr->getOperand(i), callHistory, args.at(i + 1)),
              returnValue);
        }
      } else if (std::mismatch(getValuePrefix.begin(), getValuePrefix.end(),
                               calleeName.begin()).first ==
                     getValuePrefix.end() &&
                 args.size() == 2) {
        addDependencyViaExternalFunction(
            callHistory,
            getLatestValue(instr->getOperand(0), callHistory, args.at(1)),
            getNewVersionedValue(instr, callHistory, args.at(0)));
      } else if (calleeName.equals("getenv") && args.size() == 2) {
        // We assume getenv has unknown allocation size
        getNewPointerValue(instr, callHistory, args.at(0), 0);
      } else if (calleeName.equals("printf") && args.size() >= 2) {
        ref<VersionedValue> returnValue =
            getNewVersionedValue(instr, callHistory, args.at(0));
        addDependencyViaExternalFunction(
            callHistory,
            getLatestValue(instr->getOperand(0), callHistory, args.at(1)),
            returnValue);
        for (unsigned i = 2, argsNum = args.size(); i < argsNum; ++i) {
          addDependencyViaExternalFunction(
              callHistory,
              getLatestValue(instr->getOperand(i - 1), callHistory, args.at(i)),
              returnValue);
        }
      } else if (calleeName.equals("vprintf") && args.size() == 3) {
        ref<VersionedValue> returnValue =
            getNewVersionedValue(instr, callHistory, args.at(0));
        addDependencyViaExternalFunction(
            callHistory,
            getLatestValue(instr->getOperand(0), callHistory, args.at(1)),
            returnValue);
        addDependencyViaExternalFunction(
            callHistory,
            getLatestValue(instr->getOperand(1), callHistory, args.at(2)),
            returnValue);
      } else if (((calleeName.equals("fchmodat") && args.size() == 5)) ||
                 (calleeName.equals("fchownat") && args.size() == 6)) {
        ref<VersionedValue> returnValue =
            getNewVersionedValue(instr, callHistory, args.at(0));
        for (unsigned i = 0; i < 2; ++i) {
          addDependencyViaExternalFunction(
              callHistory,
              getLatestValue(instr->getOperand(i), callHistory, args.at(i + 1)),
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
          getNewVersionedValue(instr, callHistory, args.at(0));
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
          getLatestValue(instr->getOperand(0), callHistory, argExpr);

      if (!val.isNull()) {
        addDependency(val, getNewVersionedValue(instr, callHistory, argExpr));
      } else if (!llvm::isa<llvm::Constant>(instr->getOperand(0)))
          // Constants would kill dependencies, the remaining is for
          // cases that may actually require dependencies.
      {
        if (instr->getOperand(0)->getType()->isPointerTy()) {
          uint64_t size = targetData->getTypeStoreSize(
              instr->getOperand(0)->getType()->getPointerElementType());
          addDependency(getNewPointerValue(instr->getOperand(0), callHistory,
                                           argExpr, size),
                        getNewVersionedValue(instr, callHistory, argExpr));
        } else if (llvm::isa<llvm::Argument>(instr->getOperand(0)) ||
                   llvm::isa<llvm::CallInst>(instr->getOperand(0)) ||
                   symbolicExecutionError) {
          addDependency(
              getNewVersionedValue(instr->getOperand(0), callHistory, argExpr),
              getNewVersionedValue(instr, callHistory, argExpr));
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
      getNewPointerValue(instr, callHistory, valueExpr, size);
      break;
    }
    case llvm::Instruction::Load: {
      ref<VersionedValue> addressValue =
          getLatestValue(instr->getOperand(0), callHistory, address);
      llvm::Type *loadedType =
          instr->getOperand(0)->getType()->getPointerElementType();

      if (!addressValue.isNull()) {
        std::set<ref<TxStateAddress> > locations = addressValue->getLocations();
        if (locations.empty()) {
          // The size of the allocation is unknown here as the memory region
          // might have been allocated by the environment
          ref<TxStateAddress> loc = TxStateAddress::create(
              instr->getOperand(0), callHistory, address, 0);
          addressValue->addLocation(loc);

          // Build the loaded value
          ref<VersionedValue> loadedValue =
              loadedType->isPointerTy()
                  ? getNewPointerValue(instr, callHistory, valueExpr, 0)
                  : getNewVersionedValue(instr, callHistory, valueExpr);

          updateStore(loc, addressValue, loadedValue);
          break;
        } else if (locations.size() == 1) {
          ref<TxStateAddress> loc = *(locations.begin());

          // Check the possible mismatch between Tracer-X and KLEE loaded value
          std::map<ref<TxStateAddress>,
                   std::pair<ref<VersionedValue>,
                             ref<VersionedValue> > >::iterator storeIt =
              concretelyAddressedStore.find(loc);
          std::pair<ref<VersionedValue>, ref<VersionedValue> > target;

          if (storeIt == concretelyAddressedStore.end()) {
              storeIt = symbolicallyAddressedStore.find(loc);
              if (storeIt != symbolicallyAddressedStore.end()) {
                target = storeIt->second;
              }
          } else {
              target = storeIt->second;
          }

          if (!target.second.isNull() &&
              valueExpr != target.second->getExpression()) {
            // Print a warning when the expressions mismatch, unless when the
            // expression comes from klee_make_symbolic in a loop, as the
            // expected expression recorded in Tracer-X shadow memory may be
            // outdated, and the expression that comes from KLEE is the updated
            // one from klee_make_symbolic.
            llvm::CallInst *ci =
                llvm::dyn_cast<llvm::CallInst>(target.second->getValue());
            if (ci) {
              // Here we determine if this was a call to klee_make_symbolic from
              // the LLVM source of the call instruction instead of
              // Function::getName(). This is to circumvent segmentation fault
              // issue when the KLEE runtime library is not linked.
              std::string instrSrc;
              llvm::raw_string_ostream s1(instrSrc);
              ci->print(s1);
              s1.flush();
              if (instrSrc.find("klee_make_symbolic") == std::string::npos) {
                std::string msg;
                llvm::raw_string_ostream s2(msg);
                s2 << "Loaded value ";
                target.second->getExpression()->print(s2);
                s2 << " should be ";
                valueExpr->print(s2);
                s2.flush();
                klee_warning("%s", msg.c_str());
              }
            }
          }

          if (isMainArgument(loc->getContext()->getValue())) {
            // The load corresponding to a load of the main function's argument
            // that was never allocated within this program.

            // Build the loaded value
            ref<VersionedValue> loadedValue =
                loadedType->isPointerTy()
                    ? getNewPointerValue(instr, callHistory, valueExpr, 0)
                    : getNewVersionedValue(instr, callHistory, valueExpr);

            updateStore(loc, addressValue, loadedValue);
            break;
          }
        }
      } else {
        // assert(!"loaded allocation size must not be zero");
        addressValue =
            getNewPointerValue(instr->getOperand(0), callHistory, address, 0);

        if (llvm::isa<llvm::GlobalVariable>(instr->getOperand(0))) {
          // The value not found was a global variable, record it here.
          std::set<ref<TxStateAddress> > locations =
              addressValue->getLocations();

          // Build the loaded value
          ref<VersionedValue> loadedValue =
              loadedType->isPointerTy()
                  ? getNewPointerValue(instr, callHistory, valueExpr, 0)
                  : getNewVersionedValue(instr, callHistory, valueExpr);

          updateStore(*(locations.begin()), addressValue, loadedValue);
          break;
        }
      }

      std::set<ref<TxStateAddress> > locations = addressValue->getLocations();

      for (std::set<ref<TxStateAddress> >::iterator li = locations.begin(),
                                                    le = locations.end();
           li != le; ++li) {
        std::pair<ref<VersionedValue>, ref<VersionedValue> > addressValuePair;

        std::map<ref<TxStateAddress>,
                 std::pair<ref<VersionedValue>,
                           ref<VersionedValue> > >::iterator storeIter;
        if ((*li)->hasConstantAddress()) {
          storeIter = concretelyAddressedStore.find(*li);
          if (storeIter != concretelyAddressedStore.end()) {
            addressValuePair = storeIter->second;
          }
        } else {
          storeIter = symbolicallyAddressedStore.find(*li);
          if (storeIter != symbolicallyAddressedStore.end()) {
            // FIXME: Here we assume that the expressions have to exactly be the
            // same expression object. More properly, this should instead add an
            // ite constraint onto the path condition.
            addressValuePair = storeIter->second;
          }
        }

        // Build the loaded value
        ref<VersionedValue> loadedValue =
            (addressValuePair.second.isNull() ||
             addressValuePair.second->getLocations().empty()) &&
                    loadedType->isPointerTy()
                ? getNewPointerValue(instr, callHistory, valueExpr, 0)
                : getNewVersionedValue(instr, callHistory, valueExpr);

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
          getLatestValue(instr->getOperand(0), callHistory, valueExpr);
      ref<VersionedValue> addressValue =
          getLatestValue(instr->getOperand(1), callHistory, address);

      // If there was no dependency found, we should create
      // a new value
      if (storedValue.isNull())
        storedValue =
            getNewVersionedValue(instr->getOperand(0), callHistory, valueExpr);

      if (addressValue.isNull()) {
        // assert(!"null address");
        addressValue =
            getNewPointerValue(instr->getOperand(1), callHistory, address, 0);
      } else if (addressValue->getLocations().size() == 0) {
        if (instr->getOperand(1)->getType()->isPointerTy()) {
          addressValue->addLocation(TxStateAddress::create(
              instr->getOperand(1), callHistory, address, 0));
        } else {
          assert(!"address is not a pointer");
        }
      }

      std::set<ref<TxStateAddress> > locations = addressValue->getLocations();

      for (std::set<ref<TxStateAddress> >::iterator it = locations.begin(),
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
          getLatestValue(instr->getOperand(0), callHistory, argExpr);

      if (!val.isNull()) {
        if (llvm::isa<llvm::IntToPtrInst>(instr)) {
          if (val->getLocations().size() == 0) {
            // 0 signifies unknown allocation size
            addDependencyToNonPointer(
                val, getNewPointerValue(instr, callHistory, result, 0));
          } else {
            addDependencyIntToPtr(
                val, getNewVersionedValue(instr, callHistory, result));
          }
        } else {
          addDependency(val, getNewVersionedValue(instr, callHistory, result));
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
          addDependency(getNewPointerValue(instr->getOperand(0), callHistory,
                                           argExpr, size),
                        getNewVersionedValue(instr, callHistory, result));
        } else if (llvm::isa<llvm::Argument>(instr->getOperand(0)) ||
                   llvm::isa<llvm::CallInst>(instr->getOperand(0)) ||
                   symbolicExecutionError) {
          if (llvm::isa<llvm::IntToPtrInst>(instr)) {
            // 0 signifies unknown allocation size
            addDependency(getNewVersionedValue(instr->getOperand(0),
                                               callHistory, argExpr),
                          getNewPointerValue(instr, callHistory, result, 0));
          } else {
            addDependency(getNewVersionedValue(instr->getOperand(0),
                                               callHistory, argExpr),
                          getNewVersionedValue(instr, callHistory, result));
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
          getLatestValue(instr->getOperand(1), callHistory, op1Expr);
      ref<VersionedValue> op2 =
          getLatestValue(instr->getOperand(2), callHistory, op2Expr);
      ref<VersionedValue> newValue =
          getNewVersionedValue(instr, callHistory, result);

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
          getLatestValue(instr->getOperand(0), callHistory, op1Expr);
      ref<VersionedValue> op2 =
          getLatestValue(instr->getOperand(1), callHistory, op2Expr);
      ref<VersionedValue> newValue;

      if (op1.isNull() &&
          (instr->getParent()->getParent()->getName().equals("klee_range") &&
           instr->getOperand(0)->getName().equals("start"))) {
        op1 = getNewVersionedValue(instr->getOperand(0), callHistory, op1Expr);
      }
      if (op2.isNull() &&
          (instr->getParent()->getParent()->getName().equals("klee_range") &&
           instr->getOperand(1)->getName().equals("end"))) {
        op2 = getNewVersionedValue(instr->getOperand(1), callHistory, op2Expr);
      }

      if (!op1.isNull() || !op2.isNull()) {
        newValue = getNewVersionedValue(instr, callHistory, result);
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
          getLatestValue(instr->getOperand(0), callHistory, inputAddress);
      if (addressValue.isNull()) {
        // assert(!"null address");
        addressValue = getNewPointerValue(instr->getOperand(0), callHistory,
                                          inputAddress, 0);
      } else if (addressValue->getLocations().size() == 0) {
        // Note that the allocation has unknown size here (0).
        addressValue->addLocation(TxStateAddress::create(
            instr->getOperand(0), callHistory, inputAddress, 0));
      }

      addDependencyWithOffset(
          addressValue, getNewVersionedValue(instr, callHistory, resultAddress),
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

void Dependency::executeMakeSymbolic(
    llvm::Instruction *instr,
    const std::vector<llvm::Instruction *> &callHistory, ref<Expr> address,
    const Array *array) {
  llvm::Value *pointer = instr->getOperand(0);

  ref<VersionedValue> storedValue = getNewVersionedValue(
      instr, callHistory, ConstantExpr::create(0, Expr::Bool));
  ref<VersionedValue> addressValue =
      getLatestValue(pointer, callHistory, address);

  if (addressValue.isNull()) {
    // assert(!"null address");
    addressValue = getNewPointerValue(pointer, callHistory, address, 0);
  } else if (addressValue->getLocations().size() == 0) {
    if (pointer->getType()->isPointerTy()) {
      addressValue->addLocation(
          TxStateAddress::create(pointer, callHistory, address, 0));
    } else {
      assert(!"address is not a pointer");
    }
  }

  std::set<ref<TxStateAddress> > locations = addressValue->getLocations();

  for (std::set<ref<TxStateAddress> >::iterator it = locations.begin(),
                                                ie = locations.end();
       it != ie; ++it) {
    updateStore(*it, addressValue, storedValue);
  }
}

void Dependency::executePHI(llvm::Instruction *instr,
                            unsigned int incomingBlock,
                            const std::vector<llvm::Instruction *> &callHistory,
                            ref<Expr> valueExpr, bool symbolicExecutionError) {
  llvm::PHINode *node = llvm::dyn_cast<llvm::PHINode>(instr);
  llvm::Value *llvmArgValue = node->getIncomingValue(incomingBlock);
  ref<VersionedValue> val =
      getLatestValue(llvmArgValue, callHistory, valueExpr);
  if (!val.isNull()) {
    addDependency(val, getNewVersionedValue(instr, callHistory, valueExpr));
  } else if (llvm::isa<llvm::Constant>(llvmArgValue) ||
             llvm::isa<llvm::Argument>(llvmArgValue) ||
             symbolicExecutionError) {
    getNewVersionedValue(instr, callHistory, valueExpr);
  } else {
    assert(!"operand not found");
  }
}

void Dependency::executeMemoryOperation(
    llvm::Instruction *instr,
    const std::vector<llvm::Instruction *> &callHistory,
    std::vector<ref<Expr> > &args, bool boundsCheck,
    bool symbolicExecutionError) {
  execute(instr, callHistory, args, symbolicExecutionError);
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
          std::set<ref<TxStateAddress> > locations(val->getLocations());
          for (std::set<ref<TxStateAddress> >::iterator it = locations.begin(),
                                                        ie = locations.end();
               it != ie; ++it) {
            if (llvm::ConstantExpr *ce = llvm::dyn_cast<llvm::ConstantExpr>(
                    (*it)->getContext()->getValue())) {
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

void
Dependency::bindCallArguments(llvm::Instruction *i,
                              std::vector<llvm::Instruction *> &callHistory,
                              std::vector<ref<Expr> > &arguments) {
  llvm::CallInst *site = llvm::dyn_cast<llvm::CallInst>(i);

  if (!site)
    return;

  llvm::Function *callee = site->getCalledFunction();

  // Sometimes the callee information is missing, in which case
  // the callee is not to be symbolically tracked.
  if (!callee)
    return;

  argumentValuesList.clear();
  populateArgumentValuesList(site, callHistory, arguments, argumentValuesList);

  unsigned index = 0;
  callHistory.push_back(i);
  for (llvm::Function::ArgumentListType::iterator
           it = callee->getArgumentList().begin(),
           ie = callee->getArgumentList().end();
       it != ie; ++it) {
    if (!argumentValuesList.back().isNull()) {

      addDependency(
          argumentValuesList.back(),
          getNewVersionedValue(it, callHistory,
                               argumentValuesList.back()->getExpression()));
    }
    argumentValuesList.pop_back();
    ++index;
  }
}

void Dependency::bindReturnValue(llvm::CallInst *site,
                                 std::vector<llvm::Instruction *> &callHistory,
                                 llvm::Instruction *i, ref<Expr> returnValue) {
  llvm::ReturnInst *retInst = llvm::dyn_cast<llvm::ReturnInst>(i);
  if (site && retInst &&
      retInst->getReturnValue() // For functions returning void
      ) {
    ref<VersionedValue> value =
        getLatestValue(retInst->getReturnValue(), callHistory, returnValue);
    if (!callHistory.empty()) {
      callHistory.pop_back();
    }
    if (!value.isNull())
      addDependency(value,
                    getNewVersionedValue(site, callHistory, returnValue));
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
    for (std::map<ref<TxStateAddress>,
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
    for (std::map<ref<TxStateAddress>,
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
