//===-- TimingSolver.h ------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_TIMINGSOLVER_H
#define KLEE_TIMINGSOLVER_H

#include "klee/Expr/ArrayExprOptimizer.h"
#include "klee/Expr/Constraints.h"
#include "klee/Expr/Expr.h"
#include "klee/Solver/Solver.h"
#include "klee/System/Time.h"

#include <memory>
#include <utility>
#include <vector>

namespace klee {
class ConstraintSet;
class Solver;

/// TimingSolver - A simple class which wraps a solver and handles
/// tracking the statistics that we care about.
class TimingSolver {
public:
  std::unique_ptr<Solver> solver;
  ExprOptimizer optimizer;
  bool simplifyExprs;

public:
  /// TimingSolver - Construct a new timing solver.
  ///
  /// \param _simplifyExprs - Whether expressions should be
  /// simplified (via the constraint manager interface) prior to
  /// querying.
  TimingSolver(std::unique_ptr<Solver> solver, ExprOptimizer optimizer,
               bool _simplifyExprs = true)
      : solver(std::move(solver)), optimizer(optimizer),
        simplifyExprs(_simplifyExprs) {}

  void setTimeout(time::Span t) { solver->setCoreSolverTimeout(t); }

  char *getConstraintLog(const Query &query) {
    return solver->getConstraintLog(query);
  }

  /// @brief Notify the solver that the state with specified id has been
  /// terminated
  void notifyStateTermination(std::uint32_t id);

  bool evaluate(const ConstraintSet &, ref<Expr>, PartialValidity &result,
                SolverQueryMetaData &metaData,
                bool produceValidityCore = false);

  bool evaluate(const ConstraintSet &, ref<Expr>,
                ref<SolverResponse> &queryResult,
                ref<SolverResponse> &negateQueryResult,
                SolverQueryMetaData &metaData);

  /// Writes a unique constant value for the given expression in the
  /// given state, if it has one (i.e. it provably only has a single
  /// value) in the result. Otherwise writes the original expression.
  bool tryGetUnique(const ConstraintSet &, ref<Expr>, ref<Expr> &result,
                    SolverQueryMetaData &metaData);

  bool mustBeTrue(const ConstraintSet &, ref<Expr>, bool &result,
                  SolverQueryMetaData &metaData,
                  bool produceValidityCore = false);

  bool mustBeFalse(const ConstraintSet &, ref<Expr>, bool &result,
                   SolverQueryMetaData &metaData,
                   bool produceValidityCore = false);

  bool mayBeTrue(const ConstraintSet &, ref<Expr>, bool &result,
                 SolverQueryMetaData &metaData,
                 bool produceValidityCore = false);

  bool mayBeFalse(const ConstraintSet &, ref<Expr>, bool &result,
                  SolverQueryMetaData &metaData,
                  bool produceValidityCore = false);

  bool getValue(const ConstraintSet &, ref<Expr> expr,
                ref<ConstantExpr> &result, SolverQueryMetaData &metaData);

  bool getMinimalUnsignedValue(const ConstraintSet &, ref<Expr> expr,
                               ref<ConstantExpr> &result,
                               SolverQueryMetaData &metaData);

  bool getInitialValues(const ConstraintSet &,
                        const std::vector<const Array *> &objects,
                        std::vector<SparseStorage<unsigned char>> &result,
                        SolverQueryMetaData &metaData,
                        bool produceValidityCore = false);

  bool getValidityCore(const ConstraintSet &, ref<Expr>,
                       ValidityCore &validityCore, bool &result,
                       SolverQueryMetaData &metaData);

  bool getResponse(const ConstraintSet &, ref<Expr>,
                   ref<SolverResponse> &queryResult,
                   SolverQueryMetaData &metaData);

  std::pair<ref<Expr>, ref<Expr>> getRange(const ConstraintSet &,
                                           ref<Expr> query,
                                           SolverQueryMetaData &metaData,
                                           time::Span timeout = time::Span());
};
} // namespace klee

#endif /* KLEE_TIMINGSOLVER_H */
