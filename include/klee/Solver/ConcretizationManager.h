#ifndef KLEE_CONCRETIZATIONMANAGER_H
#define KLEE_CONCRETIZATIONMANAGER_H

#include "klee/ADT/MapOfSets.h"
#include "klee/Expr/Assignment.h"
#include "klee/Expr/Constraints.h"
#include <unordered_map>

namespace klee {
struct CacheEntry;
struct CacheEntryHash;
struct Query;

class ConcretizationManager {
private:
  struct CacheEntry {
  public:
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
  public:
    unsigned operator()(const CacheEntry &ce) const {
      unsigned result = ce.query->hash();

      for (auto const &constraint : ce.constraints) {
        result ^= constraint->hash();
      }

      return result;
    }
  };

  typedef std::unordered_map<CacheEntry, const Assignment, CacheEntryHash>
      concretizations_map;
  concretizations_map concretizations;

  bool simplifyExprs;

public:
  ConcretizationManager(bool simplifyExprs) : simplifyExprs(simplifyExprs) {}

  Assignment get(const ConstraintSet &set, ref<Expr> query);
  bool contains(const ConstraintSet &set, ref<Expr> query);
  void add(const Query &q, const Assignment &assign);
};

}; // namespace klee

#endif /* KLEE_CONCRETIZATIONMANAGER_H */
