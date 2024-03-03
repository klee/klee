//===-- ValidatingSolver.cpp ----------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Expr/Constraints.h"
#include "klee/Solver/Solver.h"
#include "klee/Solver/SolverImpl.h"

#include <memory>
#include <utility>
#include <vector>

namespace klee {

class ValidatingSolver : public SolverImpl {
private:
  std::unique_ptr<Solver> solver;
  std::unique_ptr<Solver, void(*)(Solver*)> oracle;

public:
  ValidatingSolver(std::unique_ptr<Solver> solver, Solver *oracle,
                   bool ownsOracle)
      : solver(std::move(solver)),
        oracle(
            oracle, ownsOracle ? [](Solver *solver) { delete solver; }
                               : [](Solver *) {}) {}

  bool computeValidity(const Query &, Solver::Validity &result) override;
  bool computeTruth(const Query &, bool &isValid) override;
  bool computeValue(const Query &, ref<Expr> &result) override;
  bool computeInitialValues(const Query &,
                            const std::vector<const Array *> &objects,
                            std::vector<std::vector<unsigned char>> &values,
                            bool &hasSolution) override;
  SolverRunStatus getOperationStatusCode() override;
  std::string getConstraintLog(const Query &) override;
  void setCoreSolverTimeout(time::Span timeout) override;
};

bool ValidatingSolver::computeTruth(const Query &query, bool &isValid) {
  bool answer;

  if (!solver->impl->computeTruth(query, isValid))
    return false;
  if (!oracle->impl->computeTruth(query, answer))
    return false;

  if (isValid != answer)
    assert(0 && "invalid solver result (computeTruth)");

  return true;
}

bool ValidatingSolver::computeValidity(const Query &query,
                                       Solver::Validity &result) {
  Solver::Validity answer;

  if (!solver->impl->computeValidity(query, result))
    return false;
  if (!oracle->impl->computeValidity(query, answer))
    return false;

  if (result != answer)
    assert(0 && "invalid solver result (computeValidity)");

  return true;
}

bool ValidatingSolver::computeValue(const Query &query, ref<Expr> &result) {
  bool answer;

  if (!solver->impl->computeValue(query, result))
    return false;
  // We don't want to compare, but just make sure this is a legal
  // solution.
  if (!oracle->impl->computeTruth(
          query.withExpr(NeExpr::create(query.expr, result)), answer))
    return false;

  if (answer)
    assert(0 && "invalid solver result (computeValue)");

  return true;
}

bool ValidatingSolver::computeInitialValues(
    const Query &query, const std::vector<const Array *> &objects,
    std::vector<std::vector<unsigned char> > &values, bool &hasSolution) {
  bool answer;

  if (!solver->impl->computeInitialValues(query, objects, values, hasSolution))
    return false;

  if (hasSolution) {
    // Assert the bindings as constraints, and verify that the
    // conjunction of the actual constraints is satisfiable.
    ConstraintSet bindings;
    for (unsigned i = 0; i != values.size(); ++i) {
      const Array *array = objects[i];
      assert(array);
      for (unsigned j = 0; j < array->size; j++) {
        unsigned char value = values[i][j];
        bindings.push_back(EqExpr::create(
            ReadExpr::create(UpdateList(array, 0),
                             ConstantExpr::alloc(j, array->getDomain())),
            ConstantExpr::alloc(value, array->getRange())));
      }
    }
    ConstraintManager tmp(bindings);
    ref<Expr> constraints = Expr::createIsZero(query.expr);
    for (auto const &constraint : query.constraints)
      constraints = AndExpr::create(constraints, constraint);

    if (!oracle->impl->computeTruth(Query(bindings, constraints), answer))
      return false;
    if (!answer)
      assert(0 && "invalid solver result (computeInitialValues)");
  } else {
    if (!oracle->impl->computeTruth(query, answer))
      return false;
    if (!answer)
      assert(0 && "invalid solver result (computeInitialValues)");
  }

  return true;
}

SolverImpl::SolverRunStatus ValidatingSolver::getOperationStatusCode() {
  return solver->impl->getOperationStatusCode();
}

std::string ValidatingSolver::getConstraintLog(const Query &query) {
  return solver->impl->getConstraintLog(query);
}

void ValidatingSolver::setCoreSolverTimeout(time::Span timeout) {
  solver->impl->setCoreSolverTimeout(timeout);
}

std::unique_ptr<Solver> createValidatingSolver(std::unique_ptr<Solver> s,
                                               Solver *oracle,
                                               bool ownsOracle) {
  return std::make_unique<Solver>(
      std::make_unique<ValidatingSolver>(std::move(s), oracle, ownsOracle));
}
}
