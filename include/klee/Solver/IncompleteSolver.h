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
  IncompleteSolver() {}
  virtual ~IncompleteSolver() {}

  /// computeValidity - Compute a partial validity for the given query.
  ///
  /// The passed expression is non-constant with bool type.
  ///
  /// The IncompleteSolver class provides an implementation of
  /// computeValidity using computeTruth. Sub-classes may override
  /// this if a more efficient implementation is available.
  virtual PartialValidity computeValidity(const Query &);

  /// computeValidity - Compute a partial validity for the given query.
  ///
  /// The passed expression is non-constant with bool type.
  virtual PartialValidity computeTruth(const Query &) = 0;

  /// computeValue - Attempt to compute a value for the given expression.
  virtual bool computeValue(const Query &, ref<Expr> &result) = 0;

  /// computeInitialValues - Attempt to compute the constant values
  /// for the initial state of each given object. If a correct result
  /// is not found, then the values array must be unmodified.
  virtual bool
  computeInitialValues(const Query &, const std::vector<const Array *> &objects,
                       std::vector<SparseStorage<unsigned char>> &values,
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

  bool computeTruth(const Query &, bool &isValid);
  bool computeValidity(const Query &, PartialValidity &result);
  bool computeValue(const Query &, ref<Expr> &result);
  bool computeInitialValues(const Query &,
                            const std::vector<const Array *> &objects,
                            std::vector<SparseStorage<unsigned char>> &values,
                            bool &hasSolution);
  bool check(const Query &query, ref<SolverResponse> &result);
  bool computeValidityCore(const Query &query, ValidityCore &validityCore,
                           bool &hasSolution);
  SolverRunStatus getOperationStatusCode();
  char *getConstraintLog(const Query &);
  void setCoreSolverTimeout(time::Span timeout);
  void notifyStateTermination(std::uint32_t id);
};

} // namespace klee

#endif /* KLEE_INCOMPLETESOLVER_H */
