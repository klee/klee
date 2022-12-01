#ifndef KLEE_CONCRETIZATIONMANAGER_H
#define KLEE_CONCRETIZATIONMANAGER_H

#include "klee/ADT/MapOfSets.h"
#include "klee/Expr/Assignment.h"
#include "klee/Expr/Constraints.h"
#include <unordered_map>

namespace klee {
struct Query;

class ConcretizationManager {
private:
  MapOfSets<ref<Expr>, Assignment> concretizations;

public:
  Assignment get(const ConstraintSet &set);
  void add(const Query &q, const Assignment &assign);
  Query getConcretizedQuery(const Query &q);
};

}; // namespace klee

#endif /* KLEE_CONCRETIZATIONMANAGER_H */
