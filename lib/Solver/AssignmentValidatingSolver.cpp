//===-- AssignmentValidatingSolver.cpp ------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Expr/Constraints.h"
#include "klee/Expr/Assignment.h"
#include "klee/Solver/Solver.h"
#include "klee/Solver/SolverImpl.h"

#include <vector>

namespace klee {

class AssignmentValidatingSolver : public SolverImpl {
private:
  Solver *solver;
  void dumpAssignmentQuery(const Query &query, const Assignment &assignment);

public:
  AssignmentValidatingSolver(Solver *_solver) : solver(_solver) {}
  ~AssignmentValidatingSolver() { delete solver; }

  bool computeValidity(const Query &, Solver::Validity &result);
  bool computeTruth(const Query &, bool &isValid);
  bool computeValue(const Query &, ref<Expr> &result);
  bool computeInitialValues(const Query &,
                            const std::vector<const Array *> &objects,
                            std::vector<std::vector<unsigned char> > &values,
                            bool &hasSolution);
  SolverRunStatus getOperationStatusCode();
  char *getConstraintLog(const Query &);
  void setCoreSolverTimeout(time::Span timeout);
};

// TODO: use computeInitialValues for all queries for more stress testing
bool AssignmentValidatingSolver::computeValidity(const Query &query,
                                                 Solver::Validity &result) {
  return solver->impl->computeValidity(query, result);
}
bool AssignmentValidatingSolver::computeTruth(const Query &query,
                                              bool &isValid) {
  return solver->impl->computeTruth(query, isValid);
}
bool AssignmentValidatingSolver::computeValue(const Query &query,
                                              ref<Expr> &result) {
  return solver->impl->computeValue(query, result);
}

bool AssignmentValidatingSolver::computeInitialValues(
    const Query &query, const std::vector<const Array *> &objects,
    std::vector<std::vector<unsigned char> > &values, bool &hasSolution) {
  bool success =
      solver->impl->computeInitialValues(query, objects, values, hasSolution);
  if (!hasSolution)
    return success;

  // Use `_allowFreeValues` so that if we are missing an assignment
  // we can't compute a constant and flag this as a problem.
  Assignment assignment(objects, values, /*_allowFreeValues=*/true);
  // Check computed assignment satisfies query
  for (ConstraintManager::const_iterator it = query.constraints.begin(),
                                         ie = query.constraints.end();
       it != ie; ++it) {
    ref<Expr> constraint = *it;
    ref<Expr> constraintEvaluated = assignment.evaluate(constraint);
    ConstantExpr *CE = dyn_cast<ConstantExpr>(constraintEvaluated);
    if (CE == NULL) {
      llvm::errs() << "Constraint did not evalaute to a constant:\n";
      llvm::errs() << "Constraint:\n" << constraint << "\n";
      llvm::errs() << "Evaluated Constraint:\n" << constraintEvaluated << "\n";
      llvm::errs() << "Assignment:\n";
      assignment.dump();
      dumpAssignmentQuery(query, assignment);
      abort();
    }
    if (CE->isFalse()) {
      llvm::errs() << "Constraint evaluated to false when using assignment\n";
      llvm::errs() << "Constraint:\n" << constraint << "\n";
      llvm::errs() << "Assignment:\n";
      assignment.dump();
      dumpAssignmentQuery(query, assignment);
      abort();
    }
  }

  ref<Expr> queryExprEvaluated = assignment.evaluate(query.expr);
  ConstantExpr *CE = dyn_cast<ConstantExpr>(queryExprEvaluated);
  if (CE == NULL) {
    llvm::errs() << "Query expression did not evalaute to a constant:\n";
    llvm::errs() << "Expression:\n" << query.expr << "\n";
    llvm::errs() << "Evaluated expression:\n" << queryExprEvaluated << "\n";
    llvm::errs() << "Assignment:\n";
    assignment.dump();
    dumpAssignmentQuery(query, assignment);
    abort();
  }
  // KLEE queries are validity queries. A counter example to
  // ∀ x constraints(x) → query(x)
  // exists therefore
  // ¬∀ x constraints(x) → query(x)
  // Which is equivalent to
  // ∃ x constraints(x) ∧ ¬ query(x)
  // This means for the assignment we get back query expression should evaluate
  // to false.
  if (CE->isTrue()) {
    llvm::errs()
        << "Query Expression evaluated to true when using assignment\n";
    llvm::errs() << "Expression:\n" << query.expr << "\n";
    llvm::errs() << "Assignment:\n";
    assignment.dump();
    dumpAssignmentQuery(query, assignment);
    abort();
  }

  return success;
}

void AssignmentValidatingSolver::dumpAssignmentQuery(
    const Query &query, const Assignment &assignment) {
  // Create a Query that is augmented with constraints that
  // enforce the given assignment.
  std::vector<ref<Expr> > constraints;
  assignment.createConstraintsFromAssignment(constraints);
  // Add Constraints from `query`
  for (ConstraintManager::const_iterator it = query.constraints.begin(),
                                         ie = query.constraints.end();
       it != ie; ++it) {
    constraints.push_back(*it);
  }
  ConstraintManager augmentedConstraints(constraints);
  Query augmentedQuery(augmentedConstraints, query.expr);

  // Ask the solver for the log for this query.
  char *logText = solver->getConstraintLog(augmentedQuery);
  llvm::errs() << "Query with assignment as constraints:\n" << logText << "\n";
  free(logText);
}

SolverImpl::SolverRunStatus
AssignmentValidatingSolver::getOperationStatusCode() {
  return solver->impl->getOperationStatusCode();
}

char *AssignmentValidatingSolver::getConstraintLog(const Query &query) {
  return solver->impl->getConstraintLog(query);
}

void AssignmentValidatingSolver::setCoreSolverTimeout(time::Span timeout) {
  return solver->impl->setCoreSolverTimeout(timeout);
}

Solver *createAssignmentValidatingSolver(Solver *s) {
  return new Solver(new AssignmentValidatingSolver(s));
}
}
