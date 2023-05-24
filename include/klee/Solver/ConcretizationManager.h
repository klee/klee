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
  using cs_entry = constraints_ty;
  using sm_entry = symcretes_ty;

private:
  struct CacheEntry {
  public:
    CacheEntry(const cs_entry &c, const sm_entry &s, ref<Expr> q)
        : constraints(c), symcretes(s), query(q) {}

    CacheEntry(const CacheEntry &ce)
        : constraints(ce.constraints), symcretes(ce.symcretes),
          query(ce.query) {}

    cs_entry constraints;
    sm_entry symcretes;
    ref<Expr> query;

    bool operator==(const CacheEntry &b) const {
      return constraints == b.constraints && symcretes == b.symcretes &&
             query == b.query;
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

  struct CacheEntryCmp {
  public:
    bool operator()(const CacheEntry &ce1, const CacheEntry &ce2) const {
      return ce1 == ce2;
    }
  };

  typedef std::unordered_map<CacheEntry, const Assignment, CacheEntryHash,
                             CacheEntryCmp>
      concretizations_map;
  concretizations_map concretizations;
  bool simplifyExprs;

public:
  ConcretizationManager(bool simplifyExprs) : simplifyExprs(simplifyExprs) {}

  std::pair<Assignment, bool> get(const ConstraintSet &set, ref<Expr> query);
  bool contains(const ConstraintSet &set, ref<Expr> query);
  void add(const Query &q, const Assignment &assign);
};

}; // namespace klee

#endif /* KLEE_CONCRETIZATIONMANAGER_H */
