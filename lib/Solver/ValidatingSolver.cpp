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

#include <vector>

namespace klee {

class ValidatingSolver : public SolverImpl {
private:
  Solver *solver, *oracle;

public:
  ValidatingSolver(Solver *_solver, Solver *_oracle)
      : solver(_solver), oracle(_oracle) {}
  ~ValidatingSolver() { delete solver; }

  bool computeValidity(const Query &, Solver::Validity &result);
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
  char *getConstraintLog(const Query &);
  void setCoreSolverTimeout(time::Span timeout);
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
    std::vector<SparseStorage<unsigned char>> &values, bool &hasSolution) {
  bool answer;

  if (!solver->impl->computeInitialValues(query, objects, values, hasSolution))
    return false;

  if (hasSolution) {
    // Assert the bindings as constraints, and verify that the
    // conjunction of the actual constraints is satisfiable.
    ConstraintSet bindings;
    Assignment solutionAssignment(objects, values, true);

    for (unsigned i = 0; i != values.size(); ++i) {
      const Array *array = objects[i];
      assert(array);
      ref<ConstantExpr> arrayConstantSize =
          dyn_cast<ConstantExpr>(solutionAssignment.evaluate(array->size));
      assert(arrayConstantSize &&
             "Array of symbolic size had not receive value for size!");

      for (unsigned j = 0; j < arrayConstantSize->getZExtValue(); j++) {
        unsigned char value = values[i].load(j);
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

bool ValidatingSolver::check(const Query &query, ref<SolverResponse> &result) {
  ref<SolverResponse> answer;

  if (!solver->impl->check(query, result))
    return false;
  if (!oracle->impl->check(query, answer))
    return false;

  if (result->getResponseKind() != answer->getResponseKind())
    assert(0 && "invalid solver result (check)");

  bool banswer;
  if (isa<InvalidResponse>(result)) {
    // Assert the bindings as constraints, and verify that the
    // conjunction of the actual constraints is satisfiable.

    ConstraintSet bindings;
    std::map<const Array *, SparseStorage<unsigned char>> initialValues;
    cast<InvalidResponse>(result)->tryGetInitialValues(initialValues);
    Assignment solutionAssignment(initialValues);
    for (auto &arrayValues : initialValues) {
      const Array *array = arrayValues.first;
      assert(array);
      ref<ConstantExpr> arrayConstantSize =
          dyn_cast<ConstantExpr>(solutionAssignment.evaluate(array->size));
      assert(arrayConstantSize &&
             "Array of symbolic size had not receive value for size!");

      for (unsigned j = 0; j < arrayConstantSize->getZExtValue(); j++) {
        unsigned char value = arrayValues.second.load(j);
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

    if (!oracle->impl->computeTruth(Query(bindings, constraints), banswer))
      return false;
    if (!banswer)
      assert(0 && "invalid solver result (computeInitialValues)");
  } else {
    if (!oracle->impl->computeTruth(query, banswer))
      return false;
    if (!banswer)
      assert(0 && "invalid solver result (computeInitialValues)");
  }

  return true;
}

bool ValidatingSolver::computeValidityCore(const Query &query,
                                           ValidityCore &validityCore,
                                           bool &isValid) {
  ValidityCore answerCore;
  bool answer;

  if (!solver->impl->computeValidityCore(query, validityCore, isValid))
    return false;
  if (!oracle->impl->computeValidityCore(query, answerCore, answer))
    return false;

  if (validityCore != answerCore)
    assert(0 && "invalid solver result (check)");
  if (isValid != answer)
    assert(0 && "invalid solver result (check)");

  return true;
}

SolverImpl::SolverRunStatus ValidatingSolver::getOperationStatusCode() {
  return solver->impl->getOperationStatusCode();
}

char *ValidatingSolver::getConstraintLog(const Query &query) {
  return solver->impl->getConstraintLog(query);
}

void ValidatingSolver::setCoreSolverTimeout(time::Span timeout) {
  solver->impl->setCoreSolverTimeout(timeout);
}

Solver *createValidatingSolver(Solver *s, Solver *oracle) {
  return new Solver(new ValidatingSolver(s, oracle));
}
} // namespace klee
