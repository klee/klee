
#ifndef KLEE_DEPENDENCY_H
#define KLEE_DEPENDENCY_H

#include "klee/Config/Version.h"

#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 3)
#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Value.h>
#else
#include <llvm/Function.h>
#include <llvm/Instruction.h>
#include <llvm/Instructions.h>
#include <llvm/Value.h>
#endif

#include "llvm/Support/raw_ostream.h"

#include <vector>
#include <stack>

namespace klee {

  class VersionedAllocation {
    static unsigned long long nextVersion;
    unsigned long long version;

  protected:
    llvm::Value *site;

    VersionedAllocation();

  public:
    VersionedAllocation(llvm::Value *site);

    virtual ~VersionedAllocation();

    virtual bool hasAllocationSite(llvm::Value *site) const;

    virtual bool isComposite() const;

    virtual void print(llvm::raw_ostream& stream) const;

    void dump() const {
      print(llvm::errs());
      llvm::errs() << "\n";
    }
  };

  class EnvironmentAllocation : public VersionedAllocation {
  public:
    EnvironmentAllocation();

    ~EnvironmentAllocation();

    bool hasAllocationSite(llvm::Value *site) const;

    bool isComposite() const;

    void print(llvm::raw_ostream& stream) const;
  };

  class VersionedValue {
    static unsigned long long nextVersion;

    llvm::Value *value;
    unsigned long long version;

    /// @brief to indicate if any unsatisfiability core
    /// depends on this value
    bool inInterpolant;
  public:
    VersionedValue(llvm::Value *value);

    ~VersionedValue();

    bool hasValue(llvm::Value *value) const;

    void includeInInterpolant();

    bool valueInInterpolant() const;

    void print(llvm::raw_ostream& stream) const;

    void dump() const {
      print(llvm::errs());
      llvm::errs() << "\n";
    }
  };

  class PointerEquality {
    // value equals allocation (pointer)
    VersionedValue* value;
    VersionedAllocation* allocation;
  public:
    PointerEquality(VersionedValue *value, VersionedAllocation *allocation);

    ~PointerEquality();

    VersionedAllocation *equals(VersionedValue *value) const;

    void print(llvm::raw_ostream& stream) const;

    void dump() const {
      print(llvm::errs());
      llvm::errs() << "\n";
    }
  };

  class StorageCell {
    // allocation stores value
    VersionedAllocation* allocation;
    VersionedValue* value;
  public:
    StorageCell(VersionedAllocation *allocation, VersionedValue* value);

    ~StorageCell();

    VersionedValue *stores(VersionedAllocation *allocation) const;

    VersionedAllocation *storageOf(VersionedValue *value) const;

    void print(llvm::raw_ostream& stream) const;

    void dump() const {
      print(llvm::errs());
      llvm::errs() << "\n";
    }
  };

  class FlowsTo {
    // target depends on source
    VersionedValue* source;
    VersionedValue* target;
  public:
    FlowsTo(VersionedValue *source, VersionedValue *target);

    ~FlowsTo();

    VersionedValue *getSource() const;

    VersionedValue *getTarget() const;

    void print(llvm::raw_ostream& sream) const;

    void dump() const {
      print(llvm::errs());
      llvm::errs() << "\n";
    }
  };

  class Dependency {

    /// @brief Previous path condition
    Dependency *parentDependency;

    /// @brief Argument values to be passed onto callee
    std::vector<VersionedValue *> argumentValuesList;

    std::vector< PointerEquality *> equalityList;

    std::vector< StorageCell *> storesList;

    std::vector<FlowsTo *> flowsToList;

    std::vector< VersionedValue *> valuesList;

    std::vector< VersionedAllocation *> allocationsList;

    VersionedValue* getNewValue(llvm::Value *value);

    VersionedAllocation *getNewAllocation(llvm::Value *allocation);

    VersionedAllocation *getLatestAllocation(llvm::Value *allocation) const;

    void addPointerEquality(VersionedValue *value,
                            VersionedAllocation *allocation);

    void updateStore(VersionedAllocation *allocation, VersionedValue *value);

    void addDependency(VersionedValue *source, VersionedValue *target);

    VersionedAllocation *resolveAllocation(VersionedValue *value) const;

    std::vector<VersionedAllocation *> resolveAllocationTransitively(VersionedValue *value) const;

    std::vector<VersionedValue *> stores(VersionedAllocation *allocation) const;

    std::vector<VersionedValue *> locallyFlowsFrom(VersionedValue *target) const;

    std::vector<VersionedValue *> flowsFrom(VersionedValue *target) const;

    std::vector<VersionedValue *>
    populateArgumentValuesList(llvm::CallInst *site);

    /// @brief Construct dependency due to load instruction
    bool buildLoadDependency(llvm::Value *fromValue, llvm::Value *toValue);

  public:
    Dependency(Dependency *prev);

    ~Dependency();

    Dependency *cdr() const;

    void execute(llvm::Instruction *instr);

    VersionedValue *getLatestValue(llvm::Value *value) const;

    void bindCallArguments(llvm::Instruction *instr);

    void bindReturnValue(llvm::CallInst *site, llvm::Instruction *inst);

    void markAllValues(VersionedValue *value);

    void dump() const {
      this->print(llvm::errs());
      llvm::errs() << "\n";
    }

    void print(llvm::raw_ostream& stream) const;

    void print(llvm::raw_ostream& stream, const unsigned tab_num) const;

  };

  template<typename T>
  void deletePointerVector(std::vector<T*>& list);

  std::string makeTabs(const unsigned tab_num);

  std::string appendTab(const std::string &prefix);

  bool isEnvironmentAllocation(llvm::Value *site);
}

#endif
