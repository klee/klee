
#include "Dependency.h"

using namespace klee;

namespace klee {

  unsigned long long VersionedAllocation::nextVersion = 0;

  VersionedAllocation::VersionedAllocation(llvm::Value *site) :
      site(site), version(nextVersion++) {}

  VersionedAllocation::~VersionedAllocation() {}

  bool VersionedAllocation::hasAllocationSite(llvm::Value *site) {
    return this->site == site;
  }

  void VersionedAllocation::print(llvm::raw_ostream& stream) const {
    stream << "A[";
    site->print(stream);
    stream << "#" << version;
    stream << "]";
  }

  /**/

  unsigned long long VersionedValue::nextVersion = 0;

  VersionedValue::VersionedValue(llvm::Value *value) :
      value(value), version(nextVersion++) {}

  VersionedValue::~VersionedValue() {}

  bool VersionedValue::hasValue(llvm::Value *value) {
    return this->value == value;
  }

  void VersionedValue::print(llvm::raw_ostream& stream) const {
    stream << "V[";
    value->print(stream);
    stream << "#" << version;
    stream << "]";
  }

  /**/

  PointerEquality::PointerEquality(VersionedValue *value,
                                   VersionedAllocation *allocation) :
                                       value(value), allocation(allocation)
  {}

  PointerEquality::~PointerEquality() {}

  VersionedAllocation *PointerEquality::equals(VersionedValue *value) {
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
                           VersionedValue* value) :
                               allocation(allocation),
                               value(value) {}

  StorageCell::~StorageCell() {}

  VersionedValue *StorageCell::stores(VersionedAllocation *allocation) {
    return this->allocation == allocation ? this->value : 0;
  }

  VersionedAllocation *StorageCell::storageOf(VersionedValue *value) {
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

  bool FlowsTo::depends(VersionedValue *source, VersionedValue *target) {
    return this->source == source && this->target == target;
  }

  void FlowsTo::print(llvm::raw_ostream &stream) const {
    source->print(stream);
    stream << "->";
    target->print(stream);
  }

  /**/

  DependencyFrame::DependencyFrame(llvm::Function *function)
      : function(function) {
    llvm::errs() << "Creating a frame of function " << function->getName() << "\n";
    llvm::errs() << "Size of equalityList is " << equalityList.size() << "\n";
  }

  DependencyFrame::~DependencyFrame() {
    llvm::errs() << "Removing frame of function " << function->getName() << "\n";
    llvm::errs() << "Size of equalityList is " << equalityList.size() << "\n";

    // Delete the locally-constructed relations
    deletePointerVector(equalityList);

    llvm::errs() << "Size of storesList is " << storesList.size() << "\n";
    deletePointerVector(storesList);

    llvm::errs() << "Size of flowsToList is " << flowsToList.size() << "\n";
    deletePointerVector(flowsToList);

    // Delete the locally-constructed objects
    llvm::errs() << "Size of valuesList is " << valuesList.size() << "\n";
    deletePointerVector(valuesList);

    llvm::errs() << "Size of allocationsToList is " << allocationsList.size() << "\n";
    deletePointerVector(allocationsList);

    llvm::errs() << "Done removing all data\n";
  }

  VersionedValue *DependencyFrame::getNewValue(llvm::Value *value) {
    VersionedValue *ret = new VersionedValue(value);
    valuesList.push_back(ret);
    return ret;
  }

  VersionedAllocation *DependencyFrame::getNewAllocation(llvm::Value *allocation) {
    VersionedAllocation *ret = new VersionedAllocation(allocation);
    allocationsList.push_back(ret);
    return ret;
  }

  VersionedValue *DependencyFrame::getLatestValue(llvm::Value *value) {
    for (std::vector< VersionedValue *>::reverse_iterator
	it = valuesList.rbegin(),
	itEnd = valuesList.rend(); it != itEnd; ++it) {
	if ((*it)->hasValue(value))
	  return *it;
    }
    return 0;
  }

  VersionedAllocation *DependencyFrame::getLatestAllocation(llvm::Value *allocation) {
    for (std::vector< VersionedAllocation *>::reverse_iterator
	it = allocationsList.rbegin(),
	itEnd = allocationsList.rend(); it != itEnd; ++it) {
	if ((*it)->hasAllocationSite(allocation))
	  return *it;
    }
    return 0;
  }

  VersionedAllocation *DependencyFrame::resolveAllocation(VersionedValue *val) {
    if (!val) return 0;
    for (std::vector< PointerEquality *>::reverse_iterator
	it = equalityList.rbegin(),
	itEnd = equalityList.rend(); it != itEnd; ++it) {
	VersionedAllocation *alloc = (*it)->equals(val);
	if (alloc)
	  return alloc;
    }
    return 0;
  }

  void DependencyFrame::addPointerEquality(VersionedValue *value,
                                           VersionedAllocation *allocation) {
    equalityList.push_back(new PointerEquality(value, allocation));
  }

  void DependencyFrame::updateStore(VersionedAllocation *allocation,
                                    VersionedValue *value) {
    storesList.push_back(new StorageCell(allocation, value));
  }

  void DependencyFrame::addDependency(VersionedValue *source,
                                      VersionedValue *target) {
    flowsToList.push_back(new FlowsTo(source, target));
  }

  VersionedValue *DependencyFrame::stores(VersionedAllocation *allocation) {
    for (std::vector< StorageCell *>::iterator it = storesList.begin(),
	itEnd = storesList.end(); it != itEnd; ++it) {
	VersionedValue *ret = (*it)->stores(allocation);
	if (ret)
	  return ret;
    }
    return 0;
  }

  VersionedAllocation *DependencyFrame::storageOf(VersionedValue *value) {
    for (std::vector< StorageCell *>::iterator it = storesList.begin(),
	itEnd = storesList.end(); it != itEnd; ++it) {
	VersionedAllocation *ret = (*it)->storageOf(value);
	if (!ret)
	  return ret;
    }
    return 0;

  }

  bool DependencyFrame::depends(VersionedValue *source,
                                VersionedValue *target) {
    for (std::vector<FlowsTo *>::iterator it = flowsToList.begin(),
                                          itEnd = flowsToList.end();
         it != itEnd; ++it) {
      if ((*it)->depends(source, target))
        return true;
    }
    return false;
  }

  void DependencyFrame::execute(llvm::Instruction *i) {
    switch(i->getOpcode()) {
      case llvm::Instruction::Alloca: {
	addPointerEquality(getNewValue(i), getNewAllocation(i));
	break;
      }
      case llvm::Instruction::Load: {
	VersionedValue *arg = getLatestValue(i->getOperand(0));
	if (arg) {
	    VersionedAllocation *alloc = resolveAllocation(arg);
	    if (alloc) {
		VersionedValue *val = stores(alloc);
		if (val) {
		    VersionedAllocation *alloc2 = resolveAllocation(val);
		    if (alloc2) {
			addPointerEquality(getNewValue(i), alloc2);
		    } else {
			addDependency(val, getNewValue(i));
		    }
		} else {
		    // We could not find the stored value, create
		    // a new one.
		    updateStore(alloc, getNewValue(i));
		}
	    }
	}
	break;
      }
      case llvm::Instruction::Store: {
	VersionedValue *dataArg = getLatestValue(i->getOperand(0));
	VersionedAllocation *address =
	    resolveAllocation(getLatestValue(i->getOperand(1)));

	if (dataArg) {
	    // There is dependency to store (not a constant etc.)
	    if (address) {
		updateStore(address, dataArg);
	    }
	} else {
	    // There is no dependency found, we should create
	    // a new value
	    if (address) {
		updateStore(address, getNewValue(i->getOperand(0)));
	    }
	}
	break;
      }
      case llvm::Instruction::GetElementPtr: {
	VersionedValue *dataArg = getLatestValue(i->getOperand(0));
	if (dataArg) {
	    VersionedAllocation *alloc = storageOf(dataArg);
	    if (alloc) {
		addPointerEquality(getNewValue(i), alloc);
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
	      VersionedAllocation *alloc = resolveAllocation(val);
	      if (alloc) {
		  addPointerEquality(getNewValue(i), alloc);
	      } else {
		  addDependency(val, getNewValue(i));
	      }
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
      case llvm::Instruction::Invoke:
      case llvm::Instruction::Call:
	{
	  break;
	}
      case llvm::Instruction::Ret:
	{
	  break;
	}
      default:
	break;
    }

  }

  void DependencyFrame::print(llvm::raw_ostream &stream,
                              const unsigned int tab_num) const {
    std::string tabs = makeTabs(tab_num);
    stream << tabs << (function ? function->getName() : "TOP") << " FRAME\n";
    stream << tabs << "EQUALITIES:";
    std::vector<PointerEquality *>::const_iterator equalityListBegin =
	equalityList.begin();
    std::vector<StorageCell *>::const_iterator storesListBegin =
	storesList.begin();
    std::vector<FlowsTo *>::const_iterator flowsToListBegin =
	flowsToList.begin();
    for (std::vector< PointerEquality *>::const_iterator it = equalityListBegin,
	itEnd = equalityList.end(); it != itEnd; ++it) {
	if (it != equalityListBegin)
	  stream << ",";
	(*it)->print(stream);
    }
    stream << "\n";
    stream << tabs << "STORAGE:";
    for (std::vector< StorageCell *>::const_iterator it = storesList.begin(),
	itEnd = storesList.end(); it != itEnd; ++it) {
	if (it != storesListBegin)
	  stream << ",";
	(*it)->print(stream);
    }
    stream << "\n";
    stream << tabs << "FLOWDEPENDENCY:";
    for (std::vector<FlowsTo *>::const_iterator it = flowsToList.begin(),
                                                itEnd = flowsToList.begin();
         it != itEnd; ++it) {
	if (it != flowsToListBegin)
	  stream << ",";
	(*it)->print(stream);
    }
  }

  void DependencyFrame::print(llvm::raw_ostream &stream) const {
    this->print(stream, 0);
  }

  /**/

  DependencyStack::DependencyStack(llvm::Function *function,
                                   DependencyStack *prev) :
                                       frame(new DependencyFrame(function)),
                                       tail(prev) {}

  DependencyStack::~DependencyStack() {
    delete frame;
  }

  DependencyFrame *DependencyStack::car() const {
    return frame;
  }

  DependencyStack* DependencyStack::cdr() const {
    return tail;
  }

  void DependencyStack::print(llvm::raw_ostream& stream) const {
    frame->print(stream);
    llvm::errs() << "\n----------------------------------------\n";
    tail->print(stream);
  }

  /**/

  DependencyState::DependencyState() {
    llvm::errs() << "MAKING A NEW DEPENDENCY STATE\n";
  }

  DependencyState::~DependencyState() {
    deletePointerVector(stack);
  }

  void DependencyState::execute(llvm::Instruction *instr) {
    stack.back()->execute(instr);
  }

  void DependencyState::pushFrame(llvm::Function *function) {
    stack.push_back(new DependencyFrame(function));
  }

  void DependencyState::popFrame() {
    delete stack.back();
    llvm::errs() << "PopFrame\n";
    stack.pop_back();
    llvm::errs() << "Popped frame\n";
  }

  void DependencyState::print(llvm::raw_ostream &stream,
                              const unsigned int tab_num) const {
    for (std::vector<DependencyFrame *>::const_iterator it = stack.begin(),
	itEnd = stack.end(); it != itEnd; ++it) {
      (*it)->print(stream, tab_num);
      stream << "\n";
    }
  }

  void DependencyState::print(llvm::raw_ostream &stream) const {
    this->print(stream, 0);
  }

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
}
