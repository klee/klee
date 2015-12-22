
#include "Dependency.h"
#include "klee/Config/Version.h"

#if LLVM_VERSION_CODE > LLVM_VERSION(3, 2)
#include "llvm/IR/Function.h"
#else
#include "llvm/Function.h"
#endif
#include "llvm/Support/raw_ostream.h"


using namespace klee;

namespace klee {

  unsigned long long VersionedAllocation::nextVersion = 0;

  VersionedAllocation::VersionedAllocation(llvm::Value *site) :
      site(site), version(nextVersion++) {}

  VersionedAllocation::~VersionedAllocation() {}

  bool VersionedAllocation::hasAllocationSite(llvm::Value *site) {
    return this->site == site;
  }

  /**/

  unsigned long long VersionedValue::nextVersion = 0;

  VersionedValue::VersionedValue(llvm::Value *value) :
      value(value), version(nextVersion++) {}

  VersionedValue::~VersionedValue() {}

  bool VersionedValue::hasValue(llvm::Value *value) {
    return this->value == value;
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

  /**/

  ValueValueDependency::ValueValueDependency(VersionedValue *source,
                                             VersionedValue *target) :
                                        	 source(source), target(target)
  {}

  ValueValueDependency::~ValueValueDependency() {}

  bool ValueValueDependency::depends(VersionedValue *source,
                                     VersionedValue *target) {
    return this->source == source && this->target == target;
  }

  /**/

  DependencyState::DependencyState() {}

  DependencyState::~DependencyState() {
    equalityList.clear();
    dependsList.clear();
    storesList.clear();
    valuesList.clear();
    allocationsList.clear();
  }

  VersionedValue *DependencyState::getNewValue(llvm::Value *value) {
    VersionedValue *ret = new VersionedValue(value);
    valuesList.push_back(ret);
    return ret;
  }

  VersionedAllocation *DependencyState::getNewAllocation(llvm::Value *allocation) {
    VersionedAllocation *ret = new VersionedAllocation(allocation);
    allocationsList.push_back(ret);
    return ret;
  }

  VersionedValue *DependencyState::getLatestValue(llvm::Value *value) {
    for (std::vector< VersionedValue *>::reverse_iterator
	it = valuesList.rbegin(),
	itEnd = valuesList.rend(); it != itEnd; ++it) {
	if ((*it)->hasValue(value))
	  return *it;
    }
    return 0;
  }

  VersionedAllocation *DependencyState::getLatestAllocation(llvm::Value *allocation) {
    for (std::vector< VersionedAllocation *>::reverse_iterator
	it = allocationsList.rbegin(),
	itEnd = allocationsList.rend(); it != itEnd; ++it) {
	if ((*it)->hasAllocationSite(allocation))
	  return *it;
    }
    return 0;
  }

  VersionedAllocation *DependencyState::resolveAllocation(VersionedValue *val) {
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

  void DependencyState::addPointerEquality(VersionedValue *value,
                                           VersionedAllocation *allocation) {
    equalityList.push_back(new PointerEquality(value, allocation));
  }

  void DependencyState::updateStore(VersionedAllocation *allocation,
                                    VersionedValue *value) {
    storesList.push_back(new StorageCell(allocation, value));
  }

  void DependencyState::addDependency(VersionedValue *source,
                                      VersionedValue *target) {
    dependsList.push_back(new ValueValueDependency(source, target));
  }

  VersionedValue *DependencyState::stores(VersionedAllocation *allocation) {
    for (std::vector< StorageCell *>::iterator it = storesList.begin(),
	itEnd = storesList.end(); it != itEnd; ++it) {
	VersionedValue *ret = (*it)->stores(allocation);
	if (ret)
	  return ret;
    }
    return 0;
  }

  VersionedAllocation *DependencyState::storageOf(VersionedValue *value) {
    for (std::vector< StorageCell *>::iterator it = storesList.begin(),
	itEnd = storesList.end(); it != itEnd; ++it) {
	VersionedAllocation *ret = (*it)->storageOf(value);
	if (!ret)
	  return ret;
    }
    return 0;

  }

  bool DependencyState::depends(VersionedValue *source,
                                VersionedValue *target) {
    for (std::vector< ValueValueDependency *>::iterator it = dependsList.begin(),
	itEnd = dependsList.end(); it != itEnd; ++it) {
	if ((*it)->depends(source, target))
	  return true;
    }
    return false;
  }

  void DependencyState::execute(llvm::Instruction *i) {
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
		}
	    }
	}
	break;
      }
      case llvm::Instruction::Store: {
	VersionedValue *dataArg = getLatestValue(i->getOperand(0));
	if (dataArg) {
	    // There is dependency to store (not a constant etc.)
	    VersionedValue *addressArg = getLatestValue(i->getOperand(1));
	    VersionedAllocation *address = resolveAllocation(addressArg);

	    if (address) {
		updateStore(address, dataArg);
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
      default:
	break;
    }

  }

}
