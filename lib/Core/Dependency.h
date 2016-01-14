
#ifndef KLEE_DEPENDENCY_H
#define KLEE_DEPENDENCY_H

#include "AddressSpace.h"
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

class Allocation {

  protected:
    llvm::Value *site;

    Allocation();

  public:
    virtual ~Allocation();

    virtual bool hasAllocationSite(llvm::Value *site) const;

    virtual bool isComposite() const;

    virtual void print(llvm::raw_ostream& stream) const;

    void dump() const {
      print(llvm::errs());
      llvm::errs() << "\n";
    }
  };

  class CompositeAllocation : public Allocation {
  public:
    CompositeAllocation(llvm::Value *site);

    ~CompositeAllocation();

    bool hasAllocationSite(llvm::Value *site) const;

    void print(llvm::raw_ostream &stream) const;
  };

  class VersionedAllocation : public Allocation {
    static unsigned long long nextVersion;
    unsigned long long version;

  protected:
    llvm::Value *site;

  public:
    VersionedAllocation(llvm::Value *site);

    ~VersionedAllocation();

    bool hasAllocationSite(llvm::Value *site) const;

    bool isComposite() const;

    void print(llvm::raw_ostream& stream) const;
  };

  class EnvironmentAllocation : public Allocation {
  public:
    EnvironmentAllocation();

    ~EnvironmentAllocation();

    bool hasAllocationSite(llvm::Value *site) const;

    void print(llvm::raw_ostream &stream) const;
  };

  class VersionedValue {
    static unsigned long long nextVersion;

    llvm::Value *value;

    ref<Expr> valueExpr;

    unsigned long long version;

    /// @brief to indicate if any unsatisfiability core
    /// depends on this value
    bool inInterpolant;
  public:
    VersionedValue(llvm::Value *value, ref<Expr> valueExpr);

    ~VersionedValue();

    bool hasValue(llvm::Value *value) const;

    ref<Expr> getExpression() const;

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
    Allocation *allocation;

  public:
    PointerEquality(VersionedValue *value, Allocation *allocation);

    ~PointerEquality();

    Allocation *equals(VersionedValue *value) const;

    void print(llvm::raw_ostream& stream) const;

    void dump() const {
      print(llvm::errs());
      llvm::errs() << "\n";
    }
  };

  class StorageCell {
    // allocation stores value
    Allocation *allocation;
    VersionedValue* value;
  public:
    StorageCell(Allocation *allocation, VersionedValue *value);

    ~StorageCell();

    VersionedValue *stores(Allocation *allocation) const;

    Allocation *storageOf(VersionedValue *value) const;

    Allocation *getAllocation() const;

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

    std::vector<Allocation *> allocationsList;

    std::vector<llvm::Value *> newVersionedAllocations;

    VersionedValue *getNewVersionedValue(llvm::Value *value,
                                         ref<Expr> valueExpr);

    Allocation *getNewAllocation(llvm::Value *allocation);

    std::vector<llvm::Value *> getAllVersionedAllocations() const;

    std::vector< std::pair<llvm::Value *, ref<Expr> > > getLatestCoreExpressions() const;

    std::vector< std::pair<llvm::Value *, std::vector<ref<Expr> > > > getCompositeCoreExpressions() const;

    Allocation *getLatestAllocation(llvm::Value *allocation) const;

    void addPointerEquality(VersionedValue *value, Allocation *allocation);

    void updateStore(Allocation *allocation, VersionedValue *value);

    void addDependency(VersionedValue *source, VersionedValue *target);

    Allocation *resolveAllocation(VersionedValue *value) const;

    std::vector<Allocation *>
    resolveAllocationTransitively(VersionedValue *value) const;

    std::vector<VersionedValue *> stores(Allocation *allocation) const;

    /// @brief All values that flows to the target in one step, local
    /// to the current dependency / interpolation tree node
    std::vector<VersionedValue *> oneStepLocalFlowSources(VersionedValue *target) const;

    /// @brief All values that flows to the target in one step
    std::vector<VersionedValue *> oneStepFlowSources(VersionedValue *target) const;

    /// @brief All values that could flow to the target
    std::vector<VersionedValue *> allFlowSources(VersionedValue *target) const;

    /// @brief All the end sources that can flow to the target
    std::vector<VersionedValue *>
    allFlowSourcesEnds(VersionedValue *target) const;

    std::vector<VersionedValue *>
    populateArgumentValuesList(llvm::CallInst *site,
                               std::vector<ref<Expr> > &arguments);

    /// @brief Construct dependency due to load instruction
    bool buildLoadDependency(llvm::Value *fromValue, llvm::Value *toValue,
                             ref<Expr> toValueExpr);

  public:
    Dependency(Dependency *prev);

    ~Dependency();

    Dependency *cdr() const;

    void execute(llvm::Instruction *instr, ref<Expr> valueExpr);

    VersionedValue *getLatestValue(llvm::Value *value) const;

    void bindCallArguments(llvm::Instruction *instr,
                           std::vector<ref<Expr> > &arguments);

    void bindReturnValue(llvm::CallInst *site, llvm::Instruction *inst,
                         ref<Expr> returnValue);

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

  bool isCompositeAllocation(llvm::Value *site);
}

#endif
