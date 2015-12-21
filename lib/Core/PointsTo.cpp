
#include "PointsTo.h"
#include "klee/Config/Version.h"

#if LLVM_VERSION_CODE > LLVM_VERSION(3, 2)
#include "llvm/IR/Function.h"
#else
#include "llvm/Function.h"
#endif
#include "llvm/Support/raw_ostream.h"


using namespace klee;

namespace klee {

  Dependency::Dependency() :
      constantDependency(0) {}

  Dependency::~Dependency() { dependencies.clear(); }

  bool Dependency::unInitialized() {
    return (dependencies.size() == 0 && constantDependency == 0);
  }

  bool Dependency::dependsOnConstant() {
    return (dependencies.size() == 0 && constantDependency != 0);
  }

  void Dependency::initializeWithNonConstant(
      std::vector<LocationDependency *> _dependencies) {
    assert(_dependencies.size() != 0);
    constantDependency = 0;
    dependencies.clear();
    dependencies.insert(dependencies.begin(),
                        _dependencies.begin(), _dependencies.end());
  }

  void Dependency::initializeWithConstant(llvm::Constant *constant) {
    dependencies.clear();
    constantDependency = constant;
  }

  std::vector<LocationDependency *> Dependency::getDependencies() {
    return dependencies;
  }

  void Dependency::addDependency(LocationDependency *extraDependency) {
    if (dependsOnConstant() || unInitialized()) {
	if (extraDependency->dependsOnConstant()) {
	    initializeWithConstant(extraDependency->constantDependency);
	} else {
	    initializeWithNonConstant(extraDependency->dependencies);
	}
	return;
    }

    dependencies.insert(dependencies.begin(),
                        extraDependency->dependencies.begin(),
                        extraDependency->dependencies.end());
  }

  ValueDependency::ValueDependency(llvm::Value *value) :
      value(value) {}

  LocationDependency::LocationDependency(llvm::Value *allocationSite) :
      allocationSite(allocationSite) {}

  DependencyFrame::DependencyFrame(llvm::Function *function) :
      function(function) {}

  DependencyFrame::~DependencyFrame() {
    valueDependencies.clear();
    locationDependencies.clear();
  }

  void DependencyFrame::addLocation(llvm::Value *allocationSite) {
    locationDependencies.push_back(new LocationDependency(allocationSite));
  }

  void DependencyFrame::updateDependency(llvm::Value *instruction) {
    llvm::Instruction *i = llvm::dyn_cast<llvm::Instruction>(instruction);
    switch(i->getOpcode()) {
      case llvm::Instruction::Alloca: {
	LocationDependency *newLocationDependency =
	    new LocationDependency(instruction);
	ValueDependency *newValueDependency =
	    new ValueDependency(instruction);
	std::vector<LocationDependency *> locationDependencyVector;
	locationDependencyVector.push_back(newLocationDependency);
	newValueDependency->initializeWithNonConstant(
	    locationDependencyVector);
	locationDependencies.push_back(newLocationDependency);
	valueDependencies.push_back(newValueDependency);
	break;
      }
      case llvm::Instruction::Load: {
	llvm::Value *address = i->getOperand(0);
	break;
      }
      case llvm::Instruction::Store: {
	ValueDependency *addressDependency =
	    new ValueDependency(i->getOperand(0));
	ValueDependency *valueDependency =
	    new ValueDependency(i->getOperand(1));
	for (std::vector<ValueDependency *>::iterator
	    it = valueDependencies.begin(),
	    endIt = valueDependencies.end(); it != endIt; ++it) {

	}
	break;
      }
      default:
	break;
    }
  }

  DependencyState::DependencyState() {
    dependencyStack.push_back(new DependencyFrame(0));
  }

  DependencyState::~DependencyState() {
    dependencyStack.clear();
  }

  void DependencyState::pushFrame(llvm::Function *function) {
    dependencyStack.push_back(new DependencyFrame(function));
  }

  void DependencyState::popFrame() {
    dependencyStack.pop_back();
  }

  void DependencyState::addLocation(llvm::Value *allocationSite) {
    dependencyStack.back()->addLocation(allocationSite);
  }

  void DependencyState::updateDependency(llvm::Value *instruction) {
    dependencyStack.back()->updateDependency(instruction);
  }

}
