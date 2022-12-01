#include "klee/Expr/Assignment.h"
#include "klee/Expr/Constraints.h"
#include "klee/Expr/Expr.h"
#include "klee/Expr/ExprUtil.h"
#include "klee/Expr/IndependentSet.h"
#include "klee/Solver/ConcretizationManager.h"
#include "klee/Solver/Solver.h"
#include "klee/Solver/SolverImpl.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Support/Casting.h"
#include <algorithm>
#include <vector>

namespace klee {

class ConcretizingSolver : public SolverImpl {
private:
  Solver *solver;
  ConcretizationManager *concretizationManager;

public:
  ConcretizingSolver(Solver *_solver,
                     ConcretizationManager *_concretizationManager)
      : solver(_solver), concretizationManager(_concretizationManager) {}

  ~ConcretizingSolver() { delete solver; }

  bool computeTruth(const Query &, bool &isValid);
  bool computeValidity(const Query &, Solver::Validity &result);
  bool computeValue(const Query &, ref<Expr> &result);
  bool computeInitialValues(const Query &query,
                            const std::vector<const Array *> &objects,
                            std::vector<std::vector<unsigned char>> &values,
                            bool &hasSolution);
  SolverRunStatus getOperationStatusCode();
  char *getConstraintLog(const Query &);
  void setCoreSolverTimeout(time::Span timeout);

private:
  Query constructConcretizedQuery(const Query &, const Assignment &);
};

Query ConcretizingSolver::constructConcretizedQuery(const Query &query,
                                                    const Assignment &assign) {
  ConstraintSet constraints;
  for (auto e : query.constraints) {
    constraints.push_back(assign.evaluate(e));
  }
  ref<Expr> expr = assign.evaluate(query.expr);
  ConstraintSet equalities = assign.createConstraintsFromAssignment();
  for (auto e : equalities) {
    constraints.push_back(e);
  }
  return Query(constraints, expr);
}

bool ConcretizingSolver::computeValidity(const Query &query,
                                         Solver::Validity &result) {
  if (!query.containsSymcretes()) {
    return solver->impl->computeValidity(query, result);
  }

  auto assign = concretizationManager->get(query.constraints);
  auto concretizedQuery = concretizationManager->getConcretizedQuery(query);

  if (!solver->impl->computeValidity(concretizedQuery, result)) {
    return false;
  }

  switch (result) {
  case Solver::True: {
    concretizationManager->add(query, assign);
    break;
  }

  case Solver::False: {
    concretizationManager->add(query.negateExpr(), assign);
    break;
  }

  case Solver::Unknown: {
    concretizationManager->add(query, assign);
    concretizationManager->add(query.negateExpr(), assign);
    break;
  }

  default:
    assert(0 && "unreachable");
  }

  return true;
}

char *ConcretizingSolver::getConstraintLog(const Query &query) {
  return solver->impl->getConstraintLog(query);
}

bool ConcretizingSolver::computeTruth(const Query &query, bool &isValid) {
  if (!query.containsSymcretes()) {
    return solver->impl->computeTruth(query, isValid);
  }

  auto assign = concretizationManager->get(query.constraints);
  auto concretizedQuery = concretizationManager->getConcretizedQuery(query);

  if (!solver->impl->computeTruth(concretizedQuery, isValid)) {
    return false;
  }

  // If constraints always evaluate to `mustBeTrue`, then relax
  // symcretes until remove all of them or query starts to evaluate
  // to `mayBeFalse`.

  if (!isValid) {
    concretizationManager->add(query.negateExpr(), assign);
  }

  return true;
}

bool ConcretizingSolver::computeValue(const Query &query, ref<Expr> &result) {
  if (!query.containsSymcretes()) {
    return solver->impl->computeValue(query, result);
  }

  Assignment assign = concretizationManager->get(query.constraints);

  auto concretizedQuery = concretizationManager->getConcretizedQuery(query);
  if (ref<ConstantExpr> expr =
          dyn_cast<ConstantExpr>(ConstraintManager::simplifyExpr(
              concretizedQuery.constraints, concretizedQuery.expr))) {
    result = expr;
    return true;
  }
  return solver->impl->computeValue(concretizedQuery, result);
}

bool ConcretizingSolver::computeInitialValues(
    const Query &query, const std::vector<const Array *> &objects,
    std::vector<std::vector<unsigned char>> &values, bool &hasSolution) {
  if (!query.containsSymcretes()) {
    return solver->impl->computeInitialValues(query, objects, values,
                                              hasSolution);
  }

  Assignment assign = concretizationManager->get(query.constraints);

  auto concretizedQuery = concretizationManager->getConcretizedQuery(query);
  return solver->impl->computeInitialValues(concretizedQuery, objects, values,
                                            hasSolution);
}

// Redo later
SolverImpl::SolverRunStatus ConcretizingSolver::getOperationStatusCode() {
  return solver->impl->getOperationStatusCode();
}

void ConcretizingSolver::setCoreSolverTimeout(time::Span timeout) {
  solver->setCoreSolverTimeout(timeout);
}

Solver *createConcretizingSolver(Solver *s, ConcretizationManager *cm) {
  return new Solver(new ConcretizingSolver(s, cm));
}
} // namespace klee
