
#ifndef KLEE_SYMBOLICSTATE_H
#define KLEE_SYMBOLICSTATE_H

#include "klee/Config/Version.h"

#if LLVM_VERSION_CODE > LLVM_VERSION(3, 2)
#include "llvm/IR/Function.h"
#else
#include "llvm/Function.h"
#endif
#include "llvm/Support/raw_ostream.h"

#include <vector>
#include <stack>

namespace klee {
  class LocationDependency;

  class Dependency {
    std::vector< LocationDependency * > dependencies;
    llvm::Constant *constantDependency;

  public:
    Dependency();

    ~Dependency();

    bool unInitialized();

    bool dependsOnConstant();

    void initializeWithNonConstant(std::vector<LocationDependency *> _dependencies);

    void initializeWithConstant(llvm::Constant *constant);

    std::vector<LocationDependency *> getDependencies();

    void addDependency(LocationDependency *extraDependency);

  };

  class ValueDependency : public Dependency {
    llvm::Value *value;

    ValueDependency() : value(0) {}

  public:
    ValueDependency(llvm::Value *value);
  };

  class LocationDependency : public Dependency {
    llvm::Value *allocationSite;

    LocationDependency() : allocationSite(0) {}

  public:
    LocationDependency(llvm::Value *allocationSite);
  };

  class DependencyFrame {
    llvm::Function *function;
    std::vector< ValueDependency *> valueDependencies;
    std::vector< LocationDependency *> locationDependencies;
  public:
    DependencyFrame(llvm::Function *function);

    ~DependencyFrame();

    void addLocation(llvm::Value *allocationSite);

    void updateDependency(llvm::Value *instruction);
  };

  class DependencyState {
    std::vector<DependencyFrame *> dependencyStack;
  public:
    DependencyState();

    ~DependencyState();

    void pushFrame(llvm::Function *function);

    void popFrame();

    void addLocation(llvm::Value *allocationSite);

    void updateDependency(llvm::Value *instruction);
  };

}

#endif
