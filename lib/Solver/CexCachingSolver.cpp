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
#include "klee/Solver/SolverImpl.h"
#include "klee/Solver/SolverStats.h"
#include "klee/Statistics/TimerStatIncrementer.h"
#include "klee/Support/ErrorHandling.h"
#include "klee/Support/OptionCategories.h"

#include "llvm/Support/CommandLine.h"

#include <cstddef>
#include <memory>
#include <optional>

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

cl::opt<bool> CexCacheExperimental(
    "cex-cache-exp", cl::init(false),
    cl::desc("Optimization for validity queries (default=false)"),
    cl::cat(SolvingCat));

} // namespace

///

typedef std::set<ref<Expr>> KeyType;

struct AssignmentLessThan {
  bool operator()(const Assignment &a, const Assignment &b) const {
    return a.bindings < b.bindings;
  }
};

class CexCachingSolver : public SolverImpl {
  std::unique_ptr<Solver> solver;

  MapOfSets<ref<Expr>, const Assignment *> cache; // memo table
  std::set<Assignment, AssignmentLessThan> assignmentsTable;

  std::optional<const Assignment *> searchForAssignment(KeyType &key);

  std::optional<const Assignment *> lookupAssignment(const Query &query,
                                                     KeyType &key);

  std::optional<const Assignment *> lookupAssignment(const Query &query) {
    KeyType key;
    return lookupAssignment(query, key);
  }

  std::optional<const Assignment *> getAssignment(const Query &query);

public:
  CexCachingSolver(Solver *_solver) : solver(_solver) {}

  bool computeTruth(const Query &, bool &isValid);
  bool computeValidity(const Query &, Solver::Validity &result);
  bool computeValue(const Query &, ref<Expr> &result);
  bool computeInitialValues(const Query &,
                            const std::vector<const Array *> &objects,
                            std::vector<std::vector<unsigned char>> &values,
                            bool &hasSolution);
  SolverRunStatus getOperationStatusCode();
  char *getConstraintLog(const Query &query);
  void setCoreSolverTimeout(time::Span timeout);
};

///

struct NullAssignment {
  bool operator()(const Assignment *a) const { return a == nullptr; }
};

struct NonNullAssignment {
  bool operator()(const Assignment *a) const { return a != nullptr; }
};

struct NullOrSatisfyingAssignment {
  KeyType &key;

  NullOrSatisfyingAssignment(KeyType &_key) : key(_key) {}

  bool operator()(const Assignment *a) const {
    return !a || a->satisfies(key.begin(), key.end());
  }
};

/// searchForAssignment - Look for a cached solution for a query.
///
/// \param key - The query to look up.
/// \return - Contains a value if an assignment was found. The cached result, if
/// the lookup is successful. This is either a satisfying assignment (for a
/// satisfiable query), or nullptr (for an unsatisfiable query).
std::optional<const Assignment *>
CexCachingSolver::searchForAssignment(KeyType &key) {
  if (const Assignment **lookup = cache.lookup(key)) {
    return {*lookup};
  }

  if (CexCacheTryAll) {
    // Look for a satisfying assignment for a superset, which is trivially an
    // assignment for any subset.
    const Assignment **lookup = nullptr;
    if (CexCacheSuperSet) {
      lookup = cache.findSuperset(key, NonNullAssignment());
    }

    // Otherwise, look for a subset which is unsatisfiable, see below.
    if (!lookup) {
      lookup = cache.findSubset(key, NullAssignment());
    }

    // If either lookup succeeded, then we have a cached solution.
    if (lookup) {
      return {*lookup};
    }

    // Otherwise, iterate through the set of current assignments to see if one
    // of them satisfies the query.
    for (auto &a : assignmentsTable) {
      if (a.satisfies(key.begin(), key.end())) {
        return {&a};
      }
    }
  } else {
    // FIXME: Which order? one is sure to be better.

    // Look for a satisfying assignment for a superset, which is trivially an
    // assignment for any subset.
    const Assignment **lookup = nullptr;
    if (CexCacheSuperSet) {
      lookup = cache.findSuperset(key, NonNullAssignment());
    }

    // Otherwise, look for a subset which is unsatisfiable -- if the subset is
    // unsatisfiable then no additional constraints can produce a valid
    // assignment. While searching subsets, we also explicitly the solutions for
    // satisfiable subsets to see if they solve the current query and return
    // them if so. This is cheap and frequently succeeds.
    if (!lookup) {
      lookup = cache.findSubset(key, NullOrSatisfyingAssignment(key));
    }

    // If either lookup succeeded, then we have a cached solution.
    if (lookup) {
      return {*lookup};
    }
  }

  return {};
}

/// lookupAssignment - Lookup a cached result for the given \arg query.
///
/// \param query - The query to lookup.
/// \param key [out] - On return, the key constructed for the query.
/// \return - Contains a value if an assignment was found. The cached result, if
/// the lookup is successful. This is either a satisfying assignment (for a
/// satisfiable query), or nullptr (for an unsatisfiable query).
std::optional<const Assignment *>
CexCachingSolver::lookupAssignment(const Query &query, KeyType &key) {
  key = KeyType(query.constraints.begin(), query.constraints.end());
  ref<Expr> neg = Expr::createIsZero(query.expr);
  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(neg)) {
    if (CE->isFalse()) {
      ++stats::queryCexCacheHits;
      return {nullptr};
    }
  } else {
    key.insert(neg);
  }

  auto found = searchForAssignment(key);
  if (found) {
    ++stats::queryCexCacheHits;
  } else {
    ++stats::queryCexCacheMisses;
  }

  return found;
}

std::optional<const Assignment *>
CexCachingSolver::getAssignment(const Query &query) {
  KeyType key;
  if (auto result = lookupAssignment(query, key)) {
    return result;
  }

  std::vector<const Array *> objects;
  findSymbolicObjects(key.begin(), key.end(), objects);

  std::vector<std::vector<unsigned char>> values;
  bool hasSolution;
  if (!solver->impl->computeInitialValues(query, objects, values,
                                          hasSolution)) {
    return {};
  }

  const Assignment *result = nullptr;
  if (hasSolution) {
    // Memoize the result.
    auto it = assignmentsTable.emplace(objects, values).first;

    if (DebugCexCacheCheckBinding)
      if (!it->satisfies(key.begin(), key.end())) {
        query.dump();
        it->dump();
        klee_error("Generated assignment doesn't match query");
      }

    result = &*it;
  }

  cache.insert(key, result);
  return {result};
}

///

bool CexCachingSolver::computeValidity(const Query &query,
                                       Solver::Validity &result) {
  TimerStatIncrementer t(stats::cexCacheTime);
  auto a = getAssignment(query.withFalse());
  if (!a) {
    return false;
  }
  assert(*a && "computeValidity() must have assignment");
  ref<Expr> q = (*a)->evaluate(query.expr);
  assert(isa<ConstantExpr>(q) &&
         "assignment evaluation did not result in constant");

  if (cast<ConstantExpr>(q)->isTrue()) {
    a = getAssignment(query);
    if (!a) {
      return false;
    }
    result = !*a ? Solver::True : Solver::Unknown;
  } else {
    a = getAssignment(query.negateExpr());
    if (!a) {
      return false;
    }
    result = !*a ? Solver::False : Solver::Unknown;
  }

  return true;
}

bool CexCachingSolver::computeTruth(const Query &query, bool &isValid) {
  TimerStatIncrementer t(stats::cexCacheTime);

  // There is a small amount of redundancy here. We only need to know
  // truth and do not really need to compute an assignment. This means
  // that we could check the cache to see if we already know that
  // state ^ query has no assignment. In that case, by the validity of
  // state, we know that state ^ !query must have an assignment, and
  // so query cannot be true (valid). This does get hits, but doesn't
  // really seem to be worth the overhead.

  if (CexCacheExperimental) {
    auto a = lookupAssignment(query.negateExpr());
    if (a && !*a) {
      return false;
    }
  }

  auto a = getAssignment(query);
  if (!a) {
    return false;
  }

  isValid = !*a;

  return true;
}

bool CexCachingSolver::computeValue(const Query &query, ref<Expr> &result) {
  TimerStatIncrementer t(stats::cexCacheTime);

  auto a = getAssignment(query.withFalse());
  if (!a) {
    return false;
  }
  assert(*a && "computeValue() must have assignment");
  result = (*a)->evaluate(query.expr);
  assert(isa<ConstantExpr>(result) &&
         "assignment evaluation did not result in constant");
  return true;
}

bool CexCachingSolver::computeInitialValues(
    const Query &query, const std::vector<const Array *> &objects,
    std::vector<std::vector<unsigned char>> &values, bool &hasSolution) {
  TimerStatIncrementer t(stats::cexCacheTime);
  auto a = getAssignment(query);
  if (!a) {
    return false;
  }
  hasSolution = !!*a;

  if (!*a) {
    return true;
  }

  // FIXME: We should use smarter assignment for result so we don't
  // need redundant copy.
  values = std::vector<std::vector<unsigned char>>(objects.size());
  for (std::size_t i = 0; i < objects.size(); ++i) {
    const Array *os = objects[i];
    auto it = (*a)->bindings.find(os);

    if (it == (*a)->bindings.end()) {
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

char *CexCachingSolver::getConstraintLog(const Query &query) {
  return solver->impl->getConstraintLog(query);
}

void CexCachingSolver::setCoreSolverTimeout(time::Span timeout) {
  solver->impl->setCoreSolverTimeout(timeout);
}

///

Solver *klee::createCexCachingSolver(Solver *_solver) {
  return new Solver(new CexCachingSolver(_solver));
}
