
#ifndef KLEE_DEPENDENCY_H
#define KLEE_DEPENDENCY_H

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

  class VersionedAllocation {
    static unsigned long long nextVersion;

    llvm::Value *site;
    unsigned long long version;

  public:
    VersionedAllocation(llvm::Value *site);

    ~VersionedAllocation();

    bool hasAllocationSite(llvm::Value *site);
  };

  class VersionedValue {
    static unsigned long long nextVersion;

    llvm::Value *value;
    unsigned long long version;
  public:
    VersionedValue(llvm::Value *value);

    ~VersionedValue();

    bool hasValue(llvm::Value *value);
  };

  class PointerEquality {
    // value equals allocation (pointer)
    VersionedValue* value;
    VersionedAllocation* allocation;
  public:
    PointerEquality(VersionedValue *value, VersionedAllocation *allocation);

    ~PointerEquality();

    VersionedAllocation *equals(VersionedValue *value);
  };

  class StorageCell {
    // allocation stores value
    VersionedAllocation* allocation;
    VersionedValue* value;
  public:
    StorageCell(VersionedAllocation *allocation, VersionedValue* value);

    ~StorageCell();

    VersionedValue *stores(VersionedAllocation *allocation);

    VersionedAllocation *storageOf(VersionedValue *value);
  };

  class ValueValueDependency {
    // target depends on source
    VersionedValue* source;
    VersionedValue* target;
  public:
    ValueValueDependency(VersionedValue *source, VersionedValue *target);

    ~ValueValueDependency();

    bool depends(VersionedValue *source, VersionedValue *target);
  };


  class DependencyState {
    std::vector< PointerEquality *> equalityList;
    std::vector< StorageCell *> storesList;
    std::vector< ValueValueDependency *> dependsList;
    std::vector< VersionedValue *> valuesList;
    std::vector< VersionedAllocation *> allocationsList;

    VersionedValue* getNewValue(llvm::Value *value);

    VersionedAllocation *getNewAllocation(llvm::Value *allocation);

    VersionedValue* getLatestValue(llvm::Value *value);

    VersionedAllocation *getLatestAllocation(llvm::Value *allocation);

    void addPointerEquality(VersionedValue *value, VersionedAllocation *allocation);

    void updateStore(VersionedAllocation *allocation, VersionedValue *value);

    void addDependency(VersionedValue *source, VersionedValue *target);

    VersionedAllocation *resolveAllocation(VersionedValue *value);

    VersionedValue *stores(VersionedAllocation *allocation);

    VersionedAllocation *storageOf(VersionedValue *value);

    bool depends(VersionedValue *source, VersionedValue *target);

  public:
    DependencyState();

    ~DependencyState();

    void execute(llvm::Instruction *instr);
  };
}

#endif
