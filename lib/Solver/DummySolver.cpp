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

  bool computeValidity(const Query &, PartialValidity &result);
  bool computeTruth(const Query &, bool &isValid);
  bool computeValue(const Query &, ref<Expr> &result);
  bool computeInitialValues(const Query &,
                            const std::vector<const Array *> &objects,
                            std::vector<SparseStorage<unsigned char>> &values,
                            bool &hasSolution);
  bool check(const Query &query, ref<SolverResponse> &result);
  bool computeValidityCore(const Query &query, ValidityCore &validityCore,
                           bool &isValid);
  SolverRunStatus getOperationStatusCode();
  void notifyStateTermination(std::uint32_t id);
};

DummySolverImpl::DummySolverImpl() {}

bool DummySolverImpl::computeValidity(const Query &, PartialValidity &result) {
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
    std::vector<SparseStorage<unsigned char>> &values, bool &hasSolution) {
  ++stats::solverQueries;
  ++stats::queryCounterexamples;
  return false;
}

bool DummySolverImpl::check(const Query &query, ref<SolverResponse> &result) {
  ++stats::solverQueries;
  ++stats::queryCounterexamples;
  ++stats::queryValidityCores;
  return false;
}

bool DummySolverImpl::computeValidityCore(const Query &query,
                                          ValidityCore &validityCore,
                                          bool &isValid) {
  ++stats::solverQueries;
  ++stats::queryValidityCores;
  return false;
}

SolverImpl::SolverRunStatus DummySolverImpl::getOperationStatusCode() {
  return SOLVER_RUN_STATUS_FAILURE;
}

void DummySolverImpl::notifyStateTermination(std::uint32_t id) {}

std::unique_ptr<Solver> createDummySolver() {
  return std::make_unique<Solver>(std::make_unique<DummySolverImpl>());
}
} // namespace klee
