
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
class Dependency;

class ShadowArray {
  static std::map<const Array *, const Array *> shadowArray;

  static UpdateNode *getShadowUpdate(const UpdateNode *chain);

public:
  static void addShadowArrayMap(const Array *source, const Array *target);

  static ref<Expr> getShadowExpression(ref<Expr> expr);

  static std::string getShadowName(std::string name) {
    return "__shadow__" + name;
  }
};

class Allocation {
  bool core;

  protected:
    llvm::Value *site;

    Allocation() : core(false), site(0) {}

    Allocation(llvm::Value *site) : core(false), site(site) {}

  public:
    virtual ~Allocation() {}

    virtual bool hasAllocationSite(llvm::Value *site) const { return false; }

    virtual bool isComposite() const;

    virtual void print(llvm::raw_ostream& stream) const;

    llvm::Value *getSite() const { return site; }

    void setAsCore() { core = true; }

    bool isCore() { return core; }

    void dump() const {
      print(llvm::errs());
      llvm::errs() << "\n";
    }
  };

  class CompositeAllocation : public Allocation {
  public:
    CompositeAllocation(llvm::Value *site) : Allocation(site) {}

    ~CompositeAllocation() {}

    bool hasAllocationSite(llvm::Value *site) const {
      return this->site == site;
    }

    void print(llvm::raw_ostream &stream) const;
  };

  class VersionedAllocation : public Allocation {

  public:
    VersionedAllocation(llvm::Value *site) : Allocation(site) {}

    ~VersionedAllocation() {}

    bool hasAllocationSite(llvm::Value *site) const {
      return this->site == site;
    }

    bool isComposite() const;

    void print(llvm::raw_ostream& stream) const;
  };

  class EnvironmentAllocation : public Allocation {
  public:
    EnvironmentAllocation() {}

    ~EnvironmentAllocation() {}

    bool hasAllocationSite(llvm::Value *site) const;

    void print(llvm::raw_ostream &stream) const;
  };

  class VersionedValue {

    const llvm::Value *value;

    const ref<Expr> valueExpr;

    /// @brief to indicate if any unsatisfiability core
    /// depends on this value
    bool inInterpolant;
  public:
    VersionedValue(llvm::Value *value, ref<Expr> valueExpr)
        : value(value), valueExpr(valueExpr), inInterpolant(false) {}

    ~VersionedValue() {}

    bool hasValue(llvm::Value *value) const { return this->value == value; }

    ref<Expr> getExpression() const { return valueExpr; }

    void includeInInterpolant() { inInterpolant = true; }

    bool valueInInterpolant() const { return inInterpolant; }

    void print(llvm::raw_ostream& stream) const;

    void dump() const {
      print(llvm::errs());
      llvm::errs() << "\n";
    }
  };

  class PointerEquality {
    // value equals allocation (pointer)
    const VersionedValue *value;
    Allocation *allocation;

  public:
    PointerEquality(const VersionedValue *value, Allocation *allocation)
        : value(value), allocation(allocation) {}

    ~PointerEquality() {}

    Allocation *equals(const VersionedValue *value) const {
      return this->value == value ? allocation : 0;
    }

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
    StorageCell(Allocation *allocation, VersionedValue *value)
        : allocation(allocation), value(value) {}

    ~StorageCell() {}

    VersionedValue *stores(Allocation *allocation) const {
      return this->allocation == allocation ? this->value : 0;
    }

    Allocation *storageOf(const VersionedValue *value) const {
      return this->value == value ? this->allocation : 0;
    }

    Allocation *getAllocation() const { return this->allocation; }

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

    // Store-load via allocation site
    Allocation *via;

  public:
    FlowsTo(VersionedValue *source, VersionedValue *target)
        : source(source), target(target), via(0) {}

    FlowsTo(VersionedValue *source, VersionedValue *target, Allocation *via)
        : source(source), target(target), via(via) {}

    ~FlowsTo() {}

    VersionedValue *getSource() const { return this->source; }

    VersionedValue *getTarget() const { return this->target; }

    Allocation *getAllocation() const { return this->via; }

    void print(llvm::raw_ostream& sream) const;

    void dump() const {
      print(llvm::errs());
      llvm::errs() << "\n";
    }
  };

  class AllocationGraph {

    class AllocationNode {
      Allocation *allocation;
      std::vector<AllocationNode *> ancestors;

    public:
      AllocationNode(Allocation *allocation) : allocation(allocation) {}

      ~AllocationNode() { ancestors.clear(); }

      Allocation *getAllocation() const { return allocation; }

      void addParent(AllocationNode *node) {
        // The user should ensure that we don't store a duplicate
        ancestors.push_back(node);
      }

      bool isCurrentParent(AllocationNode *node) {
        if (std::find(ancestors.begin(), ancestors.end(), node) ==
            ancestors.end())
          return false;
        return true;
      }

      std::vector<AllocationNode *> getParents() const { return ancestors; }
    };

    std::vector<AllocationNode *> sinks;
    std::vector<AllocationNode *> allNodes;

    void print(llvm::raw_ostream &stream, std::vector<AllocationNode *> nodes,
               std::vector<AllocationNode *> &printed,
               const unsigned tabNum) const;

  public:
    AllocationGraph() {}

    ~AllocationGraph() {
      for (std::vector<AllocationNode *>::iterator it = allNodes.begin(),
                                                   itEnd = allNodes.end();
           it != itEnd; ++it) {
        delete *it;
      }
      allNodes.clear();
    }

    bool addNewEdge(Allocation *source, Allocation *target);

    void consumeSinkNode(Allocation *allocation);

    void dump() const {
      this->print(llvm::errs());
      llvm::errs() << "\n";
    }

    void print(llvm::raw_ostream &stream) const;
  };

  class Dependency {

  public:
    class Util {

    public:
      template <typename T>
      static void deletePointerVector(std::vector<T *> &list);

      static bool isCompositeAllocation(llvm::Value *site);

      static bool isEnvironmentAllocation(llvm::Value *site);
    };

  private:
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

    std::vector<llvm::Value *> newCompositeAllocations;

    VersionedValue *getNewVersionedValue(llvm::Value *value,
                                         ref<Expr> valueExpr);

    Allocation *getInitialAllocation(llvm::Value *allocation);

    Allocation *getNewAllocationVersion(llvm::Value *allocation);

    std::vector<llvm::Value *> getAllVersionedAllocations() const;

    std::vector<llvm::Value *> getAllCompositeAllocations() const;

    /// @brief Gets the latest version of the allocation. For unversioned
    /// allocations (e.g., composite and environment), this should return
    /// the only allocation.
    Allocation *getLatestAllocation(llvm::Value *allocation) const;

    void addPointerEquality(const VersionedValue *value,
                            Allocation *allocation);

    void updateStore(Allocation *allocation, VersionedValue *value);

    void addDependency(VersionedValue *source, VersionedValue *target);

    void addDependencyViaAllocation(VersionedValue *source,
                                    VersionedValue *target, Allocation *via);

    Allocation *resolveAllocation(VersionedValue *value) const;

    std::vector<Allocation *>
    resolveAllocationTransitively(VersionedValue *value) const;

    std::vector<VersionedValue *> stores(Allocation *allocation) const;

    /// @brief All values that flows to the target in one step, local
    /// to the current dependency / interpolation tree node
    std::vector<VersionedValue *> directLocalFlowSources(VersionedValue *target) const;

    /// @brief All values that flows to the target in one step
    std::vector<VersionedValue *> directFlowSources(VersionedValue *target) const;

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

    /// @brief Direct allocation dependency local to an interpolation tree node
    std::map<VersionedValue *, Allocation *>
    directLocalAllocationSources(VersionedValue *target) const;

    /// @brief Direct allocation dependency
    std::map<VersionedValue *, Allocation *>
    directAllocationSources(VersionedValue *target) const;

    /// @brief Builds dependency graph between memory allocations
    std::vector<Allocation *> buildAllocationGraph(AllocationGraph *g,
                                                   VersionedValue *value) const;

  public:
    Dependency(Dependency *prev);

    ~Dependency();

    Dependency *cdr() const;

    void execute(llvm::Instruction *instr, ref<Expr> valueExpr);

    VersionedValue *getLatestValue(llvm::Value *value) const;

    std::map<llvm::Value *, ref<Expr> >
    getLatestCoreExpressions(bool interpolantValueOnly) const;

    std::map<llvm::Value *, std::vector<ref<Expr> > >
    getCompositeCoreExpressions(bool interpolantValueOnly) const;

    void bindCallArguments(llvm::Instruction *instr,
                           std::vector<ref<Expr> > &arguments);

    void bindReturnValue(llvm::CallInst *site, llvm::Instruction *inst,
                         ref<Expr> returnValue);

    void markAllValues(AllocationGraph *g, VersionedValue *value);

    void dump() const {
      this->print(llvm::errs());
      llvm::errs() << "\n";
    }

    void print(llvm::raw_ostream& stream) const;

    void print(llvm::raw_ostream &stream, const unsigned tabNum) const;
  };


  std::string makeTabs(const unsigned tab_num);

  std::string appendTab(const std::string &prefix);

}

#endif
