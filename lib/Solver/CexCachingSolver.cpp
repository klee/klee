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
#include "klee/Expr/Constraints.h"
#include "klee/Expr/Expr.h"
#include "klee/Expr/ExprUtil.h"
#include "klee/Solver/SolverImpl.h"
#include "klee/Solver/SolverStats.h"
#include "klee/Statistics/TimerStatIncrementer.h"
#include "klee/Support/ErrorHandling.h"
#include "klee/Support/OptionCategories.h"

#include "klee/Support/CompilerWarning.h"
DISABLE_WARNING_PUSH
DISABLE_WARNING_DEPRECATED_DECLARATIONS
#include "llvm/Support/CommandLine.h"
DISABLE_WARNING_POP

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

cl::opt<bool> CexCacheExperimental(
    "cex-cache-exp", cl::init(false),
    cl::desc("Optimization for validity queries (default=false)"),
    cl::cat(SolvingCat));

cl::opt<bool> CexCacheValidityCores(
    "cex-cache-validity-cores", cl::init(false),
    cl::desc("Cache assignment and it's validity cores (default=false)"),
    cl::cat(SolvingCat));

} // namespace

///

typedef std::set<ref<Expr>> KeyType;

struct ResponseComparator {
  bool operator()(const ref<SolverResponse> a,
                  const ref<SolverResponse> b) const {
    return *a < *b;
  }
};

class CexCachingSolver : public SolverImpl {
  typedef std::set<ref<SolverResponse>, ResponseComparator> responseTable_ty;

  std::unique_ptr<Solver> solver;

  MapOfSets<ref<Expr>, ref<SolverResponse>> cache;
  // memo table
  responseTable_ty responseTable;

  bool searchForResponse(KeyType &key, ref<SolverResponse> &result);

  bool lookupResponse(const Query &query, KeyType &key,
                      ref<SolverResponse> &result);

  bool lookupResponse(const Query &query, ref<SolverResponse> &result) {
    KeyType key;
    return lookupResponse(query, key, result);
  }

  bool getResponse(const Query &query, ref<SolverResponse> &result);

public:
  CexCachingSolver(std::unique_ptr<Solver> solver)
      : solver(std::move(solver)) {}
  ~CexCachingSolver();

  bool computeTruth(const Query &, bool &isValid);
  bool computeValidity(const Query &, PartialValidity &result);
  bool computeValue(const Query &, ref<Expr> &result);
  bool computeInitialValues(const Query &,
                            const std::vector<const Array *> &objects,
                            std::vector<SparseStorage<unsigned char>> &values,
                            bool &hasSolution);
  bool check(const Query &query, ref<SolverResponse> &result);
  bool computeValidityCore(const Query &, ValidityCore &validityCore,
                           bool &isValid);
  SolverRunStatus getOperationStatusCode();
  char *getConstraintLog(const Query &query);
  void setCoreSolverTimeout(time::Span timeout);
  void notifyStateTermination(std::uint32_t id);
};

///

struct isValidResponse {
  bool operator()(ref<SolverResponse> a) const { return isa<ValidResponse>(a); }
};

struct isInvalidResponse {
  bool operator()(ref<SolverResponse> a) const {
    return isa<InvalidResponse>(a);
  }
};

struct isValidOrSatisfyingResponse {
  KeyType key;
  isValidOrSatisfyingResponse(KeyType &_key) : key(_key) {}

  bool operator()(ref<SolverResponse> a) const {
    return isa<ValidResponse>(a) || (isa<InvalidResponse>(a) &&
                                     cast<InvalidResponse>(a)->satisfies(key));
  }
};

/// searchForResponse - Look for a cached solution for a query.
///
/// \param key - The query to look up.
/// \param result [out] - The cached result, if the lookup is successful. This
/// is either a satisfying invalid response (for a satisfiable query), or valid
/// response (for an unsatisfiable query). \return - True if a cached result was
/// found.
bool CexCachingSolver::searchForResponse(KeyType &key,
                                         ref<SolverResponse> &result) {
  const ref<SolverResponse> *lookup = cache.lookup(key);
  if (lookup) {
    result = *lookup;
    return true;
  }

  if (CexCacheTryAll) {
    // Look for a satisfying invalid response for a superset, which is trivially
    // a response for any subset.
    ref<SolverResponse> *lookup = 0;
    if (CexCacheSuperSet)
      lookup = cache.findSuperset(key, isInvalidResponse());

    // Otherwise, look for a subset which is unsatisfiable, see below.
    if (!lookup)
      lookup = cache.findSubset(key, isValidResponse());

    // If either lookup succeeded, then we have a cached solution.
    if (lookup) {
      result = *lookup;
      return true;
    }

    // Otherwise, iterate through the set of current solver responses to see if
    // one of them satisfies the query.
    for (responseTable_ty::iterator it = responseTable.begin(),
                                    ie = responseTable.end();
         it != ie; ++it) {
      ref<SolverResponse> a = *it;
      if (isa<InvalidResponse>(a) &&
          cast<InvalidResponse>(a)->satisfiesOrConstant(key)) {
        result = a;
        return true;
      }
    }
  } else {
    // FIXME: Which order? one is sure to be better.

    // Look for a satisfying invalid response for a superset, which is trivially
    // a response for any subset.
    ref<SolverResponse> *lookup = 0;
    if (CexCacheSuperSet)
      lookup = cache.findSuperset(key, isInvalidResponse());

    // Otherwise, look for a subset which is unsatisfiable -- if the subset is
    // unsatisfiable then no additional constraints can produce a valid
    // solver response. While searching subsets, we also explicitly the
    // solutions for satisfiable subsets to see if they solve the current query
    // and return them if so. This is cheap and frequently succeeds.
    if (!lookup)
      lookup = cache.findSubset(key, isValidOrSatisfyingResponse(key));

    // If either lookup succeeded, then we have a cached solution.
    if (lookup) {
      result = *lookup;
      return true;
    }
  }

  return false;
}

/// lookupResponse - Lookup a cached result for the given \arg query.
///
/// \param query - The query to lookup.
/// \param key [out] - On return, the key constructed for the query.
/// \param result [out] - The cached result, if the lookup is successful. This
/// is either a InvalidResponse (for a satisfiable query), or ValidResponse (for
/// an unsatisfiable query). \return True if a cached result was found.
bool CexCachingSolver::lookupResponse(const Query &query, KeyType &key,
                                      ref<SolverResponse> &result) {
  key = KeyType(query.constraints.cs().begin(), query.constraints.cs().end());
  for (ref<Symcrete> s : query.constraints.symcretes()) {
    key.insert(s->symcretized);
  }
  ref<Expr> neg = Expr::createIsZero(query.expr);
  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(neg)) {
    if (CE->isFalse()) {
      result = new ValidResponse(ValidityCore());
      ++stats::queryCexCacheHits;
      return true;
    }
  } else {
    key.insert(neg);
  }

  bool found = searchForResponse(key, result);
  if (found)
    ++stats::queryCexCacheHits;
  else
    ++stats::queryCexCacheMisses;

  return found;
}

bool CexCachingSolver::getResponse(const Query &query,
                                   ref<SolverResponse> &result) {
  KeyType key;
  if (lookupResponse(query, key, result)) {
    return true;
  }

  if (!solver->impl->check(query, result)) {
    return false;
  }

  if (isa<InvalidResponse>(result)) {
    // Memorize the result.
    std::pair<responseTable_ty::iterator, bool> res =
        responseTable.insert(result);
    if (!res.second) {
      result = *res.first;
    }

    if (DebugCexCacheCheckBinding) {
      if (!cast<InvalidResponse>(result)->satisfiesOrConstant(key)) {
        query.dump();
        result->dump();
        klee_error("Generated assignment doesn't match query");
      }
    }
  }

  ValidityCore resultCore;
  if (CexCacheValidityCores && isa<ValidResponse>(result)) {
    result->tryGetValidityCore(resultCore);
    KeyType resultCoreConstarints(resultCore.constraints.begin(),
                                  resultCore.constraints.end());
    ref<Expr> neg = Expr::createIsZero(resultCore.expr);
    resultCoreConstarints.insert(neg);
    cache.insert(resultCoreConstarints, result);
  }
  if (isa<ValidResponse>(result) || isa<InvalidResponse>(result)) {
    cache.insert(key, result);
  }

  return true;
}

///

CexCachingSolver::~CexCachingSolver() { cache.clear(); }

bool CexCachingSolver::computeValidity(const Query &query,
                                       PartialValidity &result) {
  TimerStatIncrementer t(stats::cexCacheTime);
  ref<SolverResponse> a;
  ref<Expr> q;
  if (!computeValue(query, q))
    return false;

  if (cast<ConstantExpr>(q)->isTrue()) {
    bool success = getResponse(query, a);
    if (success && isa<ValidResponse>(a)) {
      result = PValidity::MustBeTrue;
    } else if (success && isa<InvalidResponse>(a)) {
      result = PValidity::TrueOrFalse;
    } else {
      result = PValidity::MayBeTrue;
    }
  } else {
    bool success = getResponse(query.negateExpr(), a);
    if (success && isa<ValidResponse>(a)) {
      result = PValidity::MustBeFalse;
    } else if (success && isa<InvalidResponse>(a)) {
      result = PValidity::TrueOrFalse;
    } else {
      result = PValidity::MayBeFalse;
    }
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
    ref<SolverResponse> a;
    if (lookupResponse(query.negateExpr(), a) && isa<ValidResponse>(a))
      return false;
  }

  ref<SolverResponse> a;
  if (!getResponse(query, a))
    return false;

  isValid = !isa<InvalidResponse>(a);

  return true;
}

bool CexCachingSolver::computeValue(const Query &query, ref<Expr> &result) {
  TimerStatIncrementer t(stats::cexCacheTime);

  ref<SolverResponse> a;
  if (!query.constraints.cs().empty()) {
    if (!getResponse(query.withFalse(), a))
      return false;
  } else {
    a = new InvalidResponse();
  }
  assert(isa<InvalidResponse>(a) && "computeValue() must have assignment");
  result = cast<InvalidResponse>(a)->evaluate(query.expr, false);

  if (!isa<ConstantExpr>(result) && solver->impl->computeValue(query, result))
    return false;

  assert(isa<ConstantExpr>(result) &&
         "assignment evaluation did not result in constant");
  return true;
}

bool CexCachingSolver::computeInitialValues(
    const Query &query, const std::vector<const Array *> &objects,
    std::vector<SparseStorage<unsigned char>> &values, bool &hasSolution) {
  TimerStatIncrementer t(stats::cexCacheTime);
  ref<SolverResponse> a;
  if (!getResponse(query, a))
    return false;
  hasSolution = isa<InvalidResponse>(a);

  if (!hasSolution)
    return true;

  // FIXME: We should use smarter assignment for result so we don't
  // need redundant copy.
  values = std::vector<SparseStorage<unsigned char>>(objects.size());
  Assignment::bindings_ty aBindings;
  a->tryGetInitialValues(aBindings);

  for (unsigned i = 0; i < objects.size(); ++i) {
    const Array *os = objects[i];
    Assignment::bindings_ty::iterator it = aBindings.find(os);

    if (it == aBindings.end()) {
      ref<ConstantExpr> arrayConstantSize =
          cast<InvalidResponse>(a)->evaluate(os->size);
      assert(arrayConstantSize &&
             "Array of symbolic size had not receive value for size!");
      values[i] = SparseStorage<unsigned char>(0);
    } else {
      values[i] = it->second;
    }
  }

  return true;
}

bool CexCachingSolver::check(const Query &query, ref<SolverResponse> &result) {
  TimerStatIncrementer t(stats::cexCacheTime);
  return getResponse(query, result);
}

bool CexCachingSolver::computeValidityCore(const Query &query,
                                           ValidityCore &validityCore,
                                           bool &isValid) {
  TimerStatIncrementer t(stats::cexCacheTime);

  ref<SolverResponse> a;
  if (!getResponse(query, a))
    return false;

  isValid = isa<ValidResponse>(a);
  if (isValid)
    cast<ValidResponse>(a)->tryGetValidityCore(validityCore);
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

void CexCachingSolver::notifyStateTermination(std::uint32_t id) {
  solver->impl->notifyStateTermination(id);
}

///

std::unique_ptr<Solver>
klee::createCexCachingSolver(std::unique_ptr<Solver> solver) {
  return std::make_unique<Solver>(
      std::make_unique<CexCachingSolver>(std::move(solver)));
}
