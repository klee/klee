//===-- IncompleteSolver.h --------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_INCOMPLETESOLVER_H
#define KLEE_INCOMPLETESOLVER_H

#include "klee/Solver/Solver.h"
#include "klee/Solver/SolverImpl.h"

#include <memory>

namespace klee {

/// IncompleteSolver - Base class for incomplete solver
/// implementations.
///
/// Incomplete solvers are useful for implementing optimizations which
/// may quickly compute an answer, but cannot always compute the
/// correct answer. They can be used with the StagedSolver to provide
/// a complete Solver implementation.
class IncompleteSolver {
public:
  /// PartialValidity - Represent a possibility incomplete query
  /// validity.
  enum PartialValidity {
    /// The query is provably true.
    MustBeTrue = 1,

    /// The query is provably false.
    MustBeFalse = -1,

    /// The query is not provably false (a true assignment is known to
    /// exist).
    MayBeTrue = 2,

    /// The query is not provably true (a false assignment is known to
    /// exist).
    MayBeFalse = -2,

    /// The query is known to have both true and false assignments.
    TrueOrFalse = 0,

    /// The validity of the query is unknown.
    None = 3
  };

  static PartialValidity negatePartialValidity(PartialValidity pv);

public:
  IncompleteSolver() {}
  virtual ~IncompleteSolver() {}

  /// computeValidity - Compute a partial validity for the given query.
  ///
  /// The passed expression is non-constant with bool type.
  ///
  /// The IncompleteSolver class provides an implementation of
  /// computeValidity using computeTruth. Sub-classes may override
  /// this if a more efficient implementation is available.
  virtual IncompleteSolver::PartialValidity computeValidity(const Query &);

  /// computeValidity - Compute a partial validity for the given query.
  ///
  /// The passed expression is non-constant with bool type.
  virtual IncompleteSolver::PartialValidity computeTruth(const Query &) = 0;

  /// computeValue - Attempt to compute a value for the given expression.
  virtual bool computeValue(const Query &, ref<Expr> &result) = 0;

  /// computeInitialValues - Attempt to compute the constant values
  /// for the initial state of each given object. If a correct result
  /// is not found, then the values array must be unmodified.
  virtual bool
  computeInitialValues(const Query &, const std::vector<const Array *> &objects,
                       std::vector<std::vector<unsigned char>> &values,
                       bool &hasSolution) = 0;
};

/// StagedSolver - Adapter class for staging an incomplete solver with
/// a complete secondary solver, to form an (optimized) complete
/// solver.
class StagedSolverImpl : public SolverImpl {
private:
  std::unique_ptr<IncompleteSolver> primary;
  std::unique_ptr<Solver> secondary;

public:
  StagedSolverImpl(std::unique_ptr<IncompleteSolver> primary,
                   std::unique_ptr<Solver> secondary);

  bool computeTruth(const Query &, bool &isValid) override;
  bool computeValidity(const Query &, Solver::Validity &result) override;
  bool computeValue(const Query &, ref<Expr> &result) override;
  bool computeInitialValues(const Query &,
                            const std::vector<const Array *> &objects,
                            std::vector<std::vector<unsigned char>> &values,
                            bool &hasSolution) override;
  SolverRunStatus getOperationStatusCode() override;
  std::string getConstraintLog(const Query &) override;
  void setCoreSolverTimeout(time::Span timeout) override;
};

} // namespace klee

#endif /* KLEE_INCOMPLETESOLVER_H */
