//===--- Dependency.h - Memory location dependency --------------*- C++ -*-===//
//
//               The Tracer-X KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declarations for the flow-insensitive dependency
/// analysis to compute the memory locations upon which the unsatisfiability
/// core
/// depends, which is used in computing the interpolant.
///
//===----------------------------------------------------------------------===//

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

/// \brief Implements the replacement mechanism for replacing variables, used in
/// replacing free with bound variables.
class ShadowArray {
  static std::map<const Array *, const Array *> shadowArray;

  static UpdateNode *getShadowUpdate(const UpdateNode *chain,
                                     std::set<const Array *> &replacements);

public:
  static ref<Expr> createBinaryOfSameKind(ref<Expr> originalExpr,
                                          ref<Expr> newLhs, ref<Expr> newRhs);

  static void addShadowArrayMap(const Array *source, const Array *target);

  static ref<Expr> getShadowExpression(ref<Expr> expr,
                                       std::set<const Array *> &replacements);

  static std::string getShadowName(std::string name) {
    return "__shadow__" + name;
  }
};

/// \brief A class to represent memory locations.
class MemoryLocation {

  protected:
    bool core;

    llvm::Value *site;

    ref<Expr> address;

    MemoryLocation(llvm::Value *_site, ref<Expr> &_address)
        : core(false), site(_site), address(_address) {}

  public:
    enum Kind {
      Unknown,
      Versioned
    };

    virtual Kind getKind() const { return Unknown; }

    virtual ~MemoryLocation() {}

    virtual bool hasAddress(llvm::Value *_site, ref<Expr> &_address) const {
      return site == _site && address == _address;
    }

    /// \brief Print the content of the object into a stream.
    ///
    /// \param The stream to print the data to.
    virtual void print(llvm::raw_ostream& stream) const;

    bool hasConstantAddress() { return llvm::isa<ConstantExpr>(address.get()); }

    uint64_t getUIntAddress() {
      return llvm::dyn_cast<ConstantExpr>(address.get())->getZExtValue();
    }

    static bool classof(const MemoryLocation *loc) { return true; }

    llvm::Value *getSite() const { return site; }

    ref<Expr> getAddress() const { return address; }

    void setAsCore() { core = true; }

    bool isCore() { return core; }

    /// \brief Print the content of the object to the LLVM error stream
    void dump() const {
      print(llvm::errs());
      llvm::errs() << "\n";
    }
};

/// \brief A class that represents locations that can be destructively updated
/// (versioned)
class VersionedLocation : public MemoryLocation {
  public:
    VersionedLocation(llvm::Value *_site, ref<Expr> &_address)
        : MemoryLocation(_site, _address) {}

    ~VersionedLocation() {}

    Kind getKind() const { return Versioned; }

    static bool classof(const MemoryLocation *loc) {
      return loc->getKind() == Versioned;
    }

    static bool classof(const VersionedLocation *loc) { return true; }

    /// \brief Print the content of the object into a stream.
    ///
    /// \param The stream to print the data to.
    void print(llvm::raw_ostream& stream) const;
  };

  /// \brief A class that represents LLVM value that can be destructively
  /// updated (versioned).
  class VersionedValue {

    llvm::Value *value;

    const ref<Expr> valueExpr;

    /// \brief Field to indicate if any unsatisfiability core depends on this
    /// value.
    bool core;

  public:
    VersionedValue(llvm::Value *value, ref<Expr> valueExpr)
        : value(value), valueExpr(valueExpr), core(false) {}

    ~VersionedValue() {}

    bool hasValue(llvm::Value *value) const { return this->value == value; }

    ref<Expr> getExpression() const { return valueExpr; }

    void setAsCore() { core = true; }

    bool isCore() const { return core; }

    llvm::Value *getValue() const { return value; }

    /// \brief Print the content of the object into a stream.
    ///
    /// \param The stream to print the data to.
    void print(llvm::raw_ostream& stream) const;

    /// \brief Print the content of the object to the LLVM error stream
    void dump() const {
      print(llvm::errs());
      llvm::errs() << "\n";
    }
  };

  /// \brief The location graph: A graph to directly represent the dependency
  /// between locations, instead of using intermediate values. This graph is
  /// computed from the relations between values in particular the FlowsTo
  /// relation.
  class LocationGraph {

    /// \brief Implements a node of the location graph.
    class LocationNode {
      MemoryLocation *loc;
      std::vector<LocationNode *> ancestors;
      uint64_t level;

    public:
      LocationNode(MemoryLocation *loc, uint64_t _level)
          : loc(loc), level(_level) {
        loc->setAsCore();
      }

      ~LocationNode() { ancestors.clear(); }

      MemoryLocation *getLocation() const { return loc; }

      void addParent(LocationNode *node) {
        // The user should ensure that we don't store a duplicate
        ancestors.push_back(node);
      }

      std::vector<LocationNode *> getParents() const { return ancestors; }

      uint64_t getLevel() const { return level; }
    };

    std::vector<LocationNode *> sinks;
    std::vector<LocationNode *> allNodes;

    /// \brief Prints the content of the location graph
    void print(llvm::raw_ostream &stream, std::vector<LocationNode *> nodes,
               std::vector<LocationNode *> &printed,
               const unsigned tabNum) const;

    /// \brief Given a location, delete all sinks having such location, and
    /// replace them as sinks with their parents.
    ///
    /// \param The memory location to match a sink node with.
    void consumeSinkNode(MemoryLocation *location);

  public:
    LocationGraph() {}

    ~LocationGraph() {
      for (std::vector<LocationNode *>::iterator it = allNodes.begin(),
                                                 itEnd = allNodes.end();
           it != itEnd; ++it) {
        delete *it;
      }
      allNodes.clear();
    }

    bool isVisited(MemoryLocation *loc);

    void addNewSink(MemoryLocation *candidateSink);

    void addNewEdge(MemoryLocation *source, MemoryLocation *target);

    std::set<MemoryLocation *> getSinkLocations() const;

    std::set<MemoryLocation *>
    getSinksWithLocations(std::vector<MemoryLocation *> valuesList) const;

    /// Given a set of locations, delete all sinks having an location in the
    /// set, and replace them as sinks with their parents.
    ///
    /// \param The location to match the sink nodes with.
    void consumeSinksWithLocations(std::vector<MemoryLocation *> locationsList);

    /// \brief Print the content of the object to the LLVM error stream
    void dump() const {
      this->print(llvm::errs());
      llvm::errs() << "\n";
    }

    /// \brief Print the content of the object into a stream.
    ///
    /// \param The stream to print the data to.
    void print(llvm::raw_ostream &stream) const;
  };

  /// \brief Implementation of value dependency for computing locations the
  /// unsatisfiability core depends upon, which is used to compute the
  /// interpolant.
  ///
  /// Following is the analysis rules to compute value dependency relations
  /// useful for computing the interpolant. Given a finite symbolic execution
  /// path, the computation of the relations terminates. The analysis rules
  /// serve as a guide to the implementation.
  ///
  /// Problems solved:
  /// 1. Components of program states upon which the unsatisfiability core
  ///    depends need to be computed. These components may not be represented
  ///    in the constraints.
  /// 2. To gain more subsumption, we need to store interpolation at more
  ///    program points. More specifically, here we would like to compute the
  ///    instructions that are related to the unsatisfiability core in order
  ///    to compute the right interpolant. That is, given a constraint c(x0)
  ///    in the core, we want to compute the set of state update statements S
  ///    from which we compose the state update function f_S where the next
  ///    state x' = f_S(x0), such that the interpolant after the state update
  ///    is exists x0 . c(x0) /\ x' = f_S(x0)
  ///
  /// Solution:
  /// The dependency computation is based on shadow data structure
  /// representing the following:
  ///
  /// Domains:
  /// VersionedValue -> LLVM values (i.e., variables) with versioning index
  /// VersionedLocation -> Memory locations with versioning index
  ///
  /// Basic Relations:
  /// stores(VersionedLocation, VersionedValue) - Memory state
  /// depends(VersionedValue, VersionedValue) - Value dependency: The output
  ///    of the analysis.
  /// equals(VersionedValue, VersionedLocation) - Pointer value equality
  ///
  /// Derived Relations:
  /// Transitive Closure of depends
  ///
  /// depends*(v, v)
  /// depends*(v, v') /\ v != v' iff depends(v, v') /\ depends*(v', v'')
  ///
  /// Indirection Relation
  ///
  /// ind(v, m, 0) iff depends*(v, v') /\ equals(v', m)
  /// ind(v, m, i) /\ i>=1 iff
  ///    depends*(v, v') /\ stores(v'', v') /\ ind(v'', m, i-1)
  ///
  /// In the following abstract operational semantics of LLVM instructions,
  /// R and R' represent the abstract states before and after the execution.
  /// An abstract state is a set having as elements ground substitutions of
  /// the above relations. Below, v and its primed versions represent
  /// VersionedValue elements whereas m and its primed versions represent
  /// VersionedLocation elements.
  ///
  /// Location: v = alloca
  ///
  /// ---------------------------------------------------
  /// R --> R U {equals(succ(v), m) | R |/- equals(_, m)}
  ///
  /// Here succ(v) denotes the next (new) version of v.
  ///
  /// Store: store v', v
  ///
  /// ----------------------------------------------------
  /// R --> R U { stores(succ(m),v) | R |- ind(v', m, 0) }
  ///
  /// Here we use succ(m) to denote the next version of m as this was a
  /// destructive update.
  ///
  /// -------------------------------------------------------------
  /// R --> R U { stores(succ(ind(m,i)), v) | R |- ind(v', m, i), i > 0 }
  ///
  /// Here ind(m,i) is an abstract memory location representing any
  /// memory location that is i-step-reachable via indirection from m.
  ///
  /// R |/- ind(v, _, _)
  /// --------------------------
  /// R --> R U {stores(UNK, v)}
  ///
  /// Here UNK represents an unknown memory location. We assume that
  /// UNK cannot be versioned (non-destructive update applies to it).
  ///
  /// Load: v = load v'
  ///
  /// Here the rules are not mutually exclusive such that we avoid using set
  /// union to denote abstract states after the execution.
  ///
  /// R |- ind(v', latest(m), 0) /\ stores(latest(m), v''')
  /// R' |- depends(succ(v), v''')
  /// -----------------------------------------------------
  /// R --> R'
  ///
  /// Here latest(m) is only the latest version of Location m.
  ///
  /// R |- ind(v', m, i) /\ i > 0 /\ stores(m, v''')
  /// R' |- depends(succ(v), v''')
  /// ----------------------------------------------
  /// R --> R'
  ///
  /// R |/- ind(v', _, _)          R' |- stores(UNK, succ(v))
  /// -------------------------------------------------------
  /// R --> R'
  ///
  /// R |- stores(UNK, v'')                R' |- depends(v, v'')
  /// ----------------------------------------------------------
  /// R --> R'
  ///
  /// Here, any stores to an unknown address would be loaded.
  ///
  /// Getelementptr: v = getelementptr v', idx
  ///
  /// --------------------------------
  /// R --> R U {depends(succ(v), v')}
  ///
  /// Unary Operation: v = UNARY_OP(v') (including binary operation with 1
  /// constant argument)
  ///
  /// --------------------------------
  /// R --> R U {depends(succ(v), v')}
  ///
  /// Binary Operation: v = BINARY_OP(v', v'')
  ///
  /// -------------------------------------------------------
  /// R --> R U {depends(succ(v), v'), depends(succ(v), v'')}
  ///
  /// Phi Node: v = PHI(v'1, ..., v'n)
  ///
  /// -------------------------------------------------------------
  /// R --> R U {depends(succ(v), v'1), ..., depends(succ(v), v'n)}
  ///
  class Dependency {

  public:
    typedef std::pair<ref<Expr>, ref<Expr> > AddressValuePair;
    typedef std::map<uint64_t, AddressValuePair> ConcreteStoreMap;
    typedef std::vector<AddressValuePair> SymbolicStoreMap;
    typedef std::map<llvm::Value *, ConcreteStoreMap> ConcreteStore;
    typedef std::map<llvm::Value *, SymbolicStoreMap> SymbolicStore;

    class Util {

    public:
      template <typename T>
      static void deletePointerVector(std::vector<T *> &list);

      template <typename Key, typename T>
      static void deletePointerMap(std::map<Key *, T *> &map);

      template <typename Key, typename T>
      static void
      deletePointerMapWithVectorValue(std::map<Key *, std::vector<T *> > &map);

      template <typename Key, typename T>
      static void
      deletePointerMapWithMapValue(std::map<Key *, std::map<Key *, T *> > &map);

      /// \brief Tests if an allocation site is main function's argument
      static bool isMainArgument(llvm::Value *site);
    };

  private:
    /// \brief Previous path condition
    Dependency *parentDependency;

    /// \brief Argument values to be passed onto callee
    std::vector<VersionedValue *> argumentValuesList;

    /// \brief Equality of value to address
    std::map<const VersionedValue *, std::vector<MemoryLocation *> >
    equalityMap;

    /// \brief The mapping of locations to stored value
    std::map<MemoryLocation *, VersionedValue *> storesMap;

    /// \brief Store the inverse map of both storesMap
    std::map<VersionedValue *, std::vector<MemoryLocation *> > storageOfMap;

    /// \brief Flow relations of target and its sources with location
    std::map<VersionedValue *, std::map<VersionedValue *, MemoryLocation *> >
    flowsToMap;

    /// \brief The store of the versioned values
    std::map<llvm::Value *, std::vector<VersionedValue *> > valuesMap;

    /// \brief The store of the versioned locations
    std::vector<MemoryLocation *> versionedLocationsList;

    /// \brief Locations of this node and its ancestors that are needed for
    /// the core and dominates other locations.
    std::set<MemoryLocation *> coreLocations;

    /// \brief Create a new versioned value object, typically when executing a
    /// new instruction, as a value for the instruction.
    VersionedValue *getNewVersionedValue(llvm::Value *value,
                                         ref<Expr> valueExpr);

    /// \brief Create a fresh location object.
    MemoryLocation *getInitialLocation(llvm::Value *site, ref<Expr> &address);

    /// \brief Create a new location object to represent a new version of a
    /// known location.
    MemoryLocation *getNewLocationVersion(llvm::Value *site,
                                          ref<Expr> &address);

    /// \brief Get all versioned locations for the current node an all of its
    /// parents
    std::vector<MemoryLocation *> getAllVersionedLocations(bool coreOnly =
                                                               false) const;

    /// \brief Gets the latest version of the location.
    MemoryLocation *getLatestLocation(llvm::Value *site,
                                      ref<Expr> address) const;

    /// \brief Gets the latest version of the location, but without checking
    /// for whether the value is constant or not
    VersionedValue *getLatestValueNoConstantCheck(llvm::Value *value);

    /// \brief Newly relate an LLVM value with destructive update to a
    /// location
    void addPointerEquality(const VersionedValue *value, MemoryLocation *loc);

    /// \brief Newly relate an location with its stored value
    void updateStore(MemoryLocation *location, VersionedValue *value);

    /// \brief Add flow dependency between source and target value
    void addDependency(VersionedValue *source, VersionedValue *target);

    /// \brief Add flow dependency between source and target value, as the
    /// result of store/load via a memory location.
    void addDependencyViaLocation(VersionedValue *source,
                                  VersionedValue *target, MemoryLocation *via);

    /// \brief Given a versioned value, retrieve the equality it is associated
    /// with
    MemoryLocation *resolveLocation(VersionedValue *value);

    /// \brief Given a versioned value, retrieve the equality it is associated
    /// with, in this object or otherwise in its ancestors.
    std::vector<MemoryLocation *>
    resolveLocationTransitively(VersionedValue *value);

    /// \brief Retrieve the versioned values that are stored in a particular
    /// location.
    std::vector<VersionedValue *> stores(MemoryLocation *loc) const;

    /// \brief All values that flows to the target in one step, local to the
    /// current dependency / interpolation tree node
    std::vector<VersionedValue *> directLocalFlowSources(VersionedValue *target) const;

    /// \brief All values that flows to the target in one step
    std::vector<VersionedValue *> directFlowSources(VersionedValue *target) const;

    /// \brief All values that could flow to the target
    std::vector<VersionedValue *> allFlowSources(VersionedValue *target) const;

    /// \brief All the end sources that can flow to the target
    std::vector<VersionedValue *>
    allFlowSourcesEnds(VersionedValue *target) const;

    /// \brief Record the expressions of a call's arguments
    std::vector<VersionedValue *>
    populateArgumentValuesList(llvm::CallInst *site,
                               std::vector<ref<Expr> > &arguments);

    /// \brief Construct dependency due to load instruction
    bool buildLoadDependency(llvm::Value *address, ref<Expr> addressExpr,
                             llvm::Value *value, ref<Expr> valueExpr);

    /// \brief Direct location dependency local to an interpolation tree node
    std::map<VersionedValue *, MemoryLocation *>
    directLocalLocationSources(VersionedValue *target) const;

    /// \brief Direct location dependency
    std::map<VersionedValue *, MemoryLocation *>
    directLocationSources(VersionedValue *target) const;

    /// \brief Builds dependency graph between memory locations
    void recursivelyBuildLocationGraph(
        LocationGraph *g, VersionedValue *source, MemoryLocation *target,
        std::set<MemoryLocation *> parentTargets) const;

    /// \brief Builds dependency graph between memory locations
    void buildLocationGraph(LocationGraph *g, VersionedValue *value) const;

  public:
    Dependency(Dependency *prev);

    ~Dependency();

    Dependency *cdr() const;

    VersionedValue *getLatestValue(llvm::Value *value, ref<Expr> valueExpr);

    /// \brief Abstract dependency state transition with argument(s)
    void execute(llvm::Instruction *instr, std::vector<ref<Expr> > &args,
                 bool symbolicExecutionError);

    /// \brief Build dependencies from PHI node
    void executePHI(llvm::Instruction *instr, unsigned int incomingBlock,
                    ref<Expr> valueExpr, bool symbolicExecutionError);

    /// \brief Execute memory operation (load/store)
    void executeMemoryOperation(llvm::Instruction *instr,
                                std::vector<ref<Expr> > &args, bool boundsCheck,
                                bool symbolicExecutionError);

    /// \brief This retrieves the locations known at this state, and the
    /// expressions stored in the locations.
    ///
    /// \param The replacement bound variables when retrieving state for
    /// creating subsumption table entry: As the resulting expression will
    /// be used for storing in the subsumption table, the variables need to be
    /// replaced with the bound ones.
    /// \param Indicate whether we are retrieving only data for locations
    /// relevant to an unsatisfiability core.
    /// \return A pair of the store part indexed by constants, and the store
    /// part
    /// indexed by symbolic expressions.
    std::pair<ConcreteStore, SymbolicStore>
    getStoredExpressions(std::set<const Array *> &replacements,
                         bool coreOnly) const;

    /// \brief Record call arguments in a function call
    void bindCallArguments(llvm::Instruction *instr,
                           std::vector<ref<Expr> > &arguments);

    /// \brief This propagates the dependency due to the return value of a call
    void bindReturnValue(llvm::CallInst *site, llvm::Instruction *inst,
                         ref<Expr> returnValue);

    /// \brief Given a versioned value, retrieve all its sources and mark them
    /// as in the core.
    void markAllValues(LocationGraph *g, VersionedValue *value);

    /// \brief Given an LLVM value, retrieve all its sources and mark them as in
    /// the core.
    void markAllValues(LocationGraph *g, llvm::Value *value);

    /// \brief Compute the locations that are relevant for the interpolant
    /// (core).
    void computeCoreLocations(LocationGraph *g);

    /// \brief Print the content of the object to the LLVM error stream
    void dump() const {
      this->print(llvm::errs());
      llvm::errs() << "\n";
    }

    /// \brief Print the content of the object into a stream.
    ///
    /// \param The stream to print the data to.
    void print(llvm::raw_ostream& stream) const;

    /// \brief Print the content of the object into a stream.
    ///
    /// \param The stream to print the data to.
    /// \param The number of whitespaces to be printed before each line.
    void print(llvm::raw_ostream &stream, const unsigned paddingAmount) const;
  };

  std::string makeTabs(const unsigned paddingAmount);

  std::string appendTab(const std::string &prefix);

}

#endif
