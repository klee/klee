//===-- DummySolver.cpp ---------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Solver/Solver.h"
#include "klee/Solver/SolverImpl.h"
#include "klee/Solver/SolverStats.h"

#include <memory>

namespace klee {

class DummySolverImpl : public SolverImpl {
public:
  DummySolverImpl();

  bool computeValidity(const Query &, Solver::Validity &result);
  bool computeTruth(const Query &, bool &isValid);
  bool computeValue(const Query &, ref<Expr> &result);
  bool computeInitialValues(const Query &,
                            const std::vector<const Array *> &objects,
                            std::vector<std::vector<unsigned char> > &values,
                            bool &hasSolution);
  SolverRunStatus getOperationStatusCode();
};

DummySolverImpl::DummySolverImpl() {}

bool DummySolverImpl::computeValidity(const Query &, Solver::Validity &result) {
  ++stats::solverQueries;
  // FIXME: We should have stats::queriesFail;
  return false;
}

bool DummySolverImpl::computeTruth(const Query &, bool &isValid) {
  ++stats::solverQueries;
  // FIXME: We should have stats::queriesFail;
  return false;
}

bool DummySolverImpl::computeValue(const Query &, ref<Expr> &result) {
  ++stats::solverQueries;
  ++stats::queryCounterexamples;
  return false;
}

bool DummySolverImpl::computeInitialValues(
    const Query &, const std::vector<const Array *> &objects,
    std::vector<std::vector<unsigned char> > &values, bool &hasSolution) {
  ++stats::solverQueries;
  ++stats::queryCounterexamples;
  return false;
}

SolverImpl::SolverRunStatus DummySolverImpl::getOperationStatusCode() {
  return SOLVER_RUN_STATUS_FAILURE;
}

std::unique_ptr<Solver> createDummySolver() {
  return std::make_unique<Solver>(std::make_unique<DummySolverImpl>());
}
}
