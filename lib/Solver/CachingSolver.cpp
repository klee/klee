//===-- CachingSolver.cpp - Caching expression solver ---------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Solver/Solver.h"

#include "klee/Expr/Constraints.h"
#include "klee/Expr/Expr.h"
#include "klee/Solver/IncompleteSolver.h"
#include "klee/Solver/SolverImpl.h"
#include "klee/Solver/SolverStats.h"

#include <unordered_map>

using namespace klee;

class CachingSolver : public SolverImpl {
private:
  ref<Expr> canonicalizeQuery(ref<Expr> originalQuery, bool &negationUsed);

  void cacheInsert(const Query &query,
                   IncompleteSolver::PartialValidity result);

  bool cacheLookup(const Query &query,
                   IncompleteSolver::PartialValidity &result);

  void validityCoreCacheInsert(const Query &query,
                               ValidityCore validityCoreResult);

  bool validityCoreCacheLookup(const Query &query,
                               ValidityCore &validityCoreResult);

  struct CacheEntry {
    CacheEntry(const ConstraintSet &c, ref<Expr> q)
        : constraints(c), query(q) {}

    CacheEntry(const CacheEntry &ce)
        : constraints(ce.constraints), query(ce.query) {}

    ConstraintSet constraints;
    ref<Expr> query;

    bool operator==(const CacheEntry &b) const {
      return constraints == b.constraints && *query.get() == *b.query.get();
    }
  };

  struct CacheEntryHash {
    unsigned operator()(const CacheEntry &ce) const {
      unsigned result = ce.query->hash();

      for (auto const &constraint : ce.constraints) {
        result ^= constraint->hash();
      }

      return result;
    }
  };

  typedef std::unordered_map<CacheEntry, IncompleteSolver::PartialValidity,
                             CacheEntryHash>
      cache_map;
  typedef std::unordered_map<CacheEntry, ValidityCore, CacheEntryHash>
      validity_core_cache_map;

  Solver *solver;
  cache_map cache;
  validity_core_cache_map validityCoreCache;

public:
  CachingSolver(Solver *s) : solver(s) {}
  ~CachingSolver() {
    cache.clear();
    delete solver;
  }

  bool computeValidity(const Query &, Solver::Validity &result);
  bool computeTruth(const Query &, bool &isValid);
  bool computeValue(const Query &query, ref<Expr> &result) {
    ++stats::queryCacheMisses;
    return solver->impl->computeValue(query, result);
  }
  bool computeInitialValues(const Query &query,
                            const std::vector<const Array *> &objects,
                            std::vector<SparseStorage<unsigned char>> &values,
                            bool &hasSolution);
  bool check(const Query &query, ref<SolverResponse> &result);
  bool computeValidityCore(const Query &, ValidityCore &validityCore,
                           bool &isValid);
  SolverRunStatus getOperationStatusCode();
  char *getConstraintLog(const Query &);
  void setCoreSolverTimeout(time::Span timeout);
};

/** @returns the canonical version of the given query.  The reference
    negationUsed is set to true if the original query was negated in
    the canonicalization process. */
ref<Expr> CachingSolver::canonicalizeQuery(ref<Expr> originalQuery,
                                           bool &negationUsed) {
  ref<Expr> negatedQuery = Expr::createIsZero(originalQuery);

  // select the "smaller" query to the be canonical representation
  if (originalQuery.compare(negatedQuery) < 0) {
    negationUsed = false;
    return originalQuery;
  } else {
    negationUsed = true;
    return negatedQuery;
  }
}

/** @returns true on a cache hit, false of a cache miss.  Reference
    value result only valid on a cache hit. */
bool CachingSolver::cacheLookup(const Query &query,
                                IncompleteSolver::PartialValidity &result) {
  bool negationUsed;
  ref<Expr> canonicalQuery = canonicalizeQuery(query.expr, negationUsed);

  CacheEntry ce(query.constraints, canonicalQuery);
  cache_map::iterator it = cache.find(ce);

  if (it != cache.end()) {
    result = (negationUsed ? IncompleteSolver::negatePartialValidity(it->second)
                           : it->second);
    return true;
  }

  return false;
}

bool CachingSolver::validityCoreCacheLookup(const Query &query,
                                            ValidityCore &result) {
  bool negationUsed;
  ref<Expr> canonicalQuery = canonicalizeQuery(query.expr, negationUsed);

  CacheEntry ce(query.constraints, canonicalQuery);
  validity_core_cache_map::iterator it = validityCoreCache.find(ce);

  if (it != validityCoreCache.end()) {
    result = (negationUsed ? it->second.negateExpr() : it->second);
    return true;
  }

  return false;
}

/// Inserts the given query, result pair into the cache.
void CachingSolver::cacheInsert(const Query &query,
                                IncompleteSolver::PartialValidity result) {
  bool negationUsed;
  ref<Expr> canonicalQuery = canonicalizeQuery(query.expr, negationUsed);

  CacheEntry ce(query.constraints, canonicalQuery);
  IncompleteSolver::PartialValidity cachedResult =
      (negationUsed ? IncompleteSolver::negatePartialValidity(result) : result);

  cache.insert(std::make_pair(ce, cachedResult));
}

void CachingSolver::validityCoreCacheInsert(const Query &query,
                                            ValidityCore validityCoreResult) {
  bool negationUsed;
  ref<Expr> canonicalQuery = canonicalizeQuery(query.expr, negationUsed);

  CacheEntry ce(query.constraints, canonicalQuery);
  ValidityCore cachedValidityCoreResult =
      (negationUsed ? validityCoreResult.negateExpr() : validityCoreResult);

  validityCoreCache.insert(std::make_pair(ce, cachedValidityCoreResult));
}

bool CachingSolver::computeValidity(const Query &query,
                                    Solver::Validity &result) {
  IncompleteSolver::PartialValidity cachedResult;
  bool tmp, cacheHit = cacheLookup(query, cachedResult);

  if (cacheHit) {
    switch (cachedResult) {
    case IncompleteSolver::MustBeTrue:
      result = Solver::True;
      ++stats::queryCacheHits;
      return true;
    case IncompleteSolver::MustBeFalse:
      result = Solver::False;
      ++stats::queryCacheHits;
      return true;
    case IncompleteSolver::TrueOrFalse:
      result = Solver::Unknown;
      ++stats::queryCacheHits;
      return true;
    case IncompleteSolver::MayBeTrue: {
      ++stats::queryCacheMisses;
      if (!solver->impl->computeTruth(query, tmp))
        return false;
      if (tmp) {
        cacheInsert(query, IncompleteSolver::MustBeTrue);
        result = Solver::True;
        return true;
      } else {
        cacheInsert(query, IncompleteSolver::TrueOrFalse);
        result = Solver::Unknown;
        return true;
      }
    }
    case IncompleteSolver::MayBeFalse: {
      ++stats::queryCacheMisses;
      if (!solver->impl->computeTruth(query.negateExpr(), tmp))
        return false;
      if (tmp) {
        cacheInsert(query, IncompleteSolver::MustBeFalse);
        result = Solver::False;
        return true;
      } else {
        cacheInsert(query, IncompleteSolver::TrueOrFalse);
        result = Solver::Unknown;
        return true;
      }
    }
    default:
      assert(0 && "unreachable");
    }
  }

  ++stats::queryCacheMisses;

  if (!solver->impl->computeValidity(query, result))
    return false;

  switch (result) {
  case Solver::True:
    cachedResult = IncompleteSolver::MustBeTrue;
    break;
  case Solver::False:
    cachedResult = IncompleteSolver::MustBeFalse;
    break;
  default:
    cachedResult = IncompleteSolver::TrueOrFalse;
    break;
  }

  cacheInsert(query, cachedResult);
  return true;
}

bool CachingSolver::computeTruth(const Query &query, bool &isValid) {
  IncompleteSolver::PartialValidity cachedResult;
  bool cacheHit = cacheLookup(query, cachedResult);

  // a cached result of MayBeTrue forces us to check whether
  // a False assignment exists.
  if (cacheHit && cachedResult != IncompleteSolver::MayBeTrue) {
    ++stats::queryCacheHits;
    isValid = (cachedResult == IncompleteSolver::MustBeTrue);
    return true;
  }

  ++stats::queryCacheMisses;

  // cache miss: query solver
  if (!solver->impl->computeTruth(query, isValid))
    return false;

  if (isValid) {
    cachedResult = IncompleteSolver::MustBeTrue;
  } else if (cacheHit) {
    // We know a true assignment exists, and query isn't valid, so
    // must be TrueOrFalse.
    assert(cachedResult == IncompleteSolver::MayBeTrue);
    cachedResult = IncompleteSolver::TrueOrFalse;
  } else {
    cachedResult = IncompleteSolver::MayBeFalse;
  }

  cacheInsert(query, cachedResult);
  return true;
}

bool CachingSolver::computeValidityCore(const Query &query,
                                        ValidityCore &validityCore,
                                        bool &isValid) {
  IncompleteSolver::PartialValidity cachedResult;
  bool tmp, cacheHit = cacheLookup(query, cachedResult);

  // a cached result of MayBeTrue forces us to check whether
  // a False assignment exists.
  if (cacheHit && cachedResult != IncompleteSolver::MayBeTrue) {
    ValidityCore cachedValidityCore;
    cacheHit = validityCoreCacheLookup(query, cachedValidityCore);
    if (cacheHit && cachedResult == IncompleteSolver::MustBeTrue) {
      ++stats::queryCacheHits;
      validityCore = cachedValidityCore;
    } else if (cachedResult == IncompleteSolver::MustBeTrue) {
      ++stats::queryCacheMisses;
      if (!solver->impl->computeValidityCore(query, validityCore, tmp))
        return false;
      assert(tmp && "Query must be true!");
      validityCoreCacheInsert(query, validityCore);
    } else {
      ++stats::queryCacheHits;
    }
    isValid = (cachedResult == IncompleteSolver::MustBeTrue);
    return true;
  }

  ++stats::queryCacheMisses;

  // cache miss: query solver
  if (!solver->impl->computeValidityCore(query, validityCore, isValid))
    return false;

  if (isValid) {
    cachedResult = IncompleteSolver::MustBeTrue;
    validityCoreCacheInsert(query, validityCore);
  } else if (cacheHit) {
    // We know a true assignment exists, and query isn't valid, so
    // must be TrueOrFalse.
    assert(cachedResult == IncompleteSolver::MayBeTrue);
    cachedResult = IncompleteSolver::TrueOrFalse;
  } else {
    cachedResult = IncompleteSolver::MayBeFalse;
  }

  cacheInsert(query, cachedResult);
  return true;
}

bool CachingSolver::computeInitialValues(
    const Query &query, const std::vector<const Array *> &objects,
    std::vector<SparseStorage<unsigned char>> &values, bool &hasSolution) {
  ++stats::queryCacheMisses;
  return solver->impl->computeInitialValues(query, objects, values,
                                            hasSolution);
}

bool CachingSolver::check(const Query &query, ref<SolverResponse> &result) {
  IncompleteSolver::PartialValidity cachedResult;
  bool tmp, cacheHit = cacheLookup(query, cachedResult);

  // a cached result of MayBeTrue forces us to check whether
  // a False assignment exists.
  if (cacheHit && cachedResult != IncompleteSolver::MayBeTrue) {
    ValidityCore cachedValidityCore;
    cacheHit = validityCoreCacheLookup(query, cachedValidityCore);
    if (cacheHit && cachedResult == IncompleteSolver::MustBeTrue) {
      ++stats::queryCacheHits;
      result = new ValidResponse(cachedValidityCore);
    } else if (cachedResult == IncompleteSolver::MustBeTrue) {
      ++stats::queryCacheMisses;
      if (!solver->impl->computeValidityCore(query, cachedValidityCore, tmp))
        return false;
      result = new ValidResponse(cachedValidityCore);
      assert(tmp && "Query must be true!");
    } else {
      ++stats::queryCacheMisses;
      if (!solver->impl->check(query, result))
        return false;
    }
    return true;
  }

  ++stats::queryCacheMisses;

  // cache miss: query solver
  if (!solver->impl->check(query, result))
    return false;

  if (isa<ValidResponse>(result)) {
    cachedResult = IncompleteSolver::MustBeTrue;
  } else if (cacheHit) {
    // We know a true assignment exists, and query isn't valid, so
    // must be TrueOrFalse.
    assert(cachedResult == IncompleteSolver::MayBeTrue);
    cachedResult = IncompleteSolver::TrueOrFalse;
  } else {
    cachedResult = IncompleteSolver::MayBeFalse;
  }

  cacheInsert(query, cachedResult);
  return true;
}

SolverImpl::SolverRunStatus CachingSolver::getOperationStatusCode() {
  return solver->impl->getOperationStatusCode();
}

char *CachingSolver::getConstraintLog(const Query &query) {
  return solver->impl->getConstraintLog(query);
}

void CachingSolver::setCoreSolverTimeout(time::Span timeout) {
  solver->impl->setCoreSolverTimeout(timeout);
}

///

Solver *klee::createCachingSolver(Solver *_solver) {
  return new Solver(new CachingSolver(_solver));
}
