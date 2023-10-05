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
#include "klee/Solver/SolverImpl.h"
#include "klee/Solver/SolverStats.h"

#include <memory>
#include <unordered_map>
#include <utility>

using namespace klee;

class CachingSolver : public SolverImpl {
private:
  ref<Expr> canonicalizeQuery(ref<Expr> originalQuery, bool &negationUsed);

  void cacheInsert(const Query &query, PartialValidity result);

  bool cacheLookup(const Query &query, PartialValidity &result);

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

      for (auto const &constraint : ce.constraints.cs()) {
        result ^= constraint->hash();
      }

      return result;
    }
  };

  typedef std::unordered_map<CacheEntry, PartialValidity, CacheEntryHash>
      cache_map;
  typedef std::unordered_map<CacheEntry, ValidityCore, CacheEntryHash>
      validity_core_cache_map;

  std::unique_ptr<Solver> solver;
  cache_map cache;
  validity_core_cache_map validityCoreCache;

public:
  CachingSolver(std::unique_ptr<Solver> solver) : solver(std::move(solver)) {}

  bool computeValidity(const Query &, PartialValidity &result);
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
  void notifyStateTermination(std::uint32_t id);
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
bool CachingSolver::cacheLookup(const Query &query, PartialValidity &result) {
  bool negationUsed;
  ref<Expr> canonicalQuery = canonicalizeQuery(query.expr, negationUsed);

  CacheEntry ce(query.constraints, canonicalQuery);
  cache_map::iterator it = cache.find(ce);

  if (it != cache.end()) {
    result = (negationUsed ? negatePartialValidity(it->second) : it->second);
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
void CachingSolver::cacheInsert(const Query &query, PartialValidity result) {
  bool negationUsed;
  ref<Expr> canonicalQuery = canonicalizeQuery(query.expr, negationUsed);

  CacheEntry ce(query.constraints, canonicalQuery);
  PartialValidity cachedResult =
      (negationUsed ? negatePartialValidity(result) : result);

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
                                    PartialValidity &result) {
  PartialValidity cachedResult;
  bool tmp, cacheHit = cacheLookup(query, cachedResult);

  if (cacheHit) {
    switch (cachedResult) {
    case PValidity::MustBeTrue:
      result = PValidity::MustBeTrue;
      ++stats::queryCacheHits;
      return true;
    case PValidity::MustBeFalse:
      result = PValidity::MustBeFalse;
      ++stats::queryCacheHits;
      return true;
    case PValidity::TrueOrFalse:
      result = PValidity::TrueOrFalse;
      ++stats::queryCacheHits;
      return true;
    case PValidity::MayBeTrue: {
      ++stats::queryCacheMisses;
      bool success = solver->impl->computeTruth(query, tmp);
      if (success && tmp) {
        cacheInsert(query, PValidity::MustBeTrue);
        result = PValidity::MustBeTrue;
        return true;
      } else if (success && !tmp) {
        cacheInsert(query, PValidity::TrueOrFalse);
        result = PValidity::TrueOrFalse;
        return true;
      } else {
        cacheInsert(query, PValidity::MayBeTrue);
        result = PValidity::MayBeTrue;
        return true;
      }
    }
    case PValidity::MayBeFalse: {
      ++stats::queryCacheMisses;
      bool success = solver->impl->computeTruth(query.negateExpr(), tmp);
      if (success && tmp) {
        cacheInsert(query, PValidity::MustBeFalse);
        result = PValidity::MustBeFalse;
        return true;
      } else if (success && !tmp) {
        cacheInsert(query, PValidity::TrueOrFalse);
        result = PValidity::TrueOrFalse;
        return true;
      } else {
        cacheInsert(query, PValidity::MayBeFalse);
        result = PValidity::MayBeFalse;
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

  cachedResult = result;
  assert(cachedResult != PValidity::None);

  cacheInsert(query, cachedResult);

  return true;
}

bool CachingSolver::computeTruth(const Query &query, bool &isValid) {
  PartialValidity cachedResult;
  bool cacheHit = cacheLookup(query, cachedResult);

  // a cached result of MayBeTrue forces us to check whether
  // a False assignment exists.
  if (cacheHit && cachedResult != PValidity::MayBeTrue) {
    ++stats::queryCacheHits;
    isValid = (cachedResult == PValidity::MustBeTrue);
    return true;
  }

  ++stats::queryCacheMisses;

  // cache miss: query solver
  if (!solver->impl->computeTruth(query, isValid))
    return false;

  if (isValid) {
    cachedResult = PValidity::MustBeTrue;
  } else if (cacheHit) {
    // We know a true assignment exists, and query isn't valid, so
    // must be TrueOrFalse.
    assert(cachedResult == PValidity::MayBeTrue);
    cachedResult = PValidity::TrueOrFalse;
  } else {
    cachedResult = PValidity::MayBeFalse;
  }

  cacheInsert(query, cachedResult);
  return true;
}

bool CachingSolver::computeValidityCore(const Query &query,
                                        ValidityCore &validityCore,
                                        bool &isValid) {
  ++stats::queryCacheMisses;
  return solver->impl->computeValidityCore(query, validityCore, isValid);
}

bool CachingSolver::computeInitialValues(
    const Query &query, const std::vector<const Array *> &objects,
    std::vector<SparseStorage<unsigned char>> &values, bool &hasSolution) {
  ++stats::queryCacheMisses;
  return solver->impl->computeInitialValues(query, objects, values,
                                            hasSolution);
}

bool CachingSolver::check(const Query &query, ref<SolverResponse> &result) {
  ++stats::queryCacheMisses;
  return solver->impl->check(query, result);
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

void CachingSolver::notifyStateTermination(std::uint32_t id) {
  solver->impl->notifyStateTermination(id);
}

///

std::unique_ptr<Solver>
klee::createCachingSolver(std::unique_ptr<Solver> solver) {
  return std::make_unique<Solver>(
      std::make_unique<CachingSolver>(std::move(solver)));
}
