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
/// compute the allocations upon which the unsatisfiability core depends,
/// which is used in computing the interpolant.
///
//===----------------------------------------------------------------------===//

#include "Dependency.h"

#include "klee/CommandLine.h"
#include "klee/Internal/Support/ErrorHandling.h"

#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 3)
#include <llvm/IR/Constants.h>
#include <llvm/IR/Intrinsics.h>
#else
#include <llvm/Constants.h>
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
    ReadExpr *readExpr = static_cast<ReadExpr *>(expr.get());
    const Array *replacementArray = shadowArray[readExpr->updates.root];

    if (std::find(replacements.begin(), replacements.end(), replacementArray) ==
        replacements.end()) {
      replacements.insert(replacementArray);
    }

    UpdateList newUpdates(
        replacementArray,
        getShadowUpdate(readExpr->updates.head, replacements));
    ret = ReadExpr::alloc(newUpdates,
                          getShadowExpression(readExpr->index, replacements));
    break;
  }
  case Expr::Constant: {
    ret = expr;
    break;
  }
  case Expr::Select: {
    ret = SelectExpr::alloc(getShadowExpression(expr->getKid(0), replacements),
                            getShadowExpression(expr->getKid(1), replacements),
                            getShadowExpression(expr->getKid(2), replacements));
    break;
  }
  case Expr::Extract: {
    ExtractExpr *extractExpr = static_cast<ExtractExpr *>(expr.get());
    ret = ExtractExpr::alloc(getShadowExpression(expr->getKid(0), replacements),
                             extractExpr->offset, extractExpr->width);
    break;
  }
  case Expr::ZExt: {
    CastExpr *castExpr = static_cast<CastExpr *>(expr.get());
    ret = ZExtExpr::alloc(getShadowExpression(expr->getKid(0), replacements),
                          castExpr->getWidth());
    break;
  }
  case Expr::SExt: {
    CastExpr *castExpr = static_cast<CastExpr *>(expr.get());
    ret = SExtExpr::alloc(getShadowExpression(expr->getKid(0), replacements),
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

void Allocation::print(llvm::raw_ostream &stream) const {
  // Do nothing
}

/**/

void VersionedAllocation::print(llvm::raw_ostream &stream) const {
  stream << "A";
  if (!llvm::isa<ConstantExpr>(this->address.get()))
    stream << "(symbolic)";
  if (core)
    stream << "(I)";
  stream << "[";
  site->print(stream);
  stream << ":";
  address->print(stream);
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
  stream << "]#" << reinterpret_cast<uintptr_t>(this);
  ;
}

/**/

bool AllocationGraph::isVisited(Allocation *alloc) {
  for (std::vector<AllocationNode *>::iterator it = allNodes.begin(),
                                               itEnd = allNodes.end();
       it != itEnd; ++it) {
    if ((*it)->getAllocation() == alloc) {
      return true;
    }
  }
  return false;
}

void AllocationGraph::addNewSink(Allocation *candidateSink) {
  if (isVisited(candidateSink))
    return;

  AllocationNode *newNode = new AllocationNode(candidateSink, 0);
  allNodes.push_back(newNode);
  sinks.push_back(newNode);
}

void AllocationGraph::addNewEdge(Allocation *source, Allocation *target) {
  AllocationNode *sourceNode = 0;
  AllocationNode *targetNode = 0;

  for (std::vector<AllocationNode *>::iterator it = allNodes.begin(),
                                               itEnd = allNodes.end();
       it != itEnd; ++it) {
    if (!targetNode && (*it)->getAllocation() == target) {
      targetNode = (*it);
      if (sourceNode)
        break;
    }

    if (!sourceNode && (*it)->getAllocation() == source) {
      sourceNode = (*it);
      if (targetNode)
        break;
    }
  }

  bool newNode = false; // indicates whether a new node is created

  uint64_t targetNodeLevel = (targetNode ? targetNode->getLevel() : 0);

  if (!sourceNode) {
    sourceNode = new AllocationNode(source, targetNodeLevel + 1);
    allNodes.push_back(sourceNode);
    newNode = true; // An edge actually added, return true
  } else {
    std::vector<AllocationNode *>::iterator pos =
        std::find(sinks.begin(), sinks.end(), sourceNode);
    if (pos == sinks.end()) {
      // Add new node if it's not in the sink
      sourceNode = new AllocationNode(source, targetNodeLevel + 1);
      allNodes.push_back(sourceNode);
      newNode = true;
    }
  }

  if (!targetNode) {
    targetNode = new AllocationNode(target, targetNodeLevel);
    allNodes.push_back(targetNode);
    sinks.push_back(targetNode);
    newNode = true; // An edge actually added, return true
  }

  // The purpose of the second condition is to prevent cycles
  // in the graph.
  if (newNode || !(targetNode->getLevel() < sourceNode->getLevel())) {
    targetNode->addParent(sourceNode);
  }
}

void AllocationGraph::consumeSinkNode(Allocation *allocation) {
  std::vector<AllocationNode *>::iterator pos = sinks.end();
  for (std::vector<AllocationNode *>::iterator it = sinks.begin(),
                                               itEnd = sinks.end();
       it != itEnd; ++it) {
    if ((*it)->getAllocation() == allocation) {
      pos = it;
      break;
    }
  }

  if (pos == sinks.end())
    return;

  std::vector<AllocationNode *> parents = (*pos)->getParents();
  sinks.erase(pos);

  for (std::vector<AllocationNode *>::iterator it = parents.begin(),
                                               itEnd = parents.end();
       it != itEnd; ++it) {
    if (std::find(sinks.begin(), sinks.end(), (*it)) == sinks.end())
      sinks.push_back(*it);
  }
}

std::set<Allocation *> AllocationGraph::getSinkAllocations() const {
  std::set<Allocation *> sinkAllocations;

  for (std::vector<AllocationNode *>::const_iterator it = sinks.begin(),
                                                     itEnd = sinks.end();
       it != itEnd; ++it) {
    sinkAllocations.insert((*it)->getAllocation());
  }

  return sinkAllocations;
}

std::set<Allocation *> AllocationGraph::getSinksWithAllocations(
    std::vector<Allocation *> allocationsList) const {
  std::set<Allocation *> sinkAllocations;

  for (std::vector<AllocationNode *>::const_iterator it = sinks.begin(),
                                                     itEnd = sinks.end();
       it != itEnd; ++it) {
    if (std::find(allocationsList.begin(), allocationsList.end(),
                  (*it)->getAllocation()) != allocationsList.end())
      sinkAllocations.insert((*it)->getAllocation());
  }

  return sinkAllocations;
}

void AllocationGraph::consumeSinksWithAllocations(
    std::vector<Allocation *> allocationsList) {
  std::set<Allocation *> sinkAllocs(getSinksWithAllocations(allocationsList));

  if (sinkAllocs.empty())
    return;

  for (std::set<Allocation *>::iterator it = sinkAllocs.begin(),
                                        itEnd = sinkAllocs.end();
       it != itEnd; ++it) {
    consumeSinkNode((*it));
  }

  // Recurse until fixpoint
  consumeSinksWithAllocations(allocationsList);
}

void AllocationGraph::print(llvm::raw_ostream &stream) const {
  std::vector<AllocationNode *> printed;
  print(stream, sinks, printed, 0);
}

void AllocationGraph::print(llvm::raw_ostream &stream,
                            std::vector<AllocationNode *> nodes,
                            std::vector<AllocationNode *> &printed,
                            const unsigned tabNum) const {
  if (nodes.size() == 0)
    return;

  std::string tabs = makeTabs(tabNum);

  for (std::vector<AllocationNode *>::iterator it = nodes.begin(),
                                               itEnd = nodes.end();
       it != itEnd; ++it) {
    Allocation *alloc = (*it)->getAllocation();
    stream << tabs;
    alloc->print(stream);
    if (std::find(printed.begin(), printed.end(), (*it)) != printed.end()) {
      stream << " (printed)\n";
    } else if ((*it)->getParents().size()) {
      stream << " depends on\n";
      printed.push_back((*it));
      print(stream, (*it)->getParents(), printed, tabNum + 1);
    } else {
      stream << "\n";
    }
  }
}

/**/

VersionedValue *Dependency::getNewVersionedValue(llvm::Value *value,
                                                 ref<Expr> valueExpr) {
  VersionedValue *ret = new VersionedValue(value, valueExpr);
  if (valuesMap.find(value) != valuesMap.end()) {
    valuesMap[value].push_back(ret);
  } else {
    std::vector<VersionedValue *> newValueList;
    newValueList.push_back(ret);
    valuesMap[value] = newValueList;
  }
  return ret;
}

Allocation *Dependency::getInitialAllocation(llvm::Value *allocation,
                                             ref<Expr> &address) {
  Allocation *ret = new VersionedAllocation(allocation, address);
  versionedAllocationsList.push_back(ret);
  return ret;
}

Allocation *Dependency::getNewAllocationVersion(llvm::Value *allocation,
                                                ref<Expr> &address) {
  Allocation *ret = getLatestAllocation(allocation, address);
  if (ret)
    return ret;

  return getInitialAllocation(allocation, address);
}

std::vector<Allocation *>
Dependency::getAllVersionedAllocations(bool coreOnly) const {
  std::vector<Allocation *> allAlloc;

  if (coreOnly)
    std::copy(coreAllocations.begin(), coreAllocations.end(),
              std::back_inserter(allAlloc));
  else
    allAlloc = versionedAllocationsList;

  if (parentDependency) {
    std::vector<Allocation *> parentVersionedAllocations =
        parentDependency->getAllVersionedAllocations(coreOnly);
    allAlloc.insert(allAlloc.begin(), parentVersionedAllocations.begin(),
                    parentVersionedAllocations.end());
  }
  return allAlloc;
}

std::pair<Dependency::ConcreteStore, Dependency::SymbolicStore>
Dependency::getStoredExpressions(std::set<const Array *> &replacements,
                                 bool coreOnly) const {
  std::vector<Allocation *> allAlloc = getAllVersionedAllocations(coreOnly);
  ConcreteStore concreteStore;
  SymbolicStore symbolicStore;

  for (std::vector<Allocation *>::iterator allocIter = allAlloc.begin(),
                                           allocIterEnd = allAlloc.end();
       allocIter != allocIterEnd; ++allocIter) {
    std::vector<VersionedValue *> stored = stores(*allocIter);

    // We should only get the latest value and no other
    assert(stored.size() <= 1);

    if (stored.size()) {
      VersionedValue *v = stored.at(0);

      if ((*allocIter)->hasConstantAddress()) {
        if (!coreOnly) {
          ref<Expr> expr = v->getExpression();
          llvm::Value *llvmAlloc = (*allocIter)->getSite();
          uint64_t uintAddress = (*allocIter)->getUIntAddress();
          ref<Expr> address = (*allocIter)->getAddress();
          concreteStore[llvmAlloc][uintAddress] =
              AddressValuePair(address, expr);
        } else if (v->isCore()) {
          ref<Expr> expr = v->getExpression();
          llvm::Value *base = (*allocIter)->getSite();
          uint64_t uintAddress = (*allocIter)->getUIntAddress();
          ref<Expr> address = (*allocIter)->getAddress();
#ifdef ENABLE_Z3
          if (!NoExistential) {
            concreteStore[base][uintAddress] = AddressValuePair(
                ShadowArray::getShadowExpression(address, replacements),
                ShadowArray::getShadowExpression(expr, replacements));
	  } else
#endif
            concreteStore[base][uintAddress] = AddressValuePair(address, expr);
        }
      } else {
        ref<Expr> address = (*allocIter)->getAddress();
        if (!coreOnly) {
          ref<Expr> expr = v->getExpression();
          llvm::Value *base = (*allocIter)->getSite();
          symbolicStore[base].push_back(AddressValuePair(address, expr));
        } else if (v->isCore()) {
          ref<Expr> expr = v->getExpression();
          llvm::Value *base = v->getValue();
#ifdef ENABLE_Z3
          if (!NoExistential) {
            symbolicStore[base].push_back(AddressValuePair(
                ShadowArray::getShadowExpression(address, replacements),
                ShadowArray::getShadowExpression(expr, replacements)));
          } else
#endif
            symbolicStore[base].push_back(AddressValuePair(address, expr));
        }
      }
    }
  }

  return std::pair<ConcreteStore, SymbolicStore>(concreteStore, symbolicStore);
}

VersionedValue *Dependency::getLatestValue(llvm::Value *value,
                                           ref<Expr> valueExpr) {
  assert(value && "value cannot be null");
  if (llvm::isa<llvm::ConstantExpr>(value)) {
    llvm::Instruction *asInstruction =
        llvm::dyn_cast<llvm::ConstantExpr>(value)->getAsInstruction();
    if (llvm::isa<llvm::GetElementPtrInst>(asInstruction)) {
      VersionedValue *ret = getNewVersionedValue(value, valueExpr);
      addPointerEquality(ret, getInitialAllocation(value, valueExpr));
      return ret;
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

  VersionedValue *ret = 0;
  if (parentDependency)
    ret = parentDependency->getLatestValue(value, valueExpr);

  if (!ret && llvm::isa<llvm::GlobalValue>(value)) {
    // We could not find the global value: we register it anew.
    ret = getNewVersionedValue(value, valueExpr);
    if (value->getType()->isPointerTy())
      addPointerEquality(ret, getInitialAllocation(value, valueExpr));
  }

  return ret;
}

VersionedValue *Dependency::getLatestValueNoConstantCheck(llvm::Value *value) {
  assert(value && "value cannot be null");

  if (valuesMap.find(value) != valuesMap.end()) {
    return valuesMap[value].back();
  }

  if (parentDependency)
    return parentDependency->getLatestValueNoConstantCheck(value);

  return 0;
}

Allocation *Dependency::getLatestAllocation(llvm::Value *allocation,
                                            ref<Expr> address) const {
  for (std::vector<Allocation *>::const_reverse_iterator
           it = versionedAllocationsList.rbegin(),
           itEnd = versionedAllocationsList.rend();
       it != itEnd; ++it) {
    if ((*it)->hasAllocationSite(allocation, address))
      return *it;
  }

  if (parentDependency)
    return parentDependency->getLatestAllocation(allocation, address);

  return 0;
}

Allocation *Dependency::resolveAllocation(VersionedValue *val) {
  if (!val)
    return 0;

  if (equalityMap.find(val) != equalityMap.end()) {
    return equalityMap[val].back();
  }

  if (parentDependency)
    return parentDependency->resolveAllocation(val);

  // This handles the case when we tried to resolve the allocation
  // yet we could not find the allocation due to it being an argument of main.
  if (Util::isMainArgument(val->getValue())) {
    // We have either argc / argv
    llvm::Argument *vArg = llvm::dyn_cast<llvm::Argument>(val->getValue());
    ref<Expr> addressExpr(val->getExpression());
    Allocation *alloc = getInitialAllocation(vArg, addressExpr);
    addPointerEquality(getNewVersionedValue(vArg, addressExpr), alloc);
    return alloc;
  }

  return 0;
}

std::vector<Allocation *>
Dependency::resolveAllocationTransitively(VersionedValue *value) {
  std::vector<Allocation *> ret;

  if (!value)
    return ret;

  // Lookup address among pointer equalities first
  Allocation *singleRet = resolveAllocation(value);
  if (singleRet) {
    ret.push_back(singleRet);
    return ret;
  }

  // Lookup address by first traversing the flow and then
  // looking up the pointer equalities.
  std::vector<VersionedValue *> valueSources = allFlowSourcesEnds(value);
  for (std::vector<VersionedValue *>::const_iterator it = valueSources.begin(),
                                                     itEnd = valueSources.end();
       it != itEnd; ++it) {
    singleRet = resolveAllocation(*it);
    if (singleRet) {
      ret.push_back(singleRet);
    }
  }

  return ret;
}

void Dependency::addPointerEquality(const VersionedValue *value,
                                    Allocation *allocation) {
  if (equalityMap.find(value) != equalityMap.end()) {
    equalityMap[value].push_back(allocation);
  } else {
    std::vector<Allocation *> newList;
    newList.push_back(allocation);
    equalityMap.insert(
        std::make_pair<const VersionedValue *, std::vector<Allocation *> >(
            value, newList));
  }
}

void Dependency::updateStore(Allocation *allocation, VersionedValue *value) {
  std::map<Allocation *, VersionedValue *>::iterator storesIter =
      storesMap.find(allocation);
  if (storesIter != storesMap.end()) {
    storesMap.at(allocation) = value;
    } else {
      storesMap.insert(
          std::pair<Allocation *, VersionedValue *>(allocation, value));
    }

  // update storageOfMap
    std::map<VersionedValue *, std::vector<Allocation *> >::iterator
    storageOfIter;
    storageOfIter = storageOfMap.find(value);
    if (storageOfIter != storageOfMap.end()) {
    storageOfMap.at(value).push_back(allocation);
  } else {
    std::vector<Allocation *> newList;
    newList.push_back(allocation);
    storageOfMap.insert(std::pair<VersionedValue *, std::vector<Allocation *> >(
        value, newList));
  }
}

void Dependency::addDependency(VersionedValue *source, VersionedValue *target) {
  addDependencyViaAllocation(source, target, 0);
}

void Dependency::addDependencyViaAllocation(VersionedValue *source,
                                            VersionedValue *target,
                                            Allocation *via) {
  if (flowsToMap.find(target) != flowsToMap.end()) {
    flowsToMap[target]
        .insert(std::make_pair<VersionedValue *, Allocation *>(source, via));
  } else {
    std::map<VersionedValue *, Allocation *> newMap;
    newMap.insert(std::make_pair<VersionedValue *, Allocation *>(source, via));
    flowsToMap[target] = newMap;
  }
}

std::vector<VersionedValue *> Dependency::stores(Allocation *allocation) const {
  std::vector<VersionedValue *> ret;

  std::map<Allocation *, VersionedValue *>::const_iterator it;
  it = storesMap.find(allocation);
  if (it != storesMap.end()) {
    ret.push_back(storesMap.at(allocation));
    return ret;
  }

  if (parentDependency)
    return parentDependency->stores(allocation);
  return ret;
}

std::vector<VersionedValue *>
Dependency::directLocalFlowSources(VersionedValue *target) const {
  std::vector<VersionedValue *> ret;
  if (flowsToMap.find(target) != flowsToMap.end()) {
    std::map<VersionedValue *, Allocation *> sources =
        flowsToMap.find(target)->second;
    for (std::map<VersionedValue *, Allocation *>::iterator it =
             sources.begin();
         it != sources.end(); ++it) {
      ret.push_back(it->first);
    }
  }
  return ret;
}

std::vector<VersionedValue *>
Dependency::directFlowSources(VersionedValue *target) const {
  std::vector<VersionedValue *> ret = directLocalFlowSources(target);
  if (parentDependency) {
    std::vector<VersionedValue *> ancestralSources =
        parentDependency->directFlowSources(target);
    ret.insert(ret.begin(), ancestralSources.begin(), ancestralSources.end());
  }
  return ret;
}

std::vector<VersionedValue *>
Dependency::allFlowSources(VersionedValue *target) const {
  std::vector<VersionedValue *> stepSources = directFlowSources(target);
  std::vector<VersionedValue *> ret = stepSources;

  for (std::vector<VersionedValue *>::iterator it = stepSources.begin(),
                                               itEnd = stepSources.end();
       it != itEnd; ++it) {
    std::vector<VersionedValue *> src = allFlowSources(*it);
    ret.insert(ret.begin(), src.begin(), src.end());
  }

  // We include the target as well
  ret.push_back(target);

  // Ensure there are no duplicates in the return
  std::sort(ret.begin(), ret.end());
  std::unique(ret.begin(), ret.end());
  return ret;
}

std::vector<VersionedValue *>
Dependency::allFlowSourcesEnds(VersionedValue *target) const {
  std::vector<VersionedValue *> stepSources = directFlowSources(target);
  std::vector<VersionedValue *> ret;
  if (stepSources.size() == 0) {
    ret.push_back(target);
    return ret;
  }
  for (std::vector<VersionedValue *>::iterator it = stepSources.begin(),
                                               itEnd = stepSources.end();
       it != itEnd; ++it) {
    std::vector<VersionedValue *> src = allFlowSourcesEnds(*it);
    if (src.size() == 0) {
      ret.push_back(*it);
    } else {
      ret.insert(ret.begin(), src.begin(), src.end());
    }
  }

  // Ensure there are no duplicates in the return
  std::sort(ret.begin(), ret.end());
  std::unique(ret.begin(), ret.end());
  return ret;
}

std::vector<VersionedValue *>
Dependency::populateArgumentValuesList(llvm::CallInst *site,
                                       std::vector<ref<Expr> > &arguments) {
  unsigned numArgs = site->getCalledFunction()->arg_size();
  std::vector<VersionedValue *> argumentValuesList;
  for (unsigned i = numArgs; i > 0;) {
    llvm::Value *argOperand = site->getArgOperand(--i);
    VersionedValue *latestValue = getLatestValue(argOperand, arguments.at(i));

    if (latestValue)
      argumentValuesList.push_back(latestValue);
    else {
      // This is for the case when latestValue was NULL, which means there is
      // no source dependency information for this node, e.g., a constant.
      argumentValuesList.push_back(
          new VersionedValue(argOperand, arguments[i]));
    }
  }
  return argumentValuesList;
}

bool Dependency::buildLoadDependency(llvm::Value *address,
                                     ref<Expr> addressExpr, llvm::Value *value,
                                     ref<Expr> valueExpr) {
  VersionedValue *addressValue = getLatestValue(address, addressExpr);
  if (!addressValue)
    return false;

  std::vector<Allocation *> addressAllocList =
      resolveAllocationTransitively(addressValue);

  if (addressAllocList.empty())
    assert(!"operand is not an allocation");

  for (std::vector<Allocation *>::iterator
           allocIter = addressAllocList.begin(),
           allocIterEnd = addressAllocList.end();
       allocIter != allocIterEnd; ++allocIter) {

    std::vector<VersionedValue *> storedValue = stores(*allocIter);

    if (storedValue.empty())
      // We could not find the stored value, create
      // a new one.
      updateStore(*allocIter, getNewVersionedValue(value, valueExpr));
    else {
      for (std::vector<VersionedValue *>::iterator
               storedValueIter = storedValue.begin(),
               storedValueIterEnd = storedValue.end();
           storedValueIter != storedValueIterEnd; ++storedValueIter) {
        // Here we check if the stored value was an address, in
        // which case we add pointer equality. Otherwise, we build
        // value dependency between the return value and the stored value.
        std::vector<Allocation *> storedValueAddressViews =
            resolveAllocationTransitively(*storedValueIter);

        if (storedValueAddressViews.empty())
          addDependencyViaAllocation(*storedValueIter,
                                     getNewVersionedValue(value, valueExpr),
                                     *allocIter);
        else {
          for (std::vector<Allocation *>::iterator
                   it2 = storedValueAddressViews.begin(),
                   it2End = storedValueAddressViews.end();
               it2 != it2End; ++it2) {
            addPointerEquality(getNewVersionedValue(value, valueExpr), *it2);
          }
        }
      }
    }
  }
  return true;
}

Dependency::Dependency(Dependency *prev) : parentDependency(prev) {}

Dependency::~Dependency() {
  // Delete the locally-constructed relations
  Util::deletePointerMapWithVectorValue(equalityMap);
  Util::deletePointerMap(storesMap);
  Util::deletePointerMapWithVectorValue(storageOfMap);
  Util::deletePointerMapWithMapValue(flowsToMap);

  // Delete the locally-constructed objects
  Util::deletePointerMapWithVectorValue(valuesMap);
  Util::deletePointerVector(versionedAllocationsList);
}

Dependency *Dependency::cdr() const { return parentDependency; }

void Dependency::execute(llvm::Instruction *instr,
                         std::vector<ref<Expr> > &args) {
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
        VersionedValue *returnValue = getNewVersionedValue(instr, args.at(0));
        for (unsigned i = 0; i < 3; ++i) {
          VersionedValue *arg =
              getLatestValue(instr->getOperand(i), args.at(i + 1));
          if (arg)
            addDependency(arg, returnValue);
        }
      } else if (calleeName.equals(
                     "_ZNSt13basic_fstreamIcSt11char_traitsIcEE7is_openEv") &&
                 args.size() == 2) {
        VersionedValue *returnValue = getNewVersionedValue(instr, args.at(0));
        VersionedValue *arg = getLatestValue(instr->getOperand(0), args.at(1));
        if (arg)
          addDependency(arg, returnValue);
      } else if (calleeName.equals("_ZNSi5tellgEv") && args.size() == 2) {
        VersionedValue *returnValue = getNewVersionedValue(instr, args.at(0));
        VersionedValue *arg = getLatestValue(instr->getOperand(0), args.at(1));
        if (arg)
          addDependency(arg, returnValue);
      } else if ((calleeName.equals("powl") && args.size() == 3) ||
                 (calleeName.equals("gettimeofday") && args.size() == 3)) {
        VersionedValue *returnValue = getNewVersionedValue(instr, args.at(0));
        for (unsigned i = 0; i < 2; ++i) {
          VersionedValue *arg =
              getLatestValue(instr->getOperand(i), args.at(i + 1));
          if (arg)
            addDependency(arg, returnValue);
        }
      } else if (calleeName.equals("malloc") && args.size() == 1) {
        // malloc is an allocation-type instruction: its single argument is the
        // return address.
        addPointerEquality(getNewVersionedValue(instr, args.at(0)),
                           getInitialAllocation(instr, args.at(0)));
      } else if (calleeName.equals("realloc") && args.size() == 1) {
        // realloc is an allocation-type instruction: its single argument is the
        // return address.
        VersionedValue *returnValue = getNewVersionedValue(instr, args.at(0));
        VersionedValue *arg = getLatestValue(instr->getOperand(0), args.at(0));
        if (arg)
          addDependency(arg, returnValue);
      } else if (calleeName.equals("calloc") && args.size() == 1) {
        // calloc is an allocation-type instruction: its single argument is the
        // return address.
        addPointerEquality(getNewVersionedValue(instr, args.at(0)),
                           getInitialAllocation(instr, args.at(0)));
      } else if (calleeName.equals("syscall") && args.size() >= 2) {
        VersionedValue *returnValue = getNewVersionedValue(instr, args.at(0));
        for (unsigned i = 0; i + 1 < args.size(); ++i) {
          VersionedValue *arg =
              getLatestValue(instr->getOperand(i), args.at(i + 1));
          if (arg)
            addDependency(arg, returnValue);
        }
      } else if (std::mismatch(getValuePrefix.begin(), getValuePrefix.end(),
                               calleeName.begin()).first ==
                     getValuePrefix.end() &&
                 args.size() == 2) {
        VersionedValue *returnValue = getNewVersionedValue(instr, args.at(0));
        VersionedValue *arg = getLatestValue(instr->getOperand(0), args.at(1));
        if (arg)
          addDependency(arg, returnValue);
      } else if (calleeName.equals("getenv") && args.size() == 2) {
        addPointerEquality(getNewVersionedValue(instr, args.at(0)),
                           getInitialAllocation(instr, args.at(0)));
      } else if (calleeName.equals("printf") && args.size() >= 2) {
        VersionedValue *returnValue = getNewVersionedValue(instr, args.at(0));
        VersionedValue *formatArg =
            getLatestValue(instr->getOperand(0), args.at(1));
        if (formatArg)
          addDependency(formatArg, returnValue);
        for (unsigned i = 2, argsNum = args.size(); i < argsNum; ++i) {
          VersionedValue *arg =
              getLatestValue(instr->getOperand(i - 1), args.at(i));
          if (arg)
            addDependency(arg, returnValue);
        }
      } else if (calleeName.equals("vprintf") && args.size() == 3) {
        VersionedValue *returnValue = getNewVersionedValue(instr, args.at(0));
        VersionedValue *arg0 = getLatestValue(instr->getOperand(0), args.at(1));
        VersionedValue *arg1 = getLatestValue(instr->getOperand(1), args.at(2));
        if (arg0)
          addDependency(arg0, returnValue);
        if (arg1)
          addDependency(arg1, returnValue);
      } else if (((calleeName.equals("fchmodat") && args.size() == 5)) ||
                 (calleeName.equals("fchownat") && args.size() == 6)) {
        VersionedValue *returnValue = getNewVersionedValue(instr, args.at(0));
        for (unsigned i = 0; i < 2; ++i) {
          VersionedValue *arg =
              getLatestValue(instr->getOperand(i), args.at(i + 1));
          if (arg)
            addDependency(arg, returnValue);
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
        AllocationGraph *g = new AllocationGraph();
        markAllValues(g, binst->getCondition());
        computeCoreAllocations(g);
        delete g;
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
    case llvm::Instruction::Alloca: {
      addPointerEquality(getNewVersionedValue(instr, argExpr),
                         getInitialAllocation(instr, argExpr));
      break;
    }
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
      VersionedValue *val = getLatestValue(instr->getOperand(0), argExpr);

      if (val) {
        addDependency(val, getNewVersionedValue(instr, argExpr));
      } else if (!llvm::isa<llvm::Constant>(instr->getOperand(0)))
          // Constants would kill dependencies, the remaining is for
          // cases that may actually require dependencies.
      {
        if (instr->getOperand(0)->getType()->isPointerTy()) {
          addPointerEquality(
              getNewVersionedValue(instr, argExpr),
              getInitialAllocation(instr->getOperand(0), argExpr));
        } else if (llvm::isa<llvm::Argument>(instr->getOperand(0))) {
          VersionedValue *arg =
              getNewVersionedValue(instr->getOperand(0), argExpr);
          VersionedValue *returnValue = getNewVersionedValue(instr, argExpr);
          if (arg)
            addDependency(arg, returnValue);
        } else if (llvm::isa<llvm::CallInst>(instr->getOperand(0))) {
          getNewVersionedValue(instr->getOperand(0), argExpr);
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
    case llvm::Instruction::Load: {
      VersionedValue *addressValue =
          getLatestValue(instr->getOperand(0), address);
      if (addressValue) {
        std::vector<Allocation *> allocations =
            resolveAllocationTransitively(addressValue);
        if (allocations.empty()) {
          Allocation *alloc =
              getInitialAllocation(instr->getOperand(0), address);
          addPointerEquality(addressValue, alloc);
          updateStore(alloc, getNewVersionedValue(instr, valueExpr));
          break;
        } else if (allocations.size() == 1) {
          if (Util::isMainArgument(allocations.at(0)->getSite())) {
            // The load corresponding to a load of the main function's
            // argument that was never allocated within this program.
            addPointerEquality(getNewVersionedValue(instr, valueExpr),
                               getNewAllocationVersion(instr, address));
            break;
          }
        }
      } else {
        // The value not found was a global variable,
        // record it here.
        if (llvm::isa<llvm::GlobalVariable>(instr->getOperand(0))) {
          addressValue = getNewVersionedValue(instr->getOperand(0), address);
          Allocation *alloc =
              getInitialAllocation(instr->getOperand(0), address);
          addPointerEquality(addressValue, alloc);
        }
      }

      if (!buildLoadDependency(instr->getOperand(0), address, instr,
                               valueExpr)) {
        Allocation *alloc = getInitialAllocation(instr->getOperand(0), address);
        updateStore(alloc, getNewVersionedValue(instr, valueExpr));
      }
      break;
    }
    case llvm::Instruction::Store: {
      VersionedValue *dataArg = getLatestValue(instr->getOperand(0), valueExpr);
      std::vector<Allocation *> addressList = resolveAllocationTransitively(
          getLatestValue(instr->getOperand(1), address));

      // If there was no dependency found, we should create
      // a new value
      if (!dataArg)
        dataArg = getNewVersionedValue(instr->getOperand(0), valueExpr);

      for (std::vector<Allocation *>::iterator it = addressList.begin(),
                                               itEnd = addressList.end();
           it != itEnd; ++it) {
        Allocation *allocation =
            getLatestAllocation((*it)->getSite(), (*it)->getAddress());
        if (!allocation) {
          allocation = getInitialAllocation((*it)->getSite(), address);
          VersionedValue *allocationValue =
              getNewVersionedValue((*it)->getSite(), valueExpr);
          addPointerEquality(allocationValue, allocation);
        }
        updateStore(allocation, dataArg);
      }

      break;
    }
    case llvm::Instruction::GetElementPtr: {
      if (llvm::isa<llvm::Constant>(instr->getOperand(0))) {
        // We look up existing allocations with the same site as the argument,
        // but with the address given as valueExpr (the value of the
        // getelementptr instruction itself).
        Allocation *actualAllocation =
            getLatestAllocation(instr->getOperand(0), valueExpr);
        if (!actualAllocation)
          actualAllocation =
              getInitialAllocation(instr->getOperand(0), valueExpr);

        // We simply propagate the pointer to the current
        addPointerEquality(getNewVersionedValue(instr, valueExpr),
                           actualAllocation);
        break;
      }

      VersionedValue *addressValue =
          getLatestValue(instr->getOperand(0), address);

      if (!addressValue) {
        // We define a new base anyway in case the operand was not found and was
        // an inbound.
        llvm::GetElementPtrInst *gepInst =
            llvm::dyn_cast<llvm::GetElementPtrInst>(instr);
        assert(gepInst->isInBounds() && "operand not found");
        addressValue = getNewVersionedValue(instr->getOperand(0), address);
      }

      std::vector<Allocation *> addressAllocations =
          resolveAllocationTransitively(addressValue);

      // Allocations
      if (addressAllocations.size() > 0) {
        VersionedValue *newValue = getNewVersionedValue(instr, valueExpr);
        for (std::vector<Allocation *>::iterator
                 it = addressAllocations.begin(),
                 itEnd = addressAllocations.end();
             it != itEnd; ++it) {
          // We check existing allocations with the same site as the allocation,
          // but with the address given as valueExpr (the value of the
          // getelementptr instruction itself).
          Allocation *actualAllocation =
              getLatestAllocation((*it)->getSite(), valueExpr);
          if (!actualAllocation)
            actualAllocation =
                getInitialAllocation((*it)->getSite(), valueExpr);
          addPointerEquality(newValue, actualAllocation);
        }
      } else {
        // Here the base is not found as an address,
        // try to add flow dependency between values
        std::vector<VersionedValue *> directSources =
            directFlowSources(addressValue);
        if (directSources.size() > 0) {
          VersionedValue *newValue = getNewVersionedValue(instr, valueExpr);
          for (std::vector<VersionedValue *>::iterator
                   it = directSources.begin(),
                   itEnd = directSources.end();
               it != itEnd; ++it) {
            if (*it)
              addDependency((*it), newValue);
          }
        } else {
          // Here getelementptr forcibly uses a value not known to be an
          // address, e.g., a loaded value, as an address. In this case, we then
          // assume that the argument is a base allocation.
          addPointerEquality(
              getNewVersionedValue(instr, valueExpr),
              getInitialAllocation(addressValue->getValue(), valueExpr));
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

      VersionedValue *op1 = getLatestValue(instr->getOperand(1), op1Expr);
      VersionedValue *op2 = getLatestValue(instr->getOperand(2), op2Expr);
      VersionedValue *newValue = 0;
      if (op1) {
        newValue = getNewVersionedValue(instr, result);
        addDependency(op1, newValue);
      }
      if (op2) {
        if (newValue)
          addDependency(op2, newValue);
        else
          addDependency(op2, getNewVersionedValue(instr, result));
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
      VersionedValue *op1 = getLatestValue(instr->getOperand(0), op1Expr);
      VersionedValue *op2 = getLatestValue(instr->getOperand(1), op2Expr);

      if (!op1 &&
          (instr->getParent()->getParent()->getName().equals("klee_range") &&
           instr->getOperand(0)->getName().equals("start"))) {
        op1 = getNewVersionedValue(instr->getOperand(0), op1Expr);
      }
      if (!op2 &&
          (instr->getParent()->getParent()->getName().equals("klee_range") &&
           instr->getOperand(1)->getName().equals("end"))) {
        op2 = getNewVersionedValue(instr->getOperand(1), op2Expr);
      }

      VersionedValue *newValue = 0;
      if (op1) {
        newValue = getNewVersionedValue(instr, result);
        addDependency(op1, newValue);
      }
      if (op2) {
        if (newValue)
          addDependency(op2, newValue);
        else
          addDependency(op2, getNewVersionedValue(instr, result));
      }
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
                            unsigned int incomingBlock, ref<Expr> valueExpr) {
  llvm::PHINode *node = llvm::dyn_cast<llvm::PHINode>(instr);
  llvm::Value *llvmArgValue = node->getIncomingValue(incomingBlock);
  VersionedValue *val = getLatestValue(llvmArgValue, valueExpr);
  if (val) {
    addDependency(val, getNewVersionedValue(instr, valueExpr));
  } else if (llvm::isa<llvm::Constant>(llvmArgValue) ||
             llvm::isa<llvm::Argument>(llvmArgValue)) {
    getNewVersionedValue(instr, valueExpr);
  } else {
    assert(!"operand not found");
  }
}

void Dependency::bindCallArguments(llvm::Instruction *i,
                                   std::vector<ref<Expr> > &arguments) {
  llvm::CallInst *site = llvm::dyn_cast<llvm::CallInst>(i);

  if (!site)
    return;

  llvm::Function *callee = site->getCalledFunction();

  // Sometimes the callee information is missing, in which case
  // the calle is not to be symbolically tracked.
  if (!callee)
    return;

  argumentValuesList = populateArgumentValuesList(site, arguments);
  unsigned index = 0;
  for (llvm::Function::ArgumentListType::iterator
           it = callee->getArgumentList().begin(),
           itEnd = callee->getArgumentList().end();
       it != itEnd; ++it) {
    if (argumentValuesList.back()) {
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
    VersionedValue *value =
        getLatestValue(retInst->getReturnValue(), returnValue);
    if (value)
      addDependency(value, getNewVersionedValue(site, returnValue));
  }
}

void Dependency::markAllValues(AllocationGraph *g, VersionedValue *value) {
  buildAllocationGraph(g, value);
  std::vector<VersionedValue *> allSources = allFlowSources(value);
  for (std::vector<VersionedValue *>::iterator it = allSources.begin(),
                                               itEnd = allSources.end();
       it != itEnd; ++it) {
    (*it)->setAsCore();
  }
}

void Dependency::markAllValues(AllocationGraph *g, llvm::Value *val) {
  VersionedValue *value = getLatestValueNoConstantCheck(val);

  // Right now we simply ignore the __dso_handle values. They are due
  // to library / linking errors caused by missing options (-shared) in the
  // compilation involving shared library.
  if (!value) {
    if (llvm::ConstantExpr *cVal = llvm::dyn_cast<llvm::ConstantExpr>(val)) {
      for (unsigned i = 0; i < cVal->getNumOperands(); ++i) {
        if (cVal->getOperand(i)->getName().equals("__dso_handle")) {
          return;
        }
      }
    }

    if (llvm::isa<llvm::Constant>(val))
      return;

    assert(!"unknown value");
  }

  markAllValues(g, value);
}

void Dependency::computeCoreAllocations(AllocationGraph *g) {
  std::set<Allocation *> sinkAllocations(g->getSinkAllocations());
  coreAllocations.insert(sinkAllocations.begin(), sinkAllocations.end());

  if (parentDependency) {
    // Here we remove sink nodes with allocations that belong to
    // this dependency node. As a result, the sinks in the graph g
    // should just contain the allocations that belong to the ancestor
    // dependency nodes, and we then recursively compute the
    // core allocations for the parent.
    g->consumeSinksWithAllocations(versionedAllocationsList);
    parentDependency->computeCoreAllocations(g);
  }
}

std::map<VersionedValue *, Allocation *>
Dependency::directLocalAllocationSources(VersionedValue *target) const {
  std::map<VersionedValue *, Allocation *> ret;

  if (flowsToMap.find(target) != flowsToMap.end()) {
    std::map<VersionedValue *, Allocation *> sources =
        flowsToMap.find(target)->second;
    for (std::map<VersionedValue *, Allocation *>::iterator it =
             sources.begin();
         it != sources.end(); ++it) {
      std::map<VersionedValue *, Allocation *> extra;
      if (!it->second) {
        // Transitively get the source
        extra = directLocalAllocationSources(it->first);
        if (extra.size()) {
          ret.insert(extra.begin(), extra.end());
        } else {
          ret[it->first] = 0;
        }
      } else {
        ret[it->first] = it->second;
      }
    }
  }

  if (ret.empty()) {
    // We try to find allocation in the local store instead
    std::map<VersionedValue *, std::vector<Allocation *> >::const_iterator it;
    it = storageOfMap.find(target);
    if (it != storageOfMap.end()) {
      std::vector<Allocation *> allocList = it->second;
      int size = allocList.size();
      ret[0] = allocList.at(size - 1);
    }
  }

  return ret;
}

std::map<VersionedValue *, Allocation *>
Dependency::directAllocationSources(VersionedValue *target) const {
  std::map<VersionedValue *, Allocation *> ret =
      directLocalAllocationSources(target);

  if (ret.empty() && parentDependency)
    return parentDependency->directAllocationSources(target);

  std::map<VersionedValue *, Allocation *> tmp;
  std::map<VersionedValue *, Allocation *>::iterator nextPos = ret.begin(),
                                                     itEnd = ret.end();

  bool elementErased = true;
  while (elementErased) {
    elementErased = false;
    for (std::map<VersionedValue *, Allocation *>::iterator it = nextPos;
         it != itEnd; ++it) {
      if (!it->second) {
        std::map<VersionedValue *, Allocation *>::iterator deletionPos = it;

        // Here we check that it->first was non-nil, as it is possibly so.
        if (parentDependency && it->first) {
          std::map<VersionedValue *, Allocation *> ancestralSources =
              parentDependency->directAllocationSources(it->first);
          tmp.insert(ancestralSources.begin(), ancestralSources.end());
        }

        nextPos = ++it;
        ret.erase(deletionPos);
        elementErased = true;
        break;
      }
    }
  }

  ret.insert(tmp.begin(), tmp.end());
  return ret;
}

void Dependency::recursivelyBuildAllocationGraph(
    AllocationGraph *g, VersionedValue *source, Allocation *target,
    std::set<Allocation *> parentTargets) const {
  if (!source)
    return;

  std::vector<Allocation *> ret;
  std::map<VersionedValue *, Allocation *> sourceEdges =
      directAllocationSources(source);

  for (std::map<VersionedValue *, Allocation *>::iterator
           it = sourceEdges.begin(),
           itEnd = sourceEdges.end();
       it != itEnd; ++it) {
    // Here we prevent construction of cycle in the graph by checking if the
    // source equals target or included as an ancestor.
    if (it->second != target &&
        parentTargets.find(it->second) == parentTargets.end()) {
      g->addNewEdge(it->second, target);
      parentTargets.insert(target);
      recursivelyBuildAllocationGraph(g, it->first, it->second, parentTargets);
    }
  }
}

void Dependency::buildAllocationGraph(AllocationGraph *g,
                                      VersionedValue *target) const {
  std::vector<Allocation *> ret;
  std::map<VersionedValue *, Allocation *> sourceEdges =
      directAllocationSources(target);

  for (std::map<VersionedValue *, Allocation *>::iterator
           it = sourceEdges.begin(),
           itEnd = sourceEdges.end();
       it != itEnd; ++it) {
    g->addNewSink(it->second);
    recursivelyBuildAllocationGraph(g, it->first, it->second,
                                    std::set<Allocation *>());
  }
}

void Dependency::print(llvm::raw_ostream &stream) const {
  this->print(stream, 0);
}

void Dependency::print(llvm::raw_ostream &stream,
                       const unsigned paddingAmount) const {
  std::string tabs = makeTabs(paddingAmount);
  stream << tabs << "EQUALITIES:";
  std::map<const VersionedValue *, std::vector<Allocation *> >::const_iterator
  equalityMapBegin = equalityMap.begin();
  std::map<Allocation *, VersionedValue *>::const_iterator storesMapBegin =
      storesMap.begin();
  std::map<VersionedValue *,
           std::map<VersionedValue *, Allocation *> >::const_iterator
  flowsToMapBegin = flowsToMap.begin();
  for (std::map<const VersionedValue *,
                std::vector<Allocation *> >::const_iterator
           it = equalityMap.begin(),
           itEnd = equalityMap.end();
       it != itEnd; ++it) {
    if (it != equalityMapBegin)
      stream << ",";
    stream << "[";
    (*it->first).print(stream);
    stream << "=={";
    std::vector<Allocation *> allocList = (*it).second;
    std::vector<Allocation *>::iterator allocListBegin = allocList.begin();
    for (std::vector<Allocation *>::iterator it2 = allocList.begin(),
                                             itEnd2 = allocList.end();
         it2 != itEnd2; ++it2) {
      if (it2 != allocListBegin)
        stream << ",";
      (*it2)->print(stream);
    }
    stream << "}]";
  }
  stream << "\n";
  stream << tabs << "STORAGE:";
  for (std::map<Allocation *, VersionedValue *>::const_iterator
           it = storesMap.begin(),
           itEnd = storesMap.end();
       it != itEnd; ++it) {
    if (it != storesMapBegin)
      stream << ",";
    stream << "[";
    (*it->first).print(stream);
    stream << ",";
    (*it->second).print(stream);
    stream << "]";
  }
  stream << "\n";
  stream << tabs << "FLOWDEPENDENCY:";
  for (std::map<VersionedValue *,
                std::map<VersionedValue *, Allocation *> >::const_iterator
           it = flowsToMap.begin(),
           itEnd = flowsToMap.end();
       it != itEnd; ++it) {
    if (it != flowsToMapBegin)
      stream << ",";
    std::map<VersionedValue *, Allocation *> sources = (*it).second;
    std::map<VersionedValue *, Allocation *>::iterator sourcesMapBegin =
        sources.begin();
    for (std::map<VersionedValue *, Allocation *>::iterator it2 =
             sources.begin();
         it2 != sources.end(); ++it2) {
      if (it2 != sourcesMapBegin)
        stream << ",";
      stream << "[";
      (*it->first).print(stream);
      stream << " <- ";
      (*it2->first).print(stream);
      stream << "]";
      if (it2->second != NULL) {
        stream << " via ";
        (*it2->second).print(stream);
      }
    }
  }

  if (parentDependency) {
    stream << "\n" << tabs << "--------- Parent Dependencies ----------\n";
    parentDependency->print(stream, paddingAmount);
  }
}

/**/

template <typename T>
void Dependency::Util::deletePointerVector(std::vector<T *> &list) {
  typedef typename std::vector<T *>::iterator IteratorType;

  for (IteratorType it = list.begin(), itEnd = list.end(); it != itEnd; ++it) {
    delete *it;
  }
  list.clear();
}

template <typename K, typename T>
void Dependency::Util::deletePointerMap(std::map<K *, T *> &map) {
  map.clear();
}

template <typename K, typename T>
void Dependency::Util::deletePointerMapWithVectorValue(
    std::map<K *, std::vector<T *> > &map) {
  typedef typename std::map<K *, std::vector<T *> >::iterator IteratorType;

  for (IteratorType it = map.begin(), itEnd = map.end(); it != itEnd; ++it) {
    it->second.clear();
  }
  map.clear();
}

template <typename K, typename T>
void Dependency::Util::deletePointerMapWithMapValue(
    std::map<K *, std::map<K *, T *> > &map) {
  typedef typename std::map<K *, std::map<K *, T *> >::iterator IteratorType;

  for (IteratorType it = map.begin(), itEnd = map.end(); it != itEnd; ++it) {
    it->second.clear();
  }
  map.clear();
}

bool Dependency::Util::isMainArgument(llvm::Value *site) {
  llvm::Argument *vArg = llvm::dyn_cast<llvm::Argument>(site);

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
