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

#include "klee/Expr/Constraints.h"
#include "klee/Expr/Expr.h"
#include "klee/Solver/Solver.h"
#include "klee/System/Time.h"

#include <memory>
#include <string>
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
  bool simplifyExprs;

public:
  /// TimingSolver - Construct a new timing solver.
  ///
  /// \param _simplifyExprs - Whether expressions should be
  /// simplified (via the constraint manager interface) prior to
  /// querying.
  TimingSolver(std::unique_ptr<Solver> solver, bool simplifyExprs = true)
      : solver(std::move(solver)), simplifyExprs(simplifyExprs) {}

  void setTimeout(time::Span t) { solver->setCoreSolverTimeout(t); }

  std::string getConstraintLog(const Query &query) {
    return solver->getConstraintLog(query);
  }

  bool evaluate(const ConstraintSet &, ref<Expr>, Solver::Validity &result,
                SolverQueryMetaData &metaData);

  bool mustBeTrue(const ConstraintSet &, ref<Expr>, bool &result,
                  SolverQueryMetaData &metaData);

  bool mustBeFalse(const ConstraintSet &, ref<Expr>, bool &result,
                   SolverQueryMetaData &metaData);

  bool mayBeTrue(const ConstraintSet &, ref<Expr>, bool &result,
                 SolverQueryMetaData &metaData);

  bool mayBeFalse(const ConstraintSet &, ref<Expr>, bool &result,
                  SolverQueryMetaData &metaData);

  bool getValue(const ConstraintSet &, ref<Expr> expr,
                ref<ConstantExpr> &result, SolverQueryMetaData &metaData);

  bool getInitialValues(const ConstraintSet &,
                        const std::vector<const Array *> &objects,
                        std::vector<std::vector<unsigned char>> &result,
                        SolverQueryMetaData &metaData);

  std::pair<ref<Expr>, ref<Expr>> getRange(const ConstraintSet &,
                                           ref<Expr> query,
                                           SolverQueryMetaData &metaData);
};
}

#endif /* KLEE_TIMINGSOLVER_H */
