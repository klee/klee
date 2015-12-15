//===-- CexCachingSolver.cpp ----------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Solver.h"

#include "klee/Constraints.h"
#include "klee/Expr.h"
#include "klee/SolverImpl.h"
#include "klee/TimerStatIncrementer.h"
#include "klee/util/Assignment.h"
#include "klee/util/ExprUtil.h"
#include "klee/util/ExprVisitor.h"
#include "klee/Internal/ADT/MapOfSets.h"

#include "SolverStats.h"

#include "llvm/Support/CommandLine.h"

using namespace klee;
using namespace llvm;

namespace {
  cl::opt<bool>
  DebugCexCacheCheckBinding("debug-cex-cache-check-binding");

  cl::opt<bool>
  CexCacheTryAll("cex-cache-try-all",
                 cl::desc("try substituting all counterexamples before asking the SMT solver"),
                 cl::init(false));

  cl::opt<bool>
  CexCacheSuperSet("cex-cache-superset",
                 cl::desc("try substituting SAT super-set counterexample before asking the SMT solver (default=false)"),
                 cl::init(false));

  cl::opt<bool>
  CexCacheExperimental("cex-cache-exp", cl::init(false));

}

///

typedef std::set< ref<Expr> > KeyType;

struct AssignmentLessThan {
  bool operator()(const Assignment *a, const Assignment *b) {
    return a->bindings < b->bindings;
  }
};


class CexCachingSolver : public SolverImpl {
  typedef std::set<Assignment*, AssignmentLessThan> assignmentsTable_ty;

  Solver *solver;
  
  MapOfSets<ref<Expr>, AssignmentCacheWrapper*> cache;
  // memo table
  assignmentsTable_ty assignmentsTable;
  std::vector< ref<Expr> > unsat_core;

  bool searchForAssignment(KeyType &key, 
                           Assignment *&result);
  
  bool lookupAssignment(const Query& query, KeyType &key, Assignment *&result);

  bool lookupAssignment(const Query& query, Assignment *&result) {
    KeyType key;
    llvm::errs() << "CexCachingSolver::lookupAssignment/2: Calling lookupAssignment/3\n";
    return lookupAssignment(query, key, result);
  }

  bool getAssignment(const Query& query, Assignment *&result);
  
public:
  CexCachingSolver(Solver *_solver) : solver(_solver) {}
  ~CexCachingSolver();
  
  bool computeTruth(const Query&, bool &isValid);
  bool computeValidity(const Query&, Solver::Validity &result);
  bool computeValue(const Query&, ref<Expr> &result);
  bool computeInitialValues(const Query&,
                            const std::vector<const Array*> &objects,
                            std::vector< std::vector<unsigned char> > &values,
                            bool &hasSolution);
  SolverRunStatus getOperationStatusCode();
  char *getConstraintLog(const Query& query);
  void setCoreSolverTimeout(double timeout);
  std::vector< ref<Expr> > getUnsatCore() {
    llvm::errs() << "CexCachingSolver::getUnsatCore\n";
    for (std::vector< ref<Expr> >::iterator it = unsat_core.begin(); it != unsat_core.end(); it++) {
	it->get()->dump();
    }
    llvm::errs() << "Ok\n";
    return unsat_core;
  }
};

///

struct NullAssignment {
  bool operator()(AssignmentCacheWrapper *a) const {
    return !(a->getAssignment());
  }
};

struct NonNullAssignment {
  bool operator()(AssignmentCacheWrapper *a) const {
    return (a->getAssignment())!=0;
  }
};

struct NullOrSatisfyingAssignment {
  KeyType &key;
  
  NullOrSatisfyingAssignment(KeyType &_key) : key(_key) {}

  bool operator()(AssignmentCacheWrapper *a) const {
    return !(a->getAssignment()) ||
	a->getAssignment()->satisfies(key.begin(), key.end());
  }
};

/// searchForAssignment - Look for a cached solution for a query.
///
/// \param key - The query to look up.
/// \param result [out] - The cached result, if the lookup is succesful. This is
/// either a satisfying assignment (for a satisfiable query), or 0 (for an
/// unsatisfiable query).
/// \return - True if a cached result was found.
bool CexCachingSolver::searchForAssignment(KeyType &key, Assignment *&result) {
  AssignmentCacheWrapper * const *lookup = cache.lookup(key);

  if (lookup) {
    result = (*lookup)->getAssignment();
    llvm::errs() << "Storing unsat core 1\n";
    unsat_core = (*lookup)->getCore();
    for (std::vector< ref<Expr> >::iterator core_it = unsat_core.begin(); core_it != unsat_core.end(); core_it++) {
	  core_it->get()->dump();
    }
    llvm::errs() << "Ok\n";
    return true;
  }

  if (CexCacheTryAll) {
    // Look for a satisfying assignment for a superset, which is trivially an
    // assignment for any subset.
    AssignmentCacheWrapper **lookup = 0;
    if (CexCacheSuperSet)
      lookup = cache.findSuperset(key, NonNullAssignment());

    // Otherwise, look for a subset which is unsatisfiable, see below.
    if (!lookup) 
      lookup = cache.findSubset(key, NullAssignment());

    // If either lookup succeeded, then we have a cached solution.
    if (lookup) {
      result = (*lookup)->getAssignment();
      llvm::errs() << "Storing unsat core 2\n";
      unsat_core = (*lookup)->getCore();
      for (std::vector< ref<Expr> >::iterator core_it = unsat_core.begin(); core_it != unsat_core.end(); core_it++) {
	  core_it->get()->dump();
      }
      llvm::errs() << "Ok\n";
      return true;
    }

    // Otherwise, iterate through the set of current assignments to see if one
    // of them satisfies the query.
    for (assignmentsTable_ty::iterator it = assignmentsTable.begin(), 
           ie = assignmentsTable.end(); it != ie; ++it) {
      Assignment *a = *it;
      if (a->satisfies(key.begin(), key.end())) {
        result = a;
        llvm::errs() << "Clearing unsat core 3\n";
        unsat_core.clear();
        for (std::vector< ref<Expr> >::iterator core_it = unsat_core.begin(); core_it != unsat_core.end(); core_it++) {
  	  core_it->get()->dump();
        }
        llvm::errs() << "Ok\n";
        return true;
      }
    }
  } else {
    // FIXME: Which order? one is sure to be better.

    // Look for a satisfying assignment for a superset, which is trivially an
    // assignment for any subset.
    AssignmentCacheWrapper **lookup = 0;
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
      llvm::errs() << "KEY ELEMENTS:\n";
      for (KeyType::iterator it0 = key.begin(); it0 != key.end(); it0++) {
	  it0->get()->dump();
      }
      llvm::errs() << "Ok\n";
      result = (*lookup)->getAssignment();
      if (result == 0) {
        llvm::errs() << "NULL RESULT\n";
      }
      llvm::errs() << "Storing unsat core 4\n";
      unsat_core = (*lookup)->getCore();
      for (std::vector< ref<Expr> >::iterator core_it = unsat_core.begin(); core_it != unsat_core.end(); core_it++) {
	  core_it->get()->dump();
      }
      llvm::errs() << "Ok\n";
      return true;
    }
  }
  
  llvm::errs() << "CexCachingSolver::searchForAssignment returning false\n";
  return false;
}

/// lookupAssignment - Lookup a cached result for the given \arg query.
///
/// \param query - The query to lookup.
/// \param key [out] - On return, the key constructed for the query.
/// \param result [out] - The cached result, if the lookup is succesful. This is
/// either a satisfying assignment (for a satisfiable query), or 0 (for an
/// unsatisfiable query).
/// \return True if a cached result was found.
bool CexCachingSolver::lookupAssignment(const Query &query, 
                                        KeyType &key,
                                        Assignment *&result) {
  key = KeyType(query.constraints.begin(), query.constraints.end());
  ref<Expr> neg = Expr::createIsZero(query.expr);
  bool keyHasAddedConstraint = false;
  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(neg)) {
    if (CE->isFalse()) {
      result = (Assignment*) 0;
      ++stats::queryCexCacheHits;
      return true;
    }
  } else {
    llvm::errs() << "   CexCachingSolver::lookupAssignment: Adding into key: ";
    neg->dump();
    key.insert(neg);
    keyHasAddedConstraint = true;
  }

  llvm::errs() << "Calling CexCachingSolver::searchForAssignment\n";
  bool found = searchForAssignment(key, result);
  if (found)
    ++stats::queryCexCacheHits;
  else ++stats::queryCexCacheMisses;

  if (keyHasAddedConstraint && !unsat_core.empty()) {
      /// Here we remove the added component (neg)
      /// from the unsatisfiability core.
      unsat_core.erase(std::remove(unsat_core.begin(), unsat_core.end(), neg), unsat_core.end());
  }
    
  return found;
}

bool CexCachingSolver::getAssignment(const Query& query, Assignment *&result) {
  llvm::errs() << "CexCachingSolver::getAssignment\n";
  KeyType key;

  llvm::errs() << "CexCachingSolver::getAssignment: Calling lookupAssignment\n";
  if (lookupAssignment(query, key, result))
    return true;

  llvm::errs() << "CexCachingSolver::getAssignment: lookupAssignment did not find an assignment\n";
  std::vector<const Array*> objects;
  findSymbolicObjects(key.begin(), key.end(), objects);

  std::vector< std::vector<unsigned char> > values;
  bool hasSolution;
  llvm::errs() << "CexCachingSolver::getAssignment: calling solver->impl->computeInitialValues\n";
  if (!solver->impl->computeInitialValues(query, objects, values, 
                                          hasSolution))
    return false;
  llvm::errs() << "CexCachingSolver::getAssignment: call to solver->impl->computeInitialValues succeeded\n";
    
  AssignmentCacheWrapper *bindingWrapper;
  Assignment *binding;
  if (hasSolution) {
      llvm::errs() << "CexCachingSolver::getAssignment: solution found\n";
    binding = new Assignment(objects, values);

    // Memoize the result.
    std::pair<assignmentsTable_ty::iterator, bool>
      res = assignmentsTable.insert(binding);
    if (!res.second) {
      delete binding;
      binding = *res.first;
    }
    
    if (DebugCexCacheCheckBinding)
      assert(binding->satisfies(key.begin(), key.end()));

    bindingWrapper = new AssignmentCacheWrapper(binding);
  } else {
    llvm::errs() << "CexCachingSolver::getAssignment: no solution found\n";
    unsat_core = solver->impl->getUnsatCore();
    binding = (Assignment *) 0;
    bindingWrapper = new AssignmentCacheWrapper(unsat_core);
  }
  
  result = binding;
  cache.insert(key, bindingWrapper);

  return true;
}

///

CexCachingSolver::~CexCachingSolver() {
  cache.clear();
  delete solver;
  for (assignmentsTable_ty::iterator it = assignmentsTable.begin(), 
         ie = assignmentsTable.end(); it != ie; ++it)
    delete *it;
}

bool CexCachingSolver::computeValidity(const Query& query,
                                       Solver::Validity &result) {
  llvm::errs() << "CexCachingSolver::computeValidity\n";
  TimerStatIncrementer t(stats::cexCacheTime);
  Assignment *a;

  llvm::errs() << "CexCachingSolver::computeValidity: CONSTRAINTS IN QUERY\n";
  for (ConstraintManager::constraint_iterator it0 = query.constraints.begin(); it0 != query.constraints.end(); it0++) {
      it0->get()->dump();
  }
  llvm::errs() << "Ok\n";

  /// Given query of the form antecedent -> consequent, here we try to
  /// decide if antecedent was satisfiable by attempting to get an
  /// assignment from the validity proof of antecedent -> false.
  llvm::errs() << "CexCachingSolver::computeValidity: Calling getAssignment 1\n";
  if (!getAssignment(query.withFalse(), a))
    return false;

  /// Logically, antecedent must be satisfiable, as we eagerly terminate a
  /// path upon the discovery of unsatisfiability.
  assert(a && "computeValidity() must have assignment");
  ref<Expr> q = a->evaluate(query.expr);
  assert(isa<ConstantExpr>(q) && 
         "assignment evaluation did not result in constant");

  if (cast<ConstantExpr>(q)->isTrue()) {
    /// Antecedent is satisfiable, and its model is also a model of the
    /// consequent, which means that the query: antecedent -> consequent
    /// is potentially valid.
    ///
    /// We next try to establish the validity of the query
    /// antecedent -> consequent itself, and when this was proven invalid,
    /// gets the solution to "antecedent and not consequent", i.e.,
    /// the counterexample, in a.
    llvm::errs() << "CexCachingSolver::computeValidity: Calling getAssignment 2\n";
    if (!getAssignment(query, a))
      return false;                  /// Return false in case of solver error

    /// Return Solver::True in result in case validity is established:
    /// Solver::Unknown otherwise.
    result = !a ? Solver::True : Solver::Unknown;
  } else {
    /// The computed model of the antecedent is not a model of the consequent.
    /// It is possible that the query is false under any interpretation. Here
    /// we try to prove antecedent -> not consequent given the original
    /// query antecedent -> consequent.
    llvm::errs() << "CexCachingSolver::computeValidity: Calling getAssignment 3\n";
    if (!getAssignment(query.negateExpr(), a))
      return false;

    /// If there was no solution, this means that antecedent -> not consequent
    /// is valid, and therefore the original query antecedent -> consequent has
    /// no model. Otherwise, we do not know.
    result = !a ? Solver::False : Solver::Unknown;
  }
  
  return true;
}

bool CexCachingSolver::computeTruth(const Query& query,
                                    bool &isValid) {
  llvm::errs() << "CexCachingSolver::computeTruth\n";
  TimerStatIncrementer t(stats::cexCacheTime);

  // There is a small amount of redundancy here. We only need to know
  // truth and do not really need to compute an assignment. This means
  // that we could check the cache to see if we already know that
  // state ^ query has no assignment. In that case, by the validity of
  // state, we know that state ^ !query must have an assignment, and
  // so query cannot be true (valid). This does get hits, but doesn't
  // really seem to be worth the overhead.

  if (CexCacheExperimental) {
    Assignment *a;
    if (lookupAssignment(query.negateExpr(), a) && !a)
      return false;
  }

  Assignment *a;
  llvm::errs() << "CexCachingSolver::computeTruth: Calling getAssignment\n";
  if (!getAssignment(query, a))
    return false;

  isValid = !a;

  return true;
}

bool CexCachingSolver::computeValue(const Query& query,
                                    ref<Expr> &result) {
  llvm::errs() << "CexCachingSolver::computeValue\n";
  TimerStatIncrementer t(stats::cexCacheTime);

  Assignment *a;
  llvm::errs() << "CexCahingSolver::computeValue: Calling getAssignment\n";
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
  llvm::errs() << "CexCachingSolver::computeInitialValues\n";
  TimerStatIncrementer t(stats::cexCacheTime);
  Assignment *a;
  llvm::errs() << "CexCachingSolver::computeInitialValues: Calling CexCachingSolver::getAssignment\n";
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

char *CexCachingSolver::getConstraintLog(const Query& query) {
  return solver->impl->getConstraintLog(query);
}

void CexCachingSolver::setCoreSolverTimeout(double timeout) {
  solver->impl->setCoreSolverTimeout(timeout);
}

///

Solver *klee::createCexCachingSolver(Solver *_solver) {
  return new Solver(new CexCachingSolver(_solver));
}
