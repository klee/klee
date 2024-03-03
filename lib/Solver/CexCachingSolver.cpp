//===-- CexCachingSolver.cpp ----------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Solver/Solver.h"

#include "klee/ADT/MapOfSets.h"
#include "klee/Expr/Assignment.h"
#include "klee/Expr/Constraints.h"
#include "klee/Expr/Expr.h"
#include "klee/Expr/ExprUtil.h"
#include "klee/Expr/ExprVisitor.h"
#include "klee/Support/OptionCategories.h"
#include "klee/Statistics/TimerStatIncrementer.h"
#include "klee/Solver/SolverImpl.h"
#include "klee/Solver/SolverStats.h"
#include "klee/Support/ErrorHandling.h"

#include "llvm/Support/CommandLine.h"

#include <memory>
#include <utility>

using namespace klee;
using namespace llvm;

namespace {
cl::opt<bool> DebugCexCacheCheckBinding(
    "debug-cex-cache-check-binding", cl::init(false),
    cl::desc("Debug the correctness of the counterexample "
             "cache assignments (default=false)"),
    cl::cat(SolvingCat));

cl::opt<bool>
    CexCacheTryAll("cex-cache-try-all", cl::init(false),
                   cl::desc("Try substituting all counterexamples before "
                            "asking the SMT solver (default=false)"),
                   cl::cat(SolvingCat));

cl::opt<bool>
    CexCacheSuperSet("cex-cache-superset", cl::init(false),
                     cl::desc("Try substituting SAT superset counterexample "
                              "before asking the SMT solver (default=false)"),
                     cl::cat(SolvingCat));

} // namespace

///

typedef std::set< ref<Expr> > KeyType;

struct AssignmentLessThan {
  bool operator()(const Assignment *a, const Assignment *b) const {
    return a->bindings < b->bindings;
  }
};


class CexCachingSolver : public SolverImpl {
  typedef std::set<Assignment*, AssignmentLessThan> assignmentsTable_ty;

  std::unique_ptr<Solver> solver;
  
  MapOfSets<ref<Expr>, Assignment*> cache;
  // memo table
  assignmentsTable_ty assignmentsTable;

  bool searchForAssignment(KeyType &key, 
                           Assignment *&result);
  
  bool lookupAssignment(const Query& query, KeyType &key, Assignment *&result);

  bool lookupAssignment(const Query& query, Assignment *&result) {
    KeyType key;
    return lookupAssignment(query, key, result);
  }

  bool getAssignment(const Query& query, Assignment *&result);
  
public:
  CexCachingSolver(std::unique_ptr<Solver> solver)
      : solver(std::move(solver)) {}
  ~CexCachingSolver();

  bool computeTruth(const Query &, bool &isValid) override;
  bool computeValidity(const Query &, Solver::Validity &result) override;
  bool computeValue(const Query &, ref<Expr> &result) override;
  bool computeInitialValues(const Query &,
                            const std::vector<const Array *> &objects,
                            std::vector<std::vector<unsigned char>> &values,
                            bool &hasSolution) override;
  SolverRunStatus getOperationStatusCode() override;
  std::string getConstraintLog(const Query &query) override;
  void setCoreSolverTimeout(time::Span timeout) override;
};

///

struct NullAssignment {
  bool operator()(Assignment *a) const { return !a; }
};

struct NonNullAssignment {
  bool operator()(Assignment *a) const { return a!=0; }
};

struct NullOrSatisfyingAssignment {
  KeyType &key;
  
  NullOrSatisfyingAssignment(KeyType &_key) : key(_key) {}

  bool operator()(Assignment *a) const { 
    return !a || a->satisfies(key.begin(), key.end()); 
  }
};

/// searchForAssignment - Look for a cached solution for a query.
///
/// \param key - The query to look up.
/// \param result [out] - The cached result, if the lookup is successful. This is
/// either a satisfying assignment (for a satisfiable query), or 0 (for an
/// unsatisfiable query).
/// \return - True if a cached result was found.
bool CexCachingSolver::searchForAssignment(KeyType &key, Assignment *&result) {
  Assignment * const *lookup = cache.lookup(key);
  if (lookup) {
    result = *lookup;
    return true;
  }

  if (CexCacheTryAll) {
    // Look for a satisfying assignment for a superset, which is trivially an
    // assignment for any subset.
    Assignment **lookup = 0;
    if (CexCacheSuperSet)
      lookup = cache.findSuperset(key, NonNullAssignment());

    // Otherwise, look for a subset which is unsatisfiable, see below.
    if (!lookup) 
      lookup = cache.findSubset(key, NullAssignment());

    // If either lookup succeeded, then we have a cached solution.
    if (lookup) {
      result = *lookup;
      return true;
    }

    // Otherwise, iterate through the set of current assignments to see if one
    // of them satisfies the query.
    for (assignmentsTable_ty::iterator it = assignmentsTable.begin(), 
           ie = assignmentsTable.end(); it != ie; ++it) {
      Assignment *a = *it;
      if (a->satisfies(key.begin(), key.end())) {
        result = a;
        return true;
      }
    }
  } else {
    // FIXME: Which order? one is sure to be better.

    // Look for a satisfying assignment for a superset, which is trivially an
    // assignment for any subset.
    Assignment **lookup = 0;
    if (CexCacheSuperSet)
      lookup = cache.findSuperset(key, NonNullAssignment());

    // Otherwise, look for a subset which is unsatisfiable -- if the subset is
    // unsatisfiable then no additional constraints can produce a valid
    // assignment. While searching subsets, we also explicitly the solutions for
    // satisfiable subsets to see if they solve the current query and return
    // them if so. This is cheap and frequently succeeds.
    if (!lookup) 
      lookup = cache.findSubset(key, NullOrSatisfyingAssignment(key));

    // If either lookup succeeded, then we have a cached solution.
    if (lookup) {
      result = *lookup;
      return true;
    }
  }
  
  return false;
}

/// lookupAssignment - Lookup a cached result for the given \arg query.
///
/// \param query - The query to lookup.
/// \param key [out] - On return, the key constructed for the query.
/// \param result [out] - The cached result, if the lookup is successful. This is
/// either a satisfying assignment (for a satisfiable query), or 0 (for an
/// unsatisfiable query).
/// \return True if a cached result was found.
bool CexCachingSolver::lookupAssignment(const Query &query, 
                                        KeyType &key,
                                        Assignment *&result) {
  key = KeyType(query.constraints.begin(), query.constraints.end());
  ref<Expr> neg = Expr::createIsZero(query.expr);
  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(neg)) {
    if (CE->isFalse()) {
      result = (Assignment*) 0;
      ++stats::queryCexCacheHits;
      return true;
    }
  } else {
    key.insert(neg);
  }

  bool found = searchForAssignment(key, result);
  if (found)
    ++stats::queryCexCacheHits;
  else ++stats::queryCexCacheMisses;
    
  return found;
}

bool CexCachingSolver::getAssignment(const Query& query, Assignment *&result) {
  KeyType key;
  if (lookupAssignment(query, key, result))
    return true;

  std::vector<const Array*> objects;
  findSymbolicObjects(key.begin(), key.end(), objects);

  std::vector< std::vector<unsigned char> > values;
  bool hasSolution;
  if (!solver->impl->computeInitialValues(query, objects, values, 
                                          hasSolution))
    return false;
    
  Assignment *binding;
  if (hasSolution) {
    binding = new Assignment(objects, values);

    // Memoize the result.
    std::pair<assignmentsTable_ty::iterator, bool>
      res = assignmentsTable.insert(binding);
    if (!res.second) {
      delete binding;
      binding = *res.first;
    }
    
    if (DebugCexCacheCheckBinding)
      if (!binding->satisfies(key.begin(), key.end())) {
        query.dump();
        binding->dump();
        klee_error("Generated assignment doesn't match query");
      }
  } else {
    binding = (Assignment*) 0;
  }
  
  result = binding;
  cache.insert(key, binding);

  return true;
}

///

CexCachingSolver::~CexCachingSolver() {
  cache.clear();
  for (assignmentsTable_ty::iterator it = assignmentsTable.begin(), 
         ie = assignmentsTable.end(); it != ie; ++it)
    delete *it;
}

bool CexCachingSolver::computeValidity(const Query& query,
                                       Solver::Validity &result) {
  TimerStatIncrementer t(stats::cexCacheTime);
  Assignment *a;
  if (!getAssignment(query.withFalse(), a))
    return false;
  assert(a && "computeValidity() must have assignment");
  ref<Expr> q = a->evaluate(query.expr);
  assert(isa<ConstantExpr>(q) && 
         "assignment evaluation did not result in constant");

  if (cast<ConstantExpr>(q)->isTrue()) {
    if (!getAssignment(query, a))
      return false;
    result = !a ? Solver::True : Solver::Unknown;
  } else {
    if (!getAssignment(query.negateExpr(), a))
      return false;
    result = !a ? Solver::False : Solver::Unknown;
  }
  
  return true;
}

bool CexCachingSolver::computeTruth(const Query& query,
                                    bool &isValid) {
  TimerStatIncrementer t(stats::cexCacheTime);

  Assignment *a;
  if (!getAssignment(query, a))
    return false;

  isValid = !a;

  return true;
}

bool CexCachingSolver::computeValue(const Query& query,
                                    ref<Expr> &result) {
  TimerStatIncrementer t(stats::cexCacheTime);

  Assignment *a;
  if (!getAssignment(query.withFalse(), a))
    return false;
  assert(a && "computeValue() must have assignment");
  result = a->evaluate(query.expr);  
  assert(isa<ConstantExpr>(result) && 
         "assignment evaluation did not result in constant");
  return true;
}

bool 
CexCachingSolver::computeInitialValues(const Query& query,
                                       const std::vector<const Array*> 
                                         &objects,
                                       std::vector< std::vector<unsigned char> >
                                         &values,
                                       bool &hasSolution) {
  TimerStatIncrementer t(stats::cexCacheTime);
  Assignment *a;
  if (!getAssignment(query, a))
    return false;
  hasSolution = !!a;
  
  if (!a)
    return true;

  // FIXME: We should use smarter assignment for result so we don't
  // need redundant copy.
  values = std::vector< std::vector<unsigned char> >(objects.size());
  for (unsigned i=0; i < objects.size(); ++i) {
    const Array *os = objects[i];
    Assignment::bindings_ty::iterator it = a->bindings.find(os);
    
    if (it == a->bindings.end()) {
      values[i] = std::vector<unsigned char>(os->size, 0);
    } else {
      values[i] = it->second;
    }
  }
  
  return true;
}

SolverImpl::SolverRunStatus CexCachingSolver::getOperationStatusCode() {
  return solver->impl->getOperationStatusCode();
}

std::string CexCachingSolver::getConstraintLog(const Query& query) {
  return solver->impl->getConstraintLog(query);
}

void CexCachingSolver::setCoreSolverTimeout(time::Span timeout) {
  solver->impl->setCoreSolverTimeout(timeout);
}

///

std::unique_ptr<Solver>
klee::createCexCachingSolver(std::unique_ptr<Solver> solver) {
  return std::make_unique<Solver>(
      std::make_unique<CexCachingSolver>(std::move(solver)));
}
