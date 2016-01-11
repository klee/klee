
#include "Dependency.h"

using namespace klee;

namespace klee {

  unsigned long long VersionedAllocation::nextVersion = 0;

  VersionedAllocation::VersionedAllocation()
      : version(0), site(0) {}

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

  bool EnvironmentAllocation::isComposite() const {
    return true;
  }

  void EnvironmentAllocation::print(llvm::raw_ostream& stream) const {
    stream << "A[@__environ]" << this;
  }

  /**/

  unsigned long long VersionedValue::nextVersion = 0;

  VersionedValue::VersionedValue(llvm::Value *value)
      : value(value), version(nextVersion++), inInterpolant(false) {}

  VersionedValue::~VersionedValue() {}

  bool VersionedValue::hasValue(llvm::Value *value) const {
    return this->value == value;
  }

  void VersionedValue::includeInInterpolant() {
    inInterpolant = true;
  }

  bool VersionedValue::valueInInterpolant() const {
    return inInterpolant;
  }

  void VersionedValue::print(llvm::raw_ostream& stream) const {
    stream << "V[";
    value->print(stream);
    stream << "#" << version;
    stream << "]";
  }

  /**/

  PointerEquality::PointerEquality(VersionedValue *value,
                                   VersionedAllocation *allocation)
      : value(value), allocation(allocation) {}

  PointerEquality::~PointerEquality() {}

  VersionedAllocation *PointerEquality::equals(VersionedValue *value) const {
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

  StorageCell::StorageCell(VersionedAllocation *allocation,
                           VersionedValue *value)
      : allocation(allocation), value(value) {}

  StorageCell::~StorageCell() {}

  VersionedValue *StorageCell::stores(VersionedAllocation *allocation) const {
    return this->allocation == allocation ? this->value : 0;
  }

  VersionedAllocation *StorageCell::storageOf(VersionedValue *value) const {
    return this->value == value ? this->allocation : 0;
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

  VersionedValue *Dependency::getNewValue(llvm::Value *value) {
    VersionedValue *ret = new VersionedValue(value);
    valuesList.push_back(ret);
    return ret;
  }

  VersionedAllocation *Dependency::getNewAllocation(llvm::Value *allocation) {
    VersionedAllocation *ret;
    if (isEnvironmentAllocation(allocation)) {
      ret = getLatestAllocation(allocation);
      if (!ret) {
	ret = new EnvironmentAllocation();
	allocationsList.push_back(ret);
      }
    } else {
	ret = new VersionedAllocation(allocation);
	allocationsList.push_back(ret);
    }
    return ret;
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

  VersionedAllocation *
  Dependency::getLatestAllocation(llvm::Value *allocation) const {
    for (std::vector<VersionedAllocation *>::const_reverse_iterator
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

  VersionedAllocation *
  Dependency::resolveAllocation(VersionedValue *val) const {
    if (!val) return 0;
    for (std::vector< PointerEquality *>::const_reverse_iterator
	it = equalityList.rbegin(),
	itEnd = equalityList.rend(); it != itEnd; ++it) {
	VersionedAllocation *alloc = (*it)->equals(val);
	if (alloc)
	  return alloc;
    }

    if (parentDependency)
      return parentDependency->resolveAllocation(val);

    return 0;
  }

  std::vector<VersionedAllocation *>
  Dependency::resolveAllocationTransitively(VersionedValue *value) const {
    std::vector<VersionedAllocation *> ret;
    VersionedAllocation *singleRet = resolveAllocation(value);
    if (singleRet) {
	ret.push_back(singleRet);
	return ret;
    }

    std::vector<VersionedValue *> valueSources = oneStepFlowSources(value);
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
                                      VersionedAllocation *allocation) {
    equalityList.push_back(new PointerEquality(value, allocation));
  }

  void Dependency::updateStore(VersionedAllocation *allocation,
                               VersionedValue *value) {
    storesList.push_back(new StorageCell(allocation, value));
  }

  void Dependency::addDependency(VersionedValue *source,
                                 VersionedValue *target) {
    flowsToList.push_back(new FlowsTo(source, target));
  }

  std::vector<VersionedValue *>
  Dependency::stores(VersionedAllocation *allocation) const {
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
  Dependency::populateArgumentValuesList(llvm::CallInst *site) {
    unsigned numArgs = site->getCalledFunction()->arg_size();
    std::vector<VersionedValue *> argumentValuesList;
    for (unsigned i = numArgs; i > 0;) {
      llvm::Value *argOperand = site->getArgOperand(--i);
      VersionedValue *latestValue = getLatestValue(argOperand);
      argumentValuesList.push_back(latestValue ? latestValue
                                               : getNewValue(argOperand));
    }
    return argumentValuesList;
  }

  bool Dependency::buildLoadDependency(llvm::Value *fromValue,
                                       llvm::Value *toValue) {
    VersionedValue *arg = getLatestValue(fromValue);
    if (!arg)
      return false;

    std::vector<VersionedAllocation *> allocList = resolveAllocationTransitively(arg);
    if (allocList.size() > 0) {
	for (std::vector<VersionedAllocation *>::iterator it0 = allocList.begin(),
	    it0End = allocList.end(); it0 != it0End; ++it0) {
	    std::vector<VersionedValue *> valList = stores(*it0);
	    if (valList.size() > 0) {
		for (std::vector<VersionedValue *>::iterator it1 = valList.begin(),
		    it1End = valList.end();
		    it1 != it1End; ++it1) {
		    std::vector<VersionedAllocation *> alloc2 = resolveAllocationTransitively(*it1);
		    if (alloc2.size() > 0) {
			for (std::vector<VersionedAllocation *>::iterator it2 = alloc2.begin(),
			    it2End = alloc2.end(); it2 != it2End; ++it2) {
			    addPointerEquality(getNewValue(toValue), *it2);
			}
		    } else {
			addDependency(*it1, getNewValue(toValue));
		    }
		}
	    } else {
		// We could not find the stored value, create
		// a new one.
		updateStore(*it0, getNewValue(toValue));
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

  void Dependency::execute(llvm::Instruction *i) {
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
        addPointerEquality(getNewValue(i), getNewAllocation(i));
        break;
      }
      case llvm::Instruction::Load: {
	if (isEnvironmentAllocation(i)) {
	    // The load corresponding to a load of the environment address
	    // that was never allocated within this program.
	    addPointerEquality(getNewValue(i), getNewAllocation(i));
	    break;
	}

        if (!buildLoadDependency(i->getOperand(0), i)) {
          VersionedAllocation *alloc = getNewAllocation(i->getOperand(0));
          updateStore(alloc, getNewValue(i));
        }
        break;
      }
      case llvm::Instruction::Store: {
	VersionedValue *dataArg = getLatestValue(i->getOperand(0));
	std::vector<VersionedAllocation *> addressList =
	    resolveAllocationTransitively(getLatestValue(i->getOperand(1)));

	// If there was no dependency found, we should create
	// a new value
	if (!dataArg)
	  dataArg = getNewValue(i->getOperand(0));

	for (std::vector<VersionedAllocation *>::iterator it = addressList.begin(),
	    itEnd = addressList.end(); it != itEnd; ++it) {
	    updateStore((*it), dataArg);
	}

	break;
      }
      case llvm::Instruction::GetElementPtr: {
	if (llvm::isa<llvm::Constant>(i->getOperand(0))) {
	    VersionedAllocation *a = getLatestAllocation(i->getOperand(0));
	    if (!a)
	      a = getNewAllocation(i->getOperand(0));
	    // We simply propagate the pointer to the current
	    // value field-insensitively.
	    addPointerEquality(getNewValue(i), a);
	    break;
	}

	VersionedValue *arg = getLatestValue(i->getOperand(0));
	assert(arg != 0 && "operand not found");

	std::vector<VersionedAllocation *> a =
	    resolveAllocationTransitively(arg);

	if (a.size() > 0) {
	    VersionedValue *newValue = getNewValue(i);
	    for (std::vector<VersionedAllocation *>::iterator it = a.begin(),
		itEnd = a.end(); it != itEnd; ++it) {
		addPointerEquality(newValue, *it);
	    }
	} else {
	    // Could not resolve to argument to an address,
	    // simply add flow dependency
	    std::vector<VersionedValue *> vec = oneStepFlowSources(arg);
	    if (vec.size() > 0) {
		VersionedValue *newValue = getNewValue(i);
		for (std::vector<VersionedValue *>::iterator it = vec.begin(),
		    itEnd = vec.end(); it != itEnd; ++it) {
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
	      addDependency(getNewValue(i), val);
	  } else if (!llvm::isa<llvm::Constant>(i->getOperand(0)))
	    // Constants would kill dependencies, the remaining is for
	    // cases that may actually require dependencies.
	    {
	      assert (!"operand not found");
	    }
	  break;
      }
      case llvm::Instruction::Select:
	{
        VersionedValue *lhs = getLatestValue(i->getOperand(1));
        VersionedValue *rhs = getLatestValue(i->getOperand(2));
        VersionedValue *newValue = 0;
        if (lhs) {
          newValue = getNewValue(i);
          addDependency(lhs, newValue);
        }
        if (rhs) {
          if (newValue)
            addDependency(rhs, newValue);
          else
            addDependency(rhs, getNewValue(i));
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
	      newValue = getNewValue(i);
	      addDependency(lhs, newValue);
	  }
	  if (rhs) {
	      if (newValue)
		addDependency(rhs, newValue);
	      else
		addDependency(rhs, getNewValue(i));
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
		  addDependency(val, getNewValue(i));
		  break;
	      }
	  }
	  break;
	}
      default:
	break;
    }

  }

  void Dependency::bindCallArguments(llvm::Instruction *instr) {
    llvm::CallInst *site = llvm::dyn_cast<llvm::CallInst>(instr);

    if (!site)
      return;

    llvm::Function *callee = site->getCalledFunction();

    // Sometimes the callee information is missing, in which case
    // the calle is not to be symbolically tracked.
    if (!callee)
      return;

    argumentValuesList = populateArgumentValuesList(site);
    unsigned index = 0;
    for (llvm::Function::ArgumentListType::iterator
             it = callee->getArgumentList().begin(),
             itEnd = callee->getArgumentList().end();
         it != itEnd; ++it) {
      addDependency(argumentValuesList.back(), getNewValue(it));
      argumentValuesList.pop_back();
      ++index;
    }
  }

  void Dependency::bindReturnValue(llvm::CallInst *site,
                                   llvm::Instruction *inst) {
    llvm::ReturnInst *retInst = llvm::dyn_cast<llvm::ReturnInst>(inst);
    if (site && retInst) {
      VersionedValue *value = getLatestValue(retInst->getReturnValue());
      if (value)
        addDependency(value, getNewValue(site));
    }
  }

  void Dependency::markAllValues(VersionedValue *value) {
    value->includeInInterpolant();

    for (std::vector<FlowsTo *>::iterator it = flowsToList.begin(),
	itEnd = flowsToList.end(); it != itEnd; ++it) {

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
