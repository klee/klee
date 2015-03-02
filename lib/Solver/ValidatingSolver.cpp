#include "klee/Solver.h"
#include "klee/SolverImpl.h"

#include "klee/Constraints.h"

#include "ValidatingSolver.h"

using namespace klee;

bool ValidatingSolver::computeTruth(const Query& query,
                                    bool &isValid) {
  bool answer;
  
  if (!solver->impl->computeTruth(query, isValid))
    return false;
  if (!oracle->impl->computeTruth(query, answer))
    return false;
  
  if (isValid != answer)
    assert(0 && "invalid solver result (computeTruth)");
  
  return true;
}

bool ValidatingSolver::computeValidity(const Query& query,
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

bool ValidatingSolver::computeValue(const Query& query,
                                    ref<Expr> &result) {  
  bool answer;

  if (!solver->impl->computeValue(query, result))
    return false;
  // We don't want to compare, but just make sure this is a legal
  // solution.
  if (!oracle->impl->computeTruth(query.withExpr(NeExpr::create(query.expr, 
                                                                result)),
                                  answer))
    return false;

  if (answer)
    assert(0 && "invalid solver result (computeValue)");

  return true;
}

bool 
ValidatingSolver::computeInitialValues(const Query& query,
                                       const std::vector<const Array*>
                                         &objects,
                                       std::vector< std::vector<unsigned char> >
                                         &values,
                                       bool &hasSolution) {
  bool answer;

  if (!solver->impl->computeInitialValues(query, objects, values, 
                                          hasSolution))
    return false;

  if (hasSolution) {
    // Assert the bindings as constraints, and verify that the
    // conjunction of the actual constraints is satisfiable.
    std::vector< ref<Expr> > bindings;
    for (unsigned i = 0; i != values.size(); ++i) {
      const Array *array = objects[i];
      assert(array);
      for (unsigned j=0; j<array->size; j++) {
        unsigned char value = values[i][j];
        bindings.push_back(EqExpr::create(ReadExpr::create(UpdateList(array, 0),
                                                           ConstantExpr::alloc(j, array->getDomain())),
                                          ConstantExpr::alloc(value, array->getRange())));
      }
    }
    ConstraintManager tmp(bindings);
    ref<Expr> constraints = Expr::createIsZero(query.expr);
    for (ConstraintManager::const_iterator it = query.constraints.begin(), 
           ie = query.constraints.end(); it != ie; ++it)
      constraints = AndExpr::create(constraints, *it);
    
    if (!oracle->impl->computeTruth(Query(tmp, constraints), answer))
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

char *ValidatingSolver::getConstraintLog(const Query& query) {
  return solver->impl->getConstraintLog(query);
}

void ValidatingSolver::setCoreSolverTimeout(double timeout) {
  solver->impl->setCoreSolverTimeout(timeout);
}

Solver *klee::createValidatingSolver(Solver *s, Solver *oracle) {
  return new Solver(new ValidatingSolver(s, oracle));
}

