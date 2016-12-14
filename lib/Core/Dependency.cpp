//===-- Dependency.cpp - Field-insensitive dependency -----------*- C++ -*-===//
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

#include "klee/CommandLine.h"
#include "klee/Internal/Support/ErrorHandling.h"

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

std::map<const Array *, const Array *> ShadowArray::shadowArray;

UpdateNode *
ShadowArray::getShadowUpdate(const UpdateNode *source,
                             std::set<const Array *> &replacements) {
  if (!source)
    return 0;

  return new UpdateNode(getShadowUpdate(source->next, replacements),
                        getShadowExpression(source->index, replacements),
                        getShadowExpression(source->value, replacements));
}

ref<Expr> ShadowArray::createBinaryOfSameKind(ref<Expr> originalExpr,
                                              ref<Expr> newLhs,
                                              ref<Expr> newRhs) {
  std::vector<Expr::CreateArg> exprs;
  Expr::CreateArg arg1(newLhs);
  Expr::CreateArg arg2(newRhs);
  exprs.push_back(arg1);
  exprs.push_back(arg2);
  return Expr::createFromKind(originalExpr->getKind(), exprs);
}

void ShadowArray::addShadowArrayMap(const Array *source, const Array *target) {
  shadowArray[source] = target;
}

ref<Expr>
ShadowArray::getShadowExpression(ref<Expr> expr,
                                 std::set<const Array *> &replacements) {
  ref<Expr> ret;

  switch (expr->getKind()) {
  case Expr::Read: {
    ReadExpr *readExpr = llvm::dyn_cast<ReadExpr>(expr);
    const Array *replacementArray = shadowArray[readExpr->updates.root];

    if (std::find(replacements.begin(), replacements.end(), replacementArray) ==
        replacements.end()) {
      replacements.insert(replacementArray);
    }

    UpdateList newUpdates(
        replacementArray,
        getShadowUpdate(readExpr->updates.head, replacements));
    ret = ReadExpr::create(newUpdates,
                           getShadowExpression(readExpr->index, replacements));
    break;
  }
  case Expr::Constant: {
    ret = expr;
    break;
  }
  case Expr::Select: {
    ret =
        SelectExpr::create(getShadowExpression(expr->getKid(0), replacements),
                           getShadowExpression(expr->getKid(1), replacements),
                           getShadowExpression(expr->getKid(2), replacements));
    break;
  }
  case Expr::Extract: {
    ExtractExpr *extractExpr = llvm::dyn_cast<ExtractExpr>(expr);
    ret =
        ExtractExpr::create(getShadowExpression(expr->getKid(0), replacements),
                            extractExpr->offset, extractExpr->width);
    break;
  }
  case Expr::ZExt: {
    CastExpr *castExpr = llvm::dyn_cast<CastExpr>(expr);
    ret = ZExtExpr::create(getShadowExpression(expr->getKid(0), replacements),
                           castExpr->getWidth());
    break;
  }
  case Expr::SExt: {
    CastExpr *castExpr = llvm::dyn_cast<CastExpr>(expr);
    ret = SExtExpr::create(getShadowExpression(expr->getKid(0), replacements),
                           castExpr->getWidth());
    break;
  }
  case Expr::Concat:
  case Expr::Add:
  case Expr::Sub:
  case Expr::Mul:
  case Expr::UDiv:
  case Expr::SDiv:
  case Expr::URem:
  case Expr::SRem:
  case Expr::Not:
  case Expr::And:
  case Expr::Or:
  case Expr::Xor:
  case Expr::Shl:
  case Expr::LShr:
  case Expr::AShr:
  case Expr::Eq:
  case Expr::Ne:
  case Expr::Ult:
  case Expr::Ule:
  case Expr::Ugt:
  case Expr::Uge:
  case Expr::Slt:
  case Expr::Sle:
  case Expr::Sgt:
  case Expr::Sge: {
    ret = createBinaryOfSameKind(
        expr, getShadowExpression(expr->getKid(0), replacements),
        getShadowExpression(expr->getKid(1), replacements));
    break;
  }
  case Expr::NotOptimized: {
    ret = NotOptimizedExpr::create(getShadowExpression(expr->getKid(0), replacements));
    break;
  }
  default:
    assert(!"unhandled Expr type");
  }

  return ret;
}

/**/

void MemoryLocation::adjustOffsetBound(ref<VersionedValue> checkedAddress) {
  std::set<ref<MemoryLocation> > locations = checkedAddress->getLocations();

  for (std::set<ref<MemoryLocation> >::iterator it = locations.begin(),
                                                ie = locations.end();
       it != ie; ++it) {
    ref<Expr> checkedOffset = (*it)->getOffset();
    if (ConstantExpr *c = llvm::dyn_cast<ConstantExpr>(checkedOffset)) {
      if (ConstantExpr *o = llvm::dyn_cast<ConstantExpr>(offset)) {
        uint64_t newBound = size - (c->getZExtValue() - o->getZExtValue());
        if (concreteOffsetBound > newBound) {
          concreteOffsetBound = newBound;
        }
        continue;
      }
    }

    symbolicOffsetBounds.insert(SubExpr::create(
        Expr::createPointer(size), SubExpr::create(checkedOffset, offset)));
  }
}

void MemoryLocation::print(llvm::raw_ostream &stream) const {
  stream << "A";
  if (!llvm::isa<ConstantExpr>(this->address))
    stream << "(symbolic)";
  stream << "[";
  value->print(stream);
  stream << ":";
  address->print(stream);
  stream << ":";
  base->print(stream);
  stream << ":";
  offset->print(stream);
  stream << "]#" << reinterpret_cast<uintptr_t>(this);
}

/**/

void VersionedValue::print(llvm::raw_ostream &stream) const {
  stream << "V";
  if (core)
    stream << "(I)";
  stream << "[";
  value->print(stream);
  stream << ":";
  valueExpr->print(stream);
  stream << "]#" << reinterpret_cast<uintptr_t>(this) << " <- {";

  for (std::map<ref<VersionedValue>, ref<MemoryLocation> >::const_iterator
           is2 = sources.begin(),
           it2 = is2, ie2 = sources.end();
       it2 != ie2; ++it2) {
    if (it2 != is2)
      stream << ", ";
    stream << "  [";
    (*it2->first).print(stream);
    if (!it2->second.isNull()) {
      stream << " via ";
      (*it2->second).print(stream);
    }
    stream << "]";
  }
  stream << "}";
}

/**/

void StoredValue::init(ref<VersionedValue> vvalue,
                       std::set<const Array *> &replacements, bool shadowing) {
  std::set<ref<MemoryLocation> > locations = vvalue->getLocations();

  refCount = 0;
  id = reinterpret_cast<uintptr_t>(this);
  expr = shadowing ? ShadowArray::getShadowExpression(vvalue->getExpression(),
                                                      replacements)
                   : vvalue->getExpression();
  value = vvalue->getValue();

  if (!locations.empty()) {
    // Here we compute memory bounds for checking pointer values. The memory
    // bound is the size of the allocation minus the offset; this is the weakest
    // precondition (interpolant) of memory bound checks done by KLEE.
    for (std::set<ref<MemoryLocation> >::iterator it = locations.begin(),
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
        for (std::set<ref<Expr> >::iterator it1 = res.begin(), ie1 = res.end();
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

ref<Expr> StoredValue::getBoundsCheck(ref<StoredValue> stateValue) const {
  ref<Expr> res;

  // For a state to be subsumed, the subsuming state weaker, which in this case
  // means that it should specify less allocations, so all allocations in the
  // subsuming (this), should be specified by the subsumed (the stateValue
  // argument).
  for (std::map<llvm::Value *, std::set<ref<Expr> > >::const_iterator
           it = allocationBounds.begin(),
           ie = allocationBounds.end();
       it != ie; ++it) {
    std::set<ref<Expr> > tabledBounds = it->second;
    std::set<ref<Expr> > stateOffsets =
        stateValue->allocationOffsets.at(it->first);

    assert(!tabledBounds.empty() && "tabled bounds empty");

    if (stateOffsets.empty())
      return ConstantExpr::create(0, Expr::Bool);

    for (std::set<ref<Expr> >::const_iterator it1 = stateOffsets.begin(),
                                              ie1 = stateOffsets.end();
         it1 != ie1; ++it1) {
      for (std::set<ref<Expr> >::const_iterator it2 = tabledBounds.begin(),
                                                ie2 = tabledBounds.end();
           it2 != ie2; ++it2) {
        if (ConstantExpr *tabledBound = llvm::dyn_cast<ConstantExpr>(*it2)) {
          uint64_t tabledBoundInt = tabledBound->getZExtValue();
          if (ConstantExpr *stateOffset = llvm::dyn_cast<ConstantExpr>(*it1)) {
            if (tabledBoundInt > 0 &&
                stateOffset->getZExtValue() >= tabledBoundInt) {
              return ConstantExpr::create(0, Expr::Bool);
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
          }
          continue;
        }
        // Create constraints for symbolic bounds
        if (res.isNull()) {
          res = UltExpr::create(*it1, *it2);
        } else {
          res = AndExpr::create(UltExpr::create(*it1, *it2), res);
        }
      }
    }
  }

  // Bounds check successful if no constraints added
  if (res.isNull())
    return ConstantExpr::create(1, Expr::Bool);
  return res;
}

void StoredValue::print(llvm::raw_ostream &stream) const {
  if (!allocationBounds.empty()) {
    stream << "BOUNDS:\n";
    for (std::map<llvm::Value *, std::set<ref<Expr> > >::const_iterator
             it = allocationBounds.begin(),
             ie = allocationBounds.end();
         it != ie; ++it) {
      std::set<ref<Expr> > boundsSet = it->second;
      stream << "[";
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
      stream << "}]\n";
    }

    if (!allocationOffsets.empty()) {
      stream << "OFFSETS:\n";
      for (std::map<llvm::Value *, std::set<ref<Expr> > >::const_iterator
               it = allocationOffsets.begin(),
               ie = allocationOffsets.end();
           it != ie; ++it) {
        std::set<ref<Expr> > boundsSet = it->second;
        stream << "[";
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
        stream << "}]\n";
      }
    }
    return;
  }
  expr->print(stream);
}

/**/

ref<VersionedValue>
Dependency::registerNewVersionedValue(llvm::Value *value,
                                      ref<VersionedValue> vvalue) {
  valuesMap[value].push_back(vvalue);
  return vvalue;
}

std::pair<Dependency::ConcreteStore, Dependency::SymbolicStore>
Dependency::getStoredExpressions(std::set<const Array *> &replacements,
                                 bool coreOnly) {
  ConcreteStore concreteStore;
  SymbolicStore symbolicStore;

  for (std::map<ref<MemoryLocation>, ref<VersionedValue> >::iterator
           it = concretelyAddressedStore.begin(),
           ie = concretelyAddressedStore.end();
       it != ie; ++it) {
    if (it->second.isNull())
      continue;

    if (!coreOnly) {
      llvm::Value *site = it->first->getValue();
      uint64_t uintAddress = it->first->getUIntAddress();
      ref<Expr> address = it->first->getAddress();
      concreteStore[site][uintAddress] = StoredValue::create(it->second);
    } else if (it->second->isCore()) {
      // An address is in the core if it stores a value that is in the core
      llvm::Value *base = it->first->getValue();
      uint64_t uintAddress = it->first->getUIntAddress();
#ifdef ENABLE_Z3
      if (!NoExistential) {
        concreteStore[base][uintAddress] =
            StoredValue::create(it->second, replacements);
      } else
#endif
        concreteStore[base][uintAddress] = StoredValue::create(it->second);
    }
  }

  for (std::map<ref<MemoryLocation>, ref<VersionedValue> >::iterator
           it = symbolicallyAddressedStore.begin(),
           ie = symbolicallyAddressedStore.end();
       it != ie; ++it) {
    if (it->second.isNull())
      continue;

    ref<Expr> address = it->first->getAddress();
    if (!coreOnly) {
      llvm::Value *base = it->first->getValue();
      symbolicStore[base].push_back(
          AddressValuePair(address, StoredValue::create(it->second)));
    } else if (it->second->isCore()) {
      // An address is in the core if it stores a value that is in the core
      llvm::Value *base = it->first->getValue();
#ifdef ENABLE_Z3
      if (!NoExistential) {
        symbolicStore[base].push_back(AddressValuePair(
            ShadowArray::getShadowExpression(address, replacements),
            StoredValue::create(it->second, replacements)));
      } else
#endif
        symbolicStore[base].push_back(
            AddressValuePair(address, StoredValue::create(it->second)));
      }
  }

  return std::pair<ConcreteStore, SymbolicStore>(concreteStore, symbolicStore);
}

ref<VersionedValue> Dependency::getLatestValue(llvm::Value *value,
                                               ref<Expr> valueExpr) {
  assert(value && "value cannot be null");
  if (llvm::isa<llvm::ConstantExpr>(value)) {
    llvm::Instruction *asInstruction =
        llvm::dyn_cast<llvm::ConstantExpr>(value)->getAsInstruction();
    if (llvm::GetElementPtrInst *gi =
            llvm::dyn_cast<llvm::GetElementPtrInst>(asInstruction)) {
      uint64_t size =
          targetData->getTypeStoreSize(gi->getPointerOperand()->getType());
      return getNewPointerValue(value, valueExpr, size);
    }
  }

  // A global value is a constant: Its value is constant throughout execution,
  // but
  // indeterministic. In case this was a non-global-value (normal) constant, we
  // immediately return with a versioned value, as dependencies are not
  // important. However, the dependencies of global values should be searched
  // for in the ancestors (later) as they need to be consistent in an execution.
  if (llvm::isa<llvm::Constant>(value) && !llvm::isa<llvm::GlobalValue>(value))
    return getNewVersionedValue(value, valueExpr);

  if (valuesMap.find(value) != valuesMap.end()) {
    return valuesMap[value].back();
  }

  ref<VersionedValue> ret = 0;
  if (parent)
    ret = parent->getLatestValue(value, valueExpr);

  if (ret.isNull()) {
    if (llvm::GlobalValue *gv = llvm::dyn_cast<llvm::GlobalValue>(value)) {
      // We could not find the global value: we register it anew.
      if (gv->getType()->isPointerTy()) {
        uint64_t size = 0;
        if (gv->getType()->getPointerElementType()->isSized())
          size = targetData->getTypeStoreSize(
              gv->getType()->getPointerElementType());
        ret = getNewPointerValue(value, valueExpr, size);
      } else {
        ret = getNewVersionedValue(value, valueExpr);
      }
    }
  }
  return ret;
}

ref<VersionedValue>
Dependency::getLatestValueNoConstantCheck(llvm::Value *value) {
  assert(value && "value cannot be null");

  if (valuesMap.find(value) != valuesMap.end()) {
    return valuesMap[value].back();
  }

  if (parent)
    return parent->getLatestValueNoConstantCheck(value);

  return 0;
}

ref<VersionedValue> Dependency::getLatestValueForMarking(llvm::Value *val) {
  ref<VersionedValue> value = getLatestValueNoConstantCheck(val);

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
                             ref<VersionedValue> value) {
  if (loc->hasConstantAddress())
    concretelyAddressedStore[loc] = value;
  else
    symbolicallyAddressedStore[loc] = value;
}

void Dependency::addDependency(ref<VersionedValue> source,
                               ref<VersionedValue> target) {
  ref<MemoryLocation> nullLocation;

  if (source.isNull() || target.isNull())
    return;

  std::set<ref<MemoryLocation> > locations = source->getLocations();
  for (std::set<ref<MemoryLocation> >::iterator it = locations.begin(),
                                                ie = locations.end();
       it != ie; ++it) {
    target->addLocation(*it);
  }
  target->addDependency(source, nullLocation);
}

void Dependency::addDependencyWithOffset(ref<VersionedValue> source,
                                         ref<VersionedValue> target,
                                         ref<Expr> offset) {
  ref<MemoryLocation> nullLocation;

  if (source.isNull() || target.isNull())
    return;

  std::set<ref<MemoryLocation> > locations = source->getLocations();
  for (std::set<ref<MemoryLocation> >::iterator it = locations.begin(),
                                                ie = locations.end();
       it != ie; ++it) {
    ref<Expr> expr(target->getExpression());
    target->addLocation(MemoryLocation::create(*it, expr, offset));
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

std::vector<ref<VersionedValue> >
Dependency::directFlowSources(ref<VersionedValue> target) const {
  std::vector<ref<VersionedValue> > ret;
  std::map<ref<VersionedValue>, ref<MemoryLocation> > sources =
      target->getSources();

    for (std::map<ref<VersionedValue>, ref<MemoryLocation> >::iterator it =
             sources.begin();
         it != sources.end(); ++it) {
      if (!it->first->isCore())
        ret.push_back(it->first);
    }
  return ret;
}

void Dependency::markFlow(ref<VersionedValue> target) const {
  target->setAsCore();
  std::vector<ref<VersionedValue> > stepSources = directFlowSources(target);
  for (std::vector<ref<VersionedValue> >::iterator it = stepSources.begin(),
                                                   ie = stepSources.end();
       it != ie; ++it) {
    markFlow(*it);
  }
}

void Dependency::markPointerFlow(ref<VersionedValue> target,
                                 ref<VersionedValue> checkedAddress) const {
  std::set<ref<MemoryLocation> > locations = target->getLocations();
  for (std::set<ref<MemoryLocation> >::iterator it = locations.begin(),
                                                ie = locations.end();
       it != ie; ++it) {
    std::set<ref<MemoryLocation> > l = checkedAddress->getLocations();
    (*it)->adjustOffsetBound(checkedAddress);
  }
  target->setAsCore();

  // Compute the direct pointer flow dependency
  std::map<ref<VersionedValue>, ref<MemoryLocation> > sources =
      target->getSources();

  for (std::map<ref<VersionedValue>, ref<MemoryLocation> >::iterator
           it = sources.begin(),
           ie = sources.end();
       it != ie; ++it) {
    markPointerFlow(it->first, checkedAddress);
  }
}

std::vector<ref<VersionedValue> >
Dependency::populateArgumentValuesList(llvm::CallInst *site,
                                       std::vector<ref<Expr> > &arguments) {
  unsigned numArgs = site->getCalledFunction()->arg_size();
  std::vector<ref<VersionedValue> > argumentValuesList;
  for (unsigned i = numArgs; i > 0;) {
    llvm::Value *argOperand = site->getArgOperand(--i);
    ref<VersionedValue> latestValue =
        getLatestValue(argOperand, arguments.at(i));

    if (!latestValue.isNull())
      argumentValuesList.push_back(latestValue);
    else {
      // This is for the case when latestValue was NULL, which means there is
      // no source dependency information for this node, e.g., a constant.
      argumentValuesList.push_back(
          VersionedValue::create(argOperand, arguments[i]));
    }
  }
  return argumentValuesList;
}

Dependency::Dependency(Dependency *parent, llvm::DataLayout *_targetData)
    : parent(parent), targetData(_targetData) {
  if (parent) {
    concretelyAddressedStore = parent->concretelyAddressedStore;
    symbolicallyAddressedStore = parent->symbolicallyAddressedStore;
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

      if ((calleeName.equals("getpagesize") && args.size() == 1) ||
          (calleeName.equals("ioctl") && args.size() == 4) ||
          (calleeName.equals("__ctype_b_loc") && args.size() == 1) ||
          (calleeName.equals("__ctype_b_locargs") && args.size() == 1) ||
          calleeName.equals("puts") || calleeName.equals("fflush") ||
          calleeName.equals("_Znwm") || calleeName.equals("_Znam") ||
          calleeName.equals("strcmp") || calleeName.equals("strncmp") ||
          (calleeName.equals("__errno_location") && args.size() == 1) ||
          (calleeName.equals("geteuid") && args.size() == 1)) {
        getNewVersionedValue(instr, args.at(0));
      } else if (calleeName.equals("_ZNSi5seekgElSt12_Ios_Seekdir") &&
                 args.size() == 4) {
        ref<VersionedValue> returnValue =
            getNewVersionedValue(instr, args.at(0));
        for (unsigned i = 0; i < 3; ++i) {
          addDependency(getLatestValue(instr->getOperand(i), args.at(i + 1)),
                        returnValue);
        }
      } else if (calleeName.equals(
                     "_ZNSt13basic_fstreamIcSt11char_traitsIcEE7is_openEv") &&
                 args.size() == 2) {
        addDependency(getLatestValue(instr->getOperand(0), args.at(1)),
                      getNewVersionedValue(instr, args.at(0)));
      } else if (calleeName.equals("_ZNSi5tellgEv") && args.size() == 2) {
        addDependency(getLatestValue(instr->getOperand(0), args.at(1)),
                      getNewVersionedValue(instr, args.at(0)));
      } else if ((calleeName.equals("powl") && args.size() == 3) ||
                 (calleeName.equals("gettimeofday") && args.size() == 3)) {
        ref<VersionedValue> returnValue =
            getNewVersionedValue(instr, args.at(0));
        for (unsigned i = 0; i < 2; ++i) {
          addDependency(getLatestValue(instr->getOperand(i), args.at(i + 1)),
                        returnValue);
        }
      } else if (calleeName.equals("malloc") && args.size() == 1) {
        // malloc is an location-type instruction. This is for the case when the
        // allocation size is unknown (0), so the
        // single argument here is the return address, for which KLEE provides
        // 0.
        getNewPointerValue(instr, args.at(0), 0);
      } else if (calleeName.equals("malloc") && args.size() == 2) {
        // malloc is an location-type instruction. This is the case when it has
        // a determined size
        uint64_t size = 0;
        if (ConstantExpr *ce = llvm::dyn_cast<ConstantExpr>(args.at(1)))
          size = ce->getZExtValue();
        getNewPointerValue(instr, args.at(0), size);
      } else if (calleeName.equals("realloc") && args.size() == 1) {
        // realloc is an location-type instruction: its single argument is the
        // return address.
        addDependency(getLatestValue(instr->getOperand(0), args.at(0)),
                      getNewVersionedValue(instr, args.at(0)));
      } else if (calleeName.equals("calloc") && args.size() == 1) {
        // calloc is a location-type instruction: its single argument is the
        // return address.
        assert(!"calloc size must not be zero");
        getNewPointerValue(instr, args.at(0), 0);
      } else if (calleeName.equals("syscall") && args.size() >= 2) {
        ref<VersionedValue> returnValue =
            getNewVersionedValue(instr, args.at(0));
        for (unsigned i = 0; i + 1 < args.size(); ++i) {
          addDependency(getLatestValue(instr->getOperand(i), args.at(i + 1)),
                        returnValue);
        }
      } else if (std::mismatch(getValuePrefix.begin(), getValuePrefix.end(),
                               calleeName.begin()).first ==
                     getValuePrefix.end() &&
                 args.size() == 2) {
        addDependency(getLatestValue(instr->getOperand(0), args.at(1)),
                      getNewVersionedValue(instr, args.at(0)));
      } else if (calleeName.equals("getenv") && args.size() == 2) {
        assert(!"getenv size must not be zero");
        getNewPointerValue(instr, args.at(0), 0);
      } else if (calleeName.equals("printf") && args.size() >= 2) {
        ref<VersionedValue> returnValue =
            getNewVersionedValue(instr, args.at(0));
        addDependency(getLatestValue(instr->getOperand(0), args.at(1)),
                      returnValue);
        for (unsigned i = 2, argsNum = args.size(); i < argsNum; ++i) {
          addDependency(getLatestValue(instr->getOperand(i - 1), args.at(i)),
                        returnValue);
        }
      } else if (calleeName.equals("vprintf") && args.size() == 3) {
        ref<VersionedValue> returnValue =
            getNewVersionedValue(instr, args.at(0));
        addDependency(getLatestValue(instr->getOperand(0), args.at(1)),
                      returnValue);
        addDependency(getLatestValue(instr->getOperand(1), args.at(2)),
                      returnValue);
      } else if (((calleeName.equals("fchmodat") && args.size() == 5)) ||
                 (calleeName.equals("fchownat") && args.size() == 6)) {
        ref<VersionedValue> returnValue =
            getNewVersionedValue(instr, args.at(0));
        for (unsigned i = 0; i < 2; ++i) {
          addDependency(getLatestValue(instr->getOperand(i), args.at(i + 1)),
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
          getNewVersionedValue(instr, args.at(0));
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
        markAllValues(binst->getCondition());
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
    case llvm::Instruction::Trunc:
    case llvm::Instruction::ZExt:
    case llvm::Instruction::SExt:
    case llvm::Instruction::IntToPtr:
    case llvm::Instruction::PtrToInt:
    case llvm::Instruction::BitCast:
    case llvm::Instruction::FPTrunc:
    case llvm::Instruction::FPExt:
    case llvm::Instruction::FPToUI:
    case llvm::Instruction::FPToSI:
    case llvm::Instruction::UIToFP:
    case llvm::Instruction::SIToFP:
    case llvm::Instruction::ExtractValue: {
      ref<VersionedValue> val = getLatestValue(instr->getOperand(0), argExpr);

      if (!val.isNull()) {
        addDependency(val, getNewVersionedValue(instr, argExpr));
      } else if (!llvm::isa<llvm::Constant>(instr->getOperand(0)))
          // Constants would kill dependencies, the remaining is for
          // cases that may actually require dependencies.
      {
        if (instr->getOperand(0)->getType()->isPointerTy()) {
          uint64_t size = targetData->getTypeStoreSize(
              instr->getOperand(0)->getType()->getPointerElementType());
          addDependency(getNewPointerValue(instr->getOperand(0), argExpr, size),
                        getNewVersionedValue(instr, argExpr));
        } else if (llvm::isa<llvm::Argument>(instr->getOperand(0)) ||
                   llvm::isa<llvm::CallInst>(instr->getOperand(0)) ||
                   symbolicExecutionError) {
          addDependency(getNewVersionedValue(instr->getOperand(0), argExpr),
                        getNewVersionedValue(instr, argExpr));
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
      getNewPointerValue(instr, valueExpr, size);
      break;
    }
    case llvm::Instruction::Load: {
      ref<VersionedValue> addressValue =
          getLatestValue(instr->getOperand(0), address);
      ref<VersionedValue> loadedValue = getNewVersionedValue(instr, valueExpr);

      if (!addressValue.isNull()) {
        std::set<ref<MemoryLocation> > locations = addressValue->getLocations();
        if (locations.empty()) {
          // The size of the allocation is unknown here as the memory region
          // might have been allocated by the environment
          ref<MemoryLocation> loc =
              MemoryLocation::create(instr->getOperand(0), address, 0);
          addressValue->addLocation(loc);
          updateStore(loc, loadedValue);
          break;
        } else if (locations.size() == 1) {
          ref<MemoryLocation> loc = *(locations.begin());
          if (Util::isMainArgument(loc->getValue())) {
            // The load corresponding to a load of the main function's argument
            // that was never allocated within this program.
            updateStore(loc, loadedValue);
            break;
          }
        }
      } else {
        assert(!"loaded allocation size must not be zero");
        addressValue = getNewPointerValue(instr->getOperand(0), address, 0);
        if (llvm::isa<llvm::GlobalVariable>(instr->getOperand(0))) {
          // The value not found was a global variable, record it here.
          std::set<ref<MemoryLocation> > locations =
              addressValue->getLocations();
          updateStore(*(locations.begin()), loadedValue);
          break;
        }
      }

      std::set<ref<MemoryLocation> > locations = addressValue->getLocations();

      for (std::set<ref<MemoryLocation> >::iterator li = locations.begin(),
                                                    le = locations.end();
           li != le; ++li) {
        ref<VersionedValue> storedValue;
        if ((*li)->hasConstantAddress()) {
          if (concretelyAddressedStore.count(*li) > 0) {
            storedValue = concretelyAddressedStore[*li];
          }
        } else if (symbolicallyAddressedStore.count(*li) > 0) {
          // FIXME: Here we assume that the expressions have to exactly be the
          // same expression object. More properly, this should instead add an
          // ite constraint onto the path condition.
          storedValue = symbolicallyAddressedStore[*li];
        }

        if (storedValue.isNull())
          // We could not find the stored value, create a new one.
          updateStore(*li, loadedValue);
        else
          addDependencyViaLocation(storedValue, loadedValue, *li);
      }

      break;
    }
    case llvm::Instruction::Store: {
      ref<VersionedValue> storedValue =
          getLatestValue(instr->getOperand(0), valueExpr);
      ref<VersionedValue> addressValue =
          getLatestValue(instr->getOperand(1), address);

      // If there was no dependency found, we should create
      // a new value
      if (storedValue.isNull())
        storedValue = getNewVersionedValue(instr->getOperand(0), valueExpr);

      if (addressValue.isNull()) {
        assert(!"stored allocation size must not be zero");
        addressValue = getNewPointerValue(instr->getOperand(1), address, 0);
      } else if (addressValue->getLocations().size() == 0) {
        assert(!"stored allocation size must not be zero");
        addressValue->addLocation(
            MemoryLocation::create(instr->getOperand(1), address, 0));
      }

      std::set<ref<MemoryLocation> > locations = addressValue->getLocations();

      for (std::set<ref<MemoryLocation> >::iterator it = locations.begin(),
                                                    ie = locations.end();
           it != ie; ++it) {
        updateStore(*it, storedValue);
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
      ref<VersionedValue> op1 = getLatestValue(instr->getOperand(1), op1Expr);
      ref<VersionedValue> op2 = getLatestValue(instr->getOperand(2), op2Expr);
      ref<VersionedValue> newValue = getNewVersionedValue(instr, result);
      addDependency(op1, newValue);
      addDependency(op2, newValue);
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
      ref<VersionedValue> op1 = getLatestValue(instr->getOperand(0), op1Expr);
      ref<VersionedValue> op2 = getLatestValue(instr->getOperand(1), op2Expr);
      ref<VersionedValue> newValue;

      if (op1.isNull() &&
          (instr->getParent()->getParent()->getName().equals("klee_range") &&
           instr->getOperand(0)->getName().equals("start"))) {
        op1 = getNewVersionedValue(instr->getOperand(0), op1Expr);
      }
      if (op2.isNull() &&
          (instr->getParent()->getParent()->getName().equals("klee_range") &&
           instr->getOperand(1)->getName().equals("end"))) {
        op2 = getNewVersionedValue(instr->getOperand(1), op2Expr);
      }

      if (!op1.isNull() || !op2.isNull()) {
        newValue = getNewVersionedValue(instr, result);
        addDependency(op1, newValue);
        addDependency(op2, newValue);
      }
      break;
    }
    case llvm::Instruction::GetElementPtr: {
      ref<Expr> resultAddress = args.at(0);
      ref<Expr> inputAddress = args.at(1);
      ref<Expr> inputOffset = args.at(2);

      ref<VersionedValue> addressValue =
          getLatestValue(instr->getOperand(0), inputAddress);
      if (addressValue.isNull()) {
        assert(!"input allocation size should not be zero");
        addressValue =
            getNewPointerValue(instr->getOperand(0), inputAddress, 0);
      } else if (addressValue->getLocations().size() == 0) {
        // Note that the allocation has unknown size here (0).
        addressValue->addLocation(
            MemoryLocation::create(instr->getOperand(0), inputAddress, 0));
      }

      addDependencyWithOffset(addressValue,
                              getNewVersionedValue(instr, resultAddress),
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
                            unsigned int incomingBlock, ref<Expr> valueExpr,
                            bool symbolicExecutionError) {
  llvm::PHINode *node = llvm::dyn_cast<llvm::PHINode>(instr);
  llvm::Value *llvmArgValue = node->getIncomingValue(incomingBlock);
  ref<VersionedValue> val = getLatestValue(llvmArgValue, valueExpr);
  if (!val.isNull()) {
    addDependency(val, getNewVersionedValue(instr, valueExpr));
  } else if (llvm::isa<llvm::Constant>(llvmArgValue) ||
             llvm::isa<llvm::Argument>(llvmArgValue) ||
             symbolicExecutionError) {
    getNewVersionedValue(instr, valueExpr);
  } else {
    assert(!"operand not found");
  }
}

void Dependency::executeMemoryOperation(llvm::Instruction *instr,
                                        std::vector<ref<Expr> > &args,
                                        bool boundsCheck,
                                        bool symbolicExecutionError) {
  execute(instr, args, symbolicExecutionError);
#ifdef ENABLE_Z3
  if (!NoBoundInterpolation && boundsCheck) {
    // The bounds check has been proven valid, we keep the dependency on the
    // address. Calling va_start within a variadic function also triggers memory
    // operation, but we ignored it here as this method is only called when load
    // / store instruction is processed.
    llvm::Value *addressOperand;
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
    markAllPointerValues(addressOperand);
  }
#endif
}

void Dependency::bindCallArguments(llvm::Instruction *i,
                                   std::vector<ref<Expr> > &arguments) {
  llvm::CallInst *site = llvm::dyn_cast<llvm::CallInst>(i);

  if (!site)
    return;

  llvm::Function *callee = site->getCalledFunction();

  // Sometimes the callee information is missing, in which case
  // the callee is not to be symbolically tracked.
  if (!callee)
    return;

  argumentValuesList = populateArgumentValuesList(site, arguments);
  unsigned index = 0;
  for (llvm::Function::ArgumentListType::iterator
           it = callee->getArgumentList().begin(),
           ie = callee->getArgumentList().end();
       it != ie; ++it) {
    if (!argumentValuesList.back().isNull()) {
      addDependency(
          argumentValuesList.back(),
          getNewVersionedValue(it, argumentValuesList.back()->getExpression()));
    }
    argumentValuesList.pop_back();
    ++index;
  }
}

void Dependency::bindReturnValue(llvm::CallInst *site, llvm::Instruction *i,
                                 ref<Expr> returnValue) {
  llvm::ReturnInst *retInst = llvm::dyn_cast<llvm::ReturnInst>(i);
  if (site && retInst &&
      retInst->getReturnValue() // For functions returning void
      ) {
    ref<VersionedValue> value =
        getLatestValue(retInst->getReturnValue(), returnValue);
    if (!value.isNull())
      addDependency(value, getNewVersionedValue(site, returnValue));
  }
}

void Dependency::markAllValues(ref<VersionedValue> value) { markFlow(value); }

void Dependency::markAllValues(llvm::Value *val) {
  ref<VersionedValue> value = getLatestValueForMarking(val);

  if (value.isNull())
    return;

  markFlow(value);
}

void Dependency::markAllPointerValues(llvm::Value *val) {
  ref<VersionedValue> value = getLatestValueForMarking(val);

  if (value.isNull())
    return;

  markPointerFlow(value, value);
}

void Dependency::print(llvm::raw_ostream &stream) const {
  this->print(stream, 0);
}

void Dependency::print(llvm::raw_ostream &stream,
                       const unsigned paddingAmount) const {
  std::string tabs = makeTabs(paddingAmount);

  stream << tabs << "CONCRETE STORE:\n";
  for (std::map<ref<MemoryLocation>, ref<VersionedValue> >::const_iterator
           it = concretelyAddressedStore.begin(),
           ie = concretelyAddressedStore.end();
       it != ie; ++it) {
    stream << tabs << "  [";
    (*it->first).print(stream);
    stream << ",";
    (*it->second).print(stream);
    stream << "]\n";
  }
  stream << tabs << "SYMBOLIC STORE:\n";
  for (std::map<ref<MemoryLocation>, ref<VersionedValue> >::const_iterator
           it = symbolicallyAddressedStore.begin(),
           ie = symbolicallyAddressedStore.end();
       it != ie; ++it) {
    stream << tabs << "  [";
    (*it->first).print(stream);
    stream << ",";
    (*it->second).print(stream);
    stream << "]\n";
  }

  if (parent) {
    stream << tabs << "--------- Parent Dependencies ----------\n";
    parent->print(stream, paddingAmount);
  }
}

/**/

bool Dependency::Util::isMainArgument(llvm::Value *loc) {
  llvm::Argument *vArg = llvm::dyn_cast<llvm::Argument>(loc);

  // FIXME: We need a more precise way to detect main argument
  if (vArg && vArg->getParent() &&
      (vArg->getParent()->getName().equals("main") ||
       vArg->getParent()->getName().equals("__user_main"))) {
    return true;
  }
  return false;
}

/**/

std::string makeTabs(const unsigned paddingAmount) {
  std::string tabsString;
  for (unsigned i = 0; i < paddingAmount; ++i) {
    tabsString += appendTab(tabsString);
  }
  return tabsString;
}

std::string appendTab(const std::string &prefix) { return prefix + "        "; }
}
