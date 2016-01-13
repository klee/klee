
#include "Dependency.h"

using namespace klee;

namespace klee {

Allocation::Allocation() : site(0) {}

Allocation::~Allocation() {}

bool Allocation::hasAllocationSite(llvm::Value *site) const { return false; }

bool Allocation::isComposite() const {
  // We return true by default as composites are
  // more generally handled.
  return true;
}

void Allocation::print(llvm::raw_ostream &stream) const {
  // Do nothing
}

/**/

unsigned long long VersionedAllocation::nextVersion = 0;

  VersionedAllocation::VersionedAllocation(llvm::Value *site)
      : version(nextVersion++), site(site) {}

  VersionedAllocation::~VersionedAllocation() {}

  bool VersionedAllocation::hasAllocationSite(llvm::Value *site) const {
    return this->site == site;
  }

  bool VersionedAllocation::isComposite() const {
    llvm::AllocaInst *inst = llvm::dyn_cast<llvm::AllocaInst>(site);

    if (inst != 0)
      return llvm::isa<llvm::CompositeType>(inst->getAllocatedType());

    switch (site->getType()->getTypeID()) {
      case llvm::Type::ArrayTyID:
      case llvm::Type::PointerTyID:
      case llvm::Type::StructTyID:
      case llvm::Type::VectorTyID:
	{
	  return true;
	}
      default:
	break;
    }

    return false;
  }

  void VersionedAllocation::print(llvm::raw_ostream& stream) const {
    stream << "A[";
    site->print(stream);
    stream << "#" << version;
    stream << "]";
  }

  /**/

  EnvironmentAllocation::EnvironmentAllocation() {}

  EnvironmentAllocation::~EnvironmentAllocation() {}

  bool EnvironmentAllocation::hasAllocationSite(llvm::Value *site) const {
    return isEnvironmentAllocation(site);
  }

  void EnvironmentAllocation::print(llvm::raw_ostream& stream) const {
    stream << "A[@__environ]" << this;
  }

  /**/

  unsigned long long VersionedValue::nextVersion = 0;

  VersionedValue::VersionedValue(llvm::Value *value, ref<Expr> valueExpr)
      : value(value), valueExpr(valueExpr), version(nextVersion++),
        inInterpolant(false) {}

  VersionedValue::~VersionedValue() {}

  bool VersionedValue::hasValue(llvm::Value *value) const {
    return this->value == value;
  }

  ref<Expr> VersionedValue::getExpression() const { return valueExpr; }

  void VersionedValue::includeInInterpolant() {
    inInterpolant = true;
  }

  bool VersionedValue::valueInInterpolant() const {
    return inInterpolant;
  }

  void VersionedValue::print(llvm::raw_ostream& stream) const {
    stream << "V";
    if (inInterpolant)
      stream << "(I)";
    stream << "[";
    value->print(stream);
    stream << "#" << version << ":";
    valueExpr->print(stream);
    stream << "]";
  }

  /**/

  PointerEquality::PointerEquality(VersionedValue *value,
                                   Allocation *allocation)
      : value(value), allocation(allocation) {}

  PointerEquality::~PointerEquality() {}

  Allocation *PointerEquality::equals(VersionedValue *value) const {
    return this->value == value ? allocation : 0;
  }

  void PointerEquality::print(llvm::raw_ostream& stream) const {
    stream << "(";
    value->print(stream);
    stream << "==";
    allocation->print(stream);
    stream << ")";
  }

  /**/

  StorageCell::StorageCell(Allocation *allocation, VersionedValue *value)
      : allocation(allocation), value(value) {}

  StorageCell::~StorageCell() {}

  VersionedValue *StorageCell::stores(Allocation *allocation) const {
    return this->allocation == allocation ? this->value : 0;
  }

  Allocation *StorageCell::storageOf(VersionedValue *value) const {
    return this->value == value ? this->allocation : 0;
  }

  Allocation *StorageCell::getAllocation() const {
    return this->allocation;
  }

  void StorageCell::print(llvm::raw_ostream& stream) const {
    stream << "[";
    allocation->print(stream);
    stream << ",";
    value->print(stream);
    stream << "]";
  }

  /**/

  FlowsTo::FlowsTo(VersionedValue *source, VersionedValue *target)
      : source(source), target(target) {}

  FlowsTo::~FlowsTo() {}

  VersionedValue *FlowsTo::getSource() const {
    return this->source;
  }

  VersionedValue *FlowsTo::getTarget() const {
    return this->target;
  }

  void FlowsTo::print(llvm::raw_ostream &stream) const {
    source->print(stream);
    stream << "->";
    target->print(stream);
  }

  /**/

  VersionedValue *Dependency::getNewVersionedValue(llvm::Value *value,
                                                   ref<Expr> valueExpr) {
    VersionedValue *ret = new VersionedValue(value, valueExpr);
    valuesList.push_back(ret);
    return ret;
  }

  Allocation *Dependency::getNewAllocation(llvm::Value *allocation) {
    Allocation *ret;
    if (isEnvironmentAllocation(allocation)) {
      ret = getLatestAllocation(allocation);
      if (!ret) {
	ret = new EnvironmentAllocation();
	allocationsList.push_back(ret);
      }
    } else {
	ret = new VersionedAllocation(allocation);
	if (!ret->isComposite()) {
	    // We register noncomposites in a special list,
	    // as these are the ones that are truly versioned.
	    // Composites are not truly versioned as destructive
	    // updates don't apply to them.
	    newVersionedAllocations.push_back(allocation);
	}
	allocationsList.push_back(ret);
    }

    return ret;
  }

  std::vector<llvm::Value *> Dependency::getAllVersionedAllocations() const {
    std::vector<llvm::Value *> allAlloc = newVersionedAllocations;
    if (parentDependency) {
	       std::vector<llvm::Value *> parentVersionedAllocations =
		   parentDependency->getAllVersionedAllocations();
	       allAlloc.insert(allAlloc.begin(), parentVersionedAllocations.begin(),
	                  parentVersionedAllocations.end());
    }
    return allAlloc;
  }

  std::vector< std::pair<llvm::Value *, ref<Expr> > > Dependency::getLatestCoreExpressions() const {
    std::vector<llvm::Value *> allAlloc = getAllVersionedAllocations();
    std::vector< std::pair<llvm::Value *, ref<Expr> > > ret;

    for (std::vector<llvm::Value *>::iterator it0 = allAlloc.begin(),
	it0End = allAlloc.end(); it0 != it0End; ++it0) {
	std::vector<VersionedValue *> stored = stores(getLatestAllocation(*it0));

	for (std::vector<VersionedValue *>::iterator it1 = stored.begin(),
	    it1End = stored.end(); it1 != it1End; ++it1) {
	    if ((*it1)->valueInInterpolant()) {
		std::pair<llvm::Value *, ref<Expr> > newPair((*it0), (*it1)->getExpression());
		ret.push_back(newPair);
	    }
	}

    }
    return ret;
  }

  std::vector< std::pair<llvm::Value *, std::vector<ref<Expr> > > >
  Dependency::getCompositeCoreExpressions() const {
    std::vector<VersionedValue *> values;

    for (std::vector<StorageCell *>::const_iterator it = storesList.begin(),
	itEnd = storesList.end(); it != itEnd; ++it) {
	Allocation *alloc = (*it)->getAllocation();
	if (alloc->isComposite()) {
	    // stores(alloc);
	}
    }
  }

  VersionedValue *Dependency::getLatestValue(llvm::Value *value) const {
    for (std::vector<VersionedValue *>::const_reverse_iterator
             it = valuesList.rbegin(),
             itEnd = valuesList.rend();
         it != itEnd; ++it) {
      if ((*it)->hasValue(value))
        return *it;
    }

    if (parentDependency)
      return parentDependency->getLatestValue(value);

    return 0;
  }

  Allocation *Dependency::getLatestAllocation(llvm::Value *allocation) const {
    for (std::vector<Allocation *>::const_reverse_iterator
             it = allocationsList.rbegin(),
             itEnd = allocationsList.rend();
         it != itEnd; ++it) {
      if ((*it)->hasAllocationSite(allocation))
        return *it;
    }

    if (parentDependency)
      return parentDependency->getLatestAllocation(allocation);

    return 0;
  }

  Allocation *Dependency::resolveAllocation(VersionedValue *val) const {
    if (!val) return 0;
    for (std::vector< PointerEquality *>::const_reverse_iterator
	it = equalityList.rbegin(),
	itEnd = equalityList.rend(); it != itEnd; ++it) {
      Allocation *alloc = (*it)->equals(val);
      if (alloc)
        return alloc;
    }

    if (parentDependency)
      return parentDependency->resolveAllocation(val);

    return 0;
  }

  std::vector<Allocation *>
  Dependency::resolveAllocationTransitively(VersionedValue *value) const {
    std::vector<Allocation *> ret;
    Allocation *singleRet = resolveAllocation(value);
    if (singleRet) {
	ret.push_back(singleRet);
	return ret;
    }

    std::vector<VersionedValue *> valueSources = allFlowSourcesEnds(value);
    for (std::vector<VersionedValue *>::const_iterator it = valueSources.begin(),
	itEnd = valueSources.end(); it != itEnd; ++it) {
	singleRet = resolveAllocation(*it);
	if (singleRet) {
	    ret.push_back(singleRet);
	}
    }
    return ret;
  }

  void Dependency::addPointerEquality(VersionedValue *value,
                                      Allocation *allocation) {
    equalityList.push_back(new PointerEquality(value, allocation));
  }

  void Dependency::updateStore(Allocation *allocation, VersionedValue *value) {
    storesList.push_back(new StorageCell(allocation, value));
  }

  void Dependency::addDependency(VersionedValue *source,
                                 VersionedValue *target) {
    flowsToList.push_back(new FlowsTo(source, target));
  }

  std::vector<VersionedValue *>
  Dependency::stores(Allocation *allocation) const {
    std::vector<VersionedValue *> ret;

    if (allocation->isComposite()) {
      // In case of composite allocation, we return all possible stores
      // due to field-insensitivity of the dependency relation
      for (std::vector<StorageCell *>::const_iterator it = storesList.begin(),
                                                      itEnd = storesList.end();
           it != itEnd; ++it) {
        VersionedValue *value = (*it)->stores(allocation);
        if (value) {
          ret.push_back(value);
        }
      }

      if (parentDependency) {
        std::vector<VersionedValue *> parentStoredValues =
            parentDependency->stores(allocation);
        ret.insert(ret.begin(), parentStoredValues.begin(),
                   parentStoredValues.end());
      }
      return ret;
    }

    for (std::vector<StorageCell *>::const_iterator it = storesList.begin(),
                                                    itEnd = storesList.end();
         it != itEnd; ++it) {
        VersionedValue *value = (*it)->stores(allocation);
        if (value) {
          ret.push_back(value);
          return ret;
        }
      }
      if (parentDependency)
        return parentDependency->stores(allocation);
    return ret;
  }

  std::vector<VersionedValue *>
  Dependency::oneStepLocalFlowSources(VersionedValue *target) const {
    std::vector<VersionedValue *> ret;
    for (std::vector<FlowsTo *>::const_iterator it = flowsToList.begin(),
                                          itEnd = flowsToList.end();
         it != itEnd; ++it) {
      if ((*it)->getTarget() == target) {
	ret.push_back((*it)->getSource());
      }
    }
    return ret;
  }

  std::vector<VersionedValue *>
  Dependency::oneStepFlowSources(VersionedValue *target) const {
    std::vector<VersionedValue *> ret = oneStepLocalFlowSources(target);
    if (parentDependency) {
	std::vector<VersionedValue *> ancestralSources =
	    parentDependency->oneStepFlowSources(target);
	ret.insert(ret.begin(), ancestralSources.begin(),
	           ancestralSources.end());
    }
    return ret;
  }

  std::vector<VersionedValue *>
  Dependency::allFlowSources(VersionedValue *target) const {
    std::vector<VersionedValue *> stepSources = oneStepFlowSources(target);
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
    std::vector<VersionedValue *> stepSources = oneStepFlowSources(target);
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
      VersionedValue *latestValue = getLatestValue(argOperand);

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

  bool Dependency::buildLoadDependency(llvm::Value *fromValue,
                                       llvm::Value *toValue,
                                       ref<Expr> toValueExpr) {
    VersionedValue *arg = getLatestValue(fromValue);
    if (!arg)
      return false;

    std::vector<Allocation *> allocList = resolveAllocationTransitively(arg);
    if (allocList.size() > 0) {
      for (std::vector<Allocation *>::iterator it0 = allocList.begin(),
                                               it0End = allocList.end();
           it0 != it0End; ++it0) {
        std::vector<VersionedValue *> valList = stores(*it0);
        if (valList.size() > 0) {
          for (std::vector<VersionedValue *>::iterator it1 = valList.begin(),
                                                       it1End = valList.end();
               it1 != it1End; ++it1) {
            std::vector<Allocation *> alloc2 =
                resolveAllocationTransitively(*it1);
            if (alloc2.size() > 0) {
              for (std::vector<Allocation *>::iterator it2 = alloc2.begin(),
                                                       it2End = alloc2.end();
                   it2 != it2End; ++it2) {
                addPointerEquality(getNewVersionedValue(toValue, toValueExpr),
                                   *it2);
              }
            } else {
              addDependency(*it1, getNewVersionedValue(toValue, toValueExpr));
            }
          }
        } else {
          // We could not find the stored value, create
          // a new one.
          updateStore(*it0, getNewVersionedValue(toValue, toValueExpr));
        }
      }
    } else {
	assert (!"operand is not an allocation");
    }

    return true;
  }

  Dependency::Dependency(Dependency *prev) : parentDependency(prev) {}

  Dependency::~Dependency() {
    // Delete the locally-constructed relations
    deletePointerVector(equalityList);
    deletePointerVector(storesList);
    deletePointerVector(flowsToList);

    // Delete the locally-constructed objects
    deletePointerVector(valuesList);
    deletePointerVector(allocationsList);
  }

  Dependency *Dependency::cdr() const { return parentDependency; }

  void Dependency::execute(llvm::Instruction *i, ref<Expr> valueExpr) {
    // The basic design principle that we need to be careful here
    // is that we should not store quadratic-sized structures in
    // the database of computed relations, e.g., not storing the
    // result of traversals of the graph. We keep the
    // quadratic blow up for only when querying the database.
    unsigned opcode = i->getOpcode();

    assert(opcode != llvm::Instruction::Invoke &&
           opcode != llvm::Instruction::Call &&
           opcode != llvm::Instruction::Ret &&
           "should not execute instruction here");

    switch (opcode) {
      case llvm::Instruction::Alloca: {
        addPointerEquality(getNewVersionedValue(i, valueExpr),
                           getNewAllocation(i));
        break;
      }
      case llvm::Instruction::Load: {
	if (isEnvironmentAllocation(i)) {
	    // The load corresponding to a load of the environment address
	    // that was never allocated within this program.
          addPointerEquality(getNewVersionedValue(i, valueExpr),
                             getNewAllocation(i));
          break;
        }

        if (!buildLoadDependency(i->getOperand(0), i, valueExpr)) {
          Allocation *alloc = getNewAllocation(i->getOperand(0));
          updateStore(alloc, getNewVersionedValue(i, valueExpr));
        }
        break;
      }
      case llvm::Instruction::Store: {
	VersionedValue *dataArg = getLatestValue(i->getOperand(0));
        std::vector<Allocation *> addressList =
            resolveAllocationTransitively(getLatestValue(i->getOperand(1)));

        // If there was no dependency found, we should create
        // a new value
        if (!dataArg)
          dataArg = getNewVersionedValue(i->getOperand(0), valueExpr);

        for (std::vector<Allocation *>::iterator it = addressList.begin(),
                                                 itEnd = addressList.end();
             it != itEnd; ++it) {
          updateStore((*it), dataArg);
        }

        break;
      }
      case llvm::Instruction::GetElementPtr: {
	if (llvm::isa<llvm::Constant>(i->getOperand(0))) {
          Allocation *a = getLatestAllocation(i->getOperand(0));
          if (!a)
            a = getNewAllocation(i->getOperand(0));
          // We simply propagate the pointer to the current
          // value field-insensitively.
          addPointerEquality(getNewVersionedValue(i, valueExpr), a);
          break;
        }

        VersionedValue *arg = getLatestValue(i->getOperand(0));
        assert(arg != 0 && "operand not found");

        std::vector<Allocation *> a = resolveAllocationTransitively(arg);

        if (a.size() > 0) {
          VersionedValue *newValue = getNewVersionedValue(i, valueExpr);
          for (std::vector<Allocation *>::iterator it = a.begin(),
                                                   itEnd = a.end();
               it != itEnd; ++it) {
            addPointerEquality(newValue, *it);
          }
        } else {
          // Could not resolve to argument to an address,
          // simply add flow dependency
          std::vector<VersionedValue *> vec = oneStepFlowSources(arg);
          if (vec.size() > 0) {
            VersionedValue *newValue = getNewVersionedValue(i, valueExpr);
            for (std::vector<VersionedValue *>::iterator it = vec.begin(),
                                                         itEnd = vec.end();
                 it != itEnd; ++it) {
              addDependency((*it), newValue);
            }
          }
        }
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
      case llvm::Instruction::ExtractValue:
	{
	  VersionedValue *val = getLatestValue(i->getOperand(0));
	  if (val) {
            addDependency(val, getNewVersionedValue(i, valueExpr));
          } else if (!llvm::isa<llvm::Constant>(i->getOperand(0)))
              // Constants would kill dependencies, the remaining is for
              // cases that may actually require dependencies.
          {
            assert(!"operand not found");
          }
          break;
      }
      case llvm::Instruction::Select:
	{
        VersionedValue *lhs = getLatestValue(i->getOperand(1));
        VersionedValue *rhs = getLatestValue(i->getOperand(2));
        VersionedValue *newValue = 0;
        if (lhs) {
          newValue = getNewVersionedValue(i, valueExpr);
          addDependency(lhs, newValue);
        }
        if (rhs) {
          if (newValue)
            addDependency(rhs, newValue);
          else
            addDependency(rhs, getNewVersionedValue(i, valueExpr));
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
      case llvm::Instruction::InsertValue:
	{
	  VersionedValue *lhs = getLatestValue(i->getOperand(0));
	  VersionedValue *rhs = getLatestValue(i->getOperand(1));
	  VersionedValue *newValue = 0;
	  if (lhs) {
            newValue = getNewVersionedValue(i, valueExpr);
            addDependency(lhs, newValue);
          }
          if (rhs) {
            if (newValue)
              addDependency(rhs, newValue);
            else
              addDependency(rhs, getNewVersionedValue(i, valueExpr));
          }
          break;
      }
      case llvm::Instruction::PHI:
	{
	  llvm::PHINode *phi = llvm::dyn_cast<llvm::PHINode>(i);
	  for (unsigned idx = 0, b = phi->getNumIncomingValues(); idx < b; ++idx) {
	      VersionedValue *val = getLatestValue(phi->getIncomingValue(idx));
	      if (val) {
		  // We only add dependency for a single value that we could
		  // find, as this was a single execution path.
                addDependency(val, getNewVersionedValue(i, valueExpr));
                break;
              }
          }
          break;
      }
      default:
	break;
    }

  }

  void Dependency::bindCallArguments(llvm::Instruction *instr,
                                     std::vector<ref<Expr> > &arguments) {
    llvm::CallInst *site = llvm::dyn_cast<llvm::CallInst>(instr);

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
        addDependency(argumentValuesList.back(),
                      getNewVersionedValue(
                          it, argumentValuesList.back()->getExpression()));
      }
      argumentValuesList.pop_back();
      ++index;
    }
  }

  void Dependency::bindReturnValue(llvm::CallInst *site,
                                   llvm::Instruction *inst,
                                   ref<Expr> returnValue) {
    llvm::ReturnInst *retInst = llvm::dyn_cast<llvm::ReturnInst>(inst);
    if (site && retInst) {
      VersionedValue *value = getLatestValue(retInst->getReturnValue());
      if (value)
        addDependency(value, getNewVersionedValue(site, returnValue));
    }
  }

  void Dependency::markAllValues(VersionedValue *value) {
    std::vector<VersionedValue *> allSources = allFlowSources(value);
    for (std::vector<VersionedValue *>::iterator it = allSources.begin(),
                                                 itEnd = allSources.end();
         it != itEnd; ++it) {
      (*it)->includeInInterpolant();
    }
  }

  void Dependency::print(llvm::raw_ostream &stream) const {
    this->print(stream, 0);
  }

  void Dependency::print(llvm::raw_ostream &stream,
                         const unsigned int tab_num) const {
    std::string tabs = makeTabs(tab_num);
    stream << tabs << "EQUALITIES:";
    std::vector<PointerEquality *>::const_iterator equalityListBegin =
        equalityList.begin();
    std::vector<StorageCell *>::const_iterator storesListBegin =
        storesList.begin();
    std::vector<FlowsTo *>::const_iterator flowsToListBegin =
        flowsToList.begin();
    for (std::vector<PointerEquality *>::const_iterator
             it = equalityListBegin,
             itEnd = equalityList.end();
         it != itEnd; ++it) {
      if (it != equalityListBegin)
        stream << ",";
      (*it)->print(stream);
    }
    stream << "\n";
    stream << tabs << "STORAGE:";
    for (std::vector<StorageCell *>::const_iterator it = storesList.begin(),
                                                    itEnd = storesList.end();
         it != itEnd; ++it) {
      if (it != storesListBegin)
        stream << ",";
      (*it)->print(stream);
    }
    stream << "\n";
    stream << tabs << "FLOWDEPENDENCY:";
    for (std::vector<FlowsTo *>::const_iterator it = flowsToList.begin(),
                                                itEnd = flowsToList.end();
         it != itEnd; ++it) {
      if (it != flowsToListBegin)
        stream << ",";
      (*it)->print(stream);
    }

    if (parentDependency) {
      stream << "\n" << tabs << "--------- Parent Dependencies ----------\n";
      parentDependency->print(stream, tab_num);
    }
  }

  /**/

  template<typename T>
  void deletePointerVector(std::vector<T*>& list) {
    typedef typename std::vector<T*>::iterator IteratorType;

    for (IteratorType it = list.begin(),
	itEnd = list.end(); it != itEnd; ++it) {
	delete *it;
    }
    list.clear();
  }

  std::string makeTabs(const unsigned int tab_num) {
    std::string tabs_string;
    for (unsigned int i = 0; i < tab_num; i++) {
      tabs_string += appendTab(tabs_string);
    }
    return tabs_string;
  }

  std::string appendTab(const std::string &prefix) {
    return prefix + "        ";
  }

  bool isEnvironmentAllocation(llvm::Value *site) {
    llvm::LoadInst *inst = llvm::dyn_cast<llvm::LoadInst>(site);

    if (!inst)
      return false;

    llvm::Value *address = inst->getOperand(0);
    if (llvm::isa<llvm::Constant>(address) &&
	address->getName() == "__environ") {
	return true;
    }
    return false;
  }

}
