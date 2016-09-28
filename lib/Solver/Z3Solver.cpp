//===-- Z3Solver.cpp -------------------------------------------*- C++ -*-====//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#include "klee/Config/config.h"
#include "klee/Internal/Support/ErrorHandling.h"
#ifdef ENABLE_Z3
#include "Z3Builder.h"
#include "klee/Constraints.h"
#include "klee/Solver.h"
#include "klee/SolverImpl.h"
#include "klee/util/Assignment.h"
#include "klee/util/ExprUtil.h"

#include "llvm/Support/ErrorHandling.h"

namespace klee {

class Z3SolverImpl : public SolverImpl {
private:
  Z3Builder *builder;
  double timeout;
  SolverRunStatus runStatusCode;
  std::vector<ref<Expr> > unsatCore;
  ::Z3_params solverParameters;
  // Parameter symbols
  ::Z3_symbol timeoutParamStrSymbol;

  bool internalRunSolver(const Query &,
                         const std::vector<const Array *> *objects,
                         std::vector<std::vector<unsigned char> > *values,
                         bool &hasSolution);

  static void getModel(Z3Builder *builder, Z3_solver solver,
                       const std::vector<const Array *> &objects,
                       std::vector<std::vector<unsigned char> > &values);

  /// getUnsatCoreVector - Declare the routine to extract the unsatisfiability
  /// core vector
  ///
  /// \return - A ref<Expr> vector of unsatisfiability core: empty if there was
  /// no core.
  static std::vector<ref<Expr> > getUnsatCoreVector(const Query &query,
                                                    const Z3Builder *builder,
                                                    const Z3_solver solver);

  /// runAndGetCex - Determine the satisfiability of a query, given assertions
  /// that already included in the Z3 solver.
  ///
  /// \param [out] hasSolution - On success, a boolean indicating the
  /// satisfiability
  /// of the formula.
  /// \param [out] values - On success and satisfiable, a vector containing the
  /// solution.
  /// \return A value of SolverRunStatus: SOLVER_RUN_STATUS_SUCCESS_SOLVABLE
  /// (satisfiable)
  /// or SOLVER_RUN_STATUS_SUCCESS_UNSOLVABLE (unsatisfiable) indicates success,
  /// others
  /// indicate solver failure.
  SolverRunStatus runAndGetCex(const Query &query, Z3Builder *builder,
                               Z3_solver the_solver, Z3_ast q,
                               const std::vector<const Array *> &objects,
                               std::vector<std::vector<unsigned char> > &values,
                               bool &hasSolution);

public:
  Z3SolverImpl();
  ~Z3SolverImpl();

  char *getConstraintLog(const Query &);
  void setCoreSolverTimeout(double _timeout) {
    assert(_timeout >= 0.0 && "timeout must be >= 0");
    timeout = _timeout;

    unsigned int timeoutInMilliSeconds = (unsigned int)((timeout * 1000) + 0.5);
    if (timeoutInMilliSeconds == 0)
      timeoutInMilliSeconds = UINT_MAX;
    Z3_params_set_uint(builder->ctx, solverParameters, timeoutParamStrSymbol,
                       timeoutInMilliSeconds);
  }

  bool computeTruth(const Query &, bool &isValid);
  bool computeValue(const Query &, ref<Expr> &result);
  bool computeInitialValues(const Query &,
                            const std::vector<const Array *> &objects,
                            std::vector<std::vector<unsigned char> > &values,
                            bool &hasSolution);
  SolverRunStatus
  handleSolverResponse(::Z3_solver theSolver, ::Z3_lbool satisfiable,
                       const std::vector<const Array *> *objects,
                       std::vector<std::vector<unsigned char> > *values,
                       bool &hasSolution);
  SolverRunStatus getOperationStatusCode();
  std::vector<ref<Expr> > getUnsatCore();
};

Z3SolverImpl::Z3SolverImpl()
    : builder(new Z3Builder(/*autoClearConstructCache=*/false)), timeout(0.0),
      runStatusCode(SOLVER_RUN_STATUS_FAILURE) {
  assert(builder && "unable to create Z3Builder");
  solverParameters = Z3_mk_params(builder->ctx);
  Z3_params_inc_ref(builder->ctx, solverParameters);
  timeoutParamStrSymbol = Z3_mk_string_symbol(builder->ctx, "timeout");
  setCoreSolverTimeout(timeout);
}

Z3SolverImpl::~Z3SolverImpl() {
  Z3_params_dec_ref(builder->ctx, solverParameters);
  delete builder;
}

/**/

Z3Solver::Z3Solver() : Solver(new Z3SolverImpl()) {}

char *Z3Solver::getConstraintLog(const Query &query) {
  return impl->getConstraintLog(query);
}

void Z3Solver::setCoreSolverTimeout(double timeout) {
  impl->setCoreSolverTimeout(timeout);
}

bool Z3Solver::directComputeValidity(const Query &query,
                                     Solver::Validity &result) {
  return impl->computeValidity(query, result);
}

/***/

char *Z3SolverImpl::getConstraintLog(const Query &query) {
  std::vector<Z3ASTHandle> assumptions;
  for (std::vector<ref<Expr> >::const_iterator it = query.constraints.begin(),
                                               ie = query.constraints.end();
       it != ie; ++it) {
    assumptions.push_back(builder->construct(*it));
  }
  ::Z3_ast *assumptionsArray = NULL;
  int numAssumptions = query.constraints.size();
  if (numAssumptions) {
    assumptionsArray = (::Z3_ast *)malloc(sizeof(::Z3_ast) * numAssumptions);
    for (int index = 0; index < numAssumptions; ++index) {
      assumptionsArray[index] = (::Z3_ast)assumptions[index];
    }
  }

  // KLEE Queries are validity queries i.e.
  // ∀ X Constraints(X) → query(X)
  // but Z3 works in terms of satisfiability so instead we ask the
  // the negation of the equivalent i.e.
  // ∃ X Constraints(X) ∧ ¬ query(X)
  Z3ASTHandle formula = Z3ASTHandle(
      Z3_mk_not(builder->ctx, builder->construct(query.expr)), builder->ctx);

  ::Z3_string result = Z3_benchmark_to_smtlib_string(
      builder->ctx,
      /*name=*/"Emited by klee::Z3SolverImpl::getConstraintLog()",
      /*logic=*/"",
      /*status=*/"unknown",
      /*attributes=*/"",
      /*num_assumptions=*/numAssumptions,
      /*assumptions=*/assumptionsArray,
      /*formula=*/formula);

  if (numAssumptions)
    free(assumptionsArray);
  // Client is responsible for freeing the returned C-string
  return strdup(result);
}

bool Z3SolverImpl::computeTruth(const Query &query, bool &isValid) {
  bool hasSolution;
  bool status =
      internalRunSolver(query, /*objects=*/NULL, /*values=*/NULL, hasSolution);
  isValid = !hasSolution;
  return status;
}

bool Z3SolverImpl::computeValue(const Query &query, ref<Expr> &result) {
  std::vector<const Array *> objects;
  std::vector<std::vector<unsigned char> > values;
  bool hasSolution;

  // Find the object used in the expression, and compute an assignment
  // for them.
  findSymbolicObjects(query.expr, objects);
  if (!computeInitialValues(query.withFalse(), objects, values, hasSolution))
    return false;
  assert(hasSolution && "state has invalid constraint set");

  // Evaluate the expression with the computed assignment.
  Assignment a(objects, values);
  result = a.evaluate(query.expr);

  return true;
}

bool Z3SolverImpl::computeInitialValues(
    const Query &query, const std::vector<const Array *> &objects,
    std::vector<std::vector<unsigned char> > &values, bool &hasSolution) {
  return internalRunSolver(query, &objects, &values, hasSolution);
}

bool Z3SolverImpl::internalRunSolver(
    const Query &query, const std::vector<const Array *> *objects,
    std::vector<std::vector<unsigned char> > *values, bool &hasSolution) {
  TimerStatIncrementer t(stats::queryTime);
  // TODO: Does making a new solver for each query have a performance
  // impact vs making one global solver and using push and pop?
  // TODO: is the "simple_solver" the right solver to use for
  // best performance?
  Z3_solver theSolver = Z3_mk_simple_solver(builder->ctx);
  Z3_solver_inc_ref(builder->ctx, theSolver);
  Z3_solver_set_params(builder->ctx, theSolver, solverParameters);

  runStatusCode = SOLVER_RUN_STATUS_FAILURE;

  for (ConstraintManager::const_iterator it = query.constraints.begin(),
                                         ie = query.constraints.end();
       it != ie; ++it) {
    Z3_solver_assert(builder->ctx, theSolver, builder->construct(*it));
  }
  ++stats::queries;
  if (objects)
    ++stats::queryCounterexamples;

  Z3ASTHandle z3QueryExpr =
      Z3ASTHandle(builder->construct(query.expr), builder->ctx);

  // KLEE Queries are validity queries i.e.
  // ∀ X Constraints(X) → query(X)
  // but Z3 works in terms of satisfiability so instead we ask the
  // negation of the equivalent i.e.
  // ∃ X Constraints(X) ∧ ¬ query(X)
  Z3_solver_assert(
      builder->ctx, theSolver,
      Z3ASTHandle(Z3_mk_not(builder->ctx, z3QueryExpr), builder->ctx));

  ::Z3_lbool satisfiable = Z3_solver_check(builder->ctx, theSolver);
  runStatusCode = handleSolverResponse(theSolver, satisfiable, objects, values,
                                       hasSolution);

  Z3_solver_dec_ref(builder->ctx, theSolver);
  // Clear the builder's cache to prevent memory usage exploding.
  // By using ``autoClearConstructCache=false`` and clearning now
  // we allow Z3_ast expressions to be shared from an entire
  // ``Query`` rather than only sharing within a single call to
  // ``builder->construct()``.
  builder->clearConstructCache();

  if (runStatusCode == SolverImpl::SOLVER_RUN_STATUS_SUCCESS_SOLVABLE ||
      runStatusCode == SolverImpl::SOLVER_RUN_STATUS_SUCCESS_UNSOLVABLE) {
    if (hasSolution) {
      ++stats::queriesInvalid;
    } else {
      ++stats::queriesValid;
    }
    return true; // success
  }
  return false; // failed
}

SolverImpl::SolverRunStatus Z3SolverImpl::handleSolverResponse(
    ::Z3_solver theSolver, ::Z3_lbool satisfiable,
    const std::vector<const Array *> *objects,
    std::vector<std::vector<unsigned char> > *values, bool &hasSolution) {
  switch (satisfiable) {
  case Z3_L_TRUE: {
    hasSolution = true;
    if (!objects) {
      // No assignment is needed
      assert(values == NULL);
      return SolverImpl::SOLVER_RUN_STATUS_SUCCESS_SOLVABLE;
    }
    assert(values && "values cannot be nullptr");
    ::Z3_model theModel = Z3_solver_get_model(builder->ctx, theSolver);
    assert(theModel && "Failed to retrieve model");
    Z3_model_inc_ref(builder->ctx, theModel);
    values->reserve(objects->size());
    for (std::vector<const Array *>::const_iterator it = objects->begin(),
                                                    ie = objects->end();
         it != ie; ++it) {
      const Array *array = *it;
      std::vector<unsigned char> data;

      data.reserve(array->size);
      for (unsigned offset = 0; offset < array->size; offset++) {
        // We can't use Z3ASTHandle here so have to do ref counting manually
        ::Z3_ast arrayElementExpr;
        Z3ASTHandle initial_read = builder->getInitialRead(array, offset);

        bool successfulEval =
            Z3_model_eval(builder->ctx, theModel, initial_read,
                          /*model_completion=*/Z3_TRUE, &arrayElementExpr);
        assert(successfulEval && "Failed to evaluate model");
        Z3_inc_ref(builder->ctx, arrayElementExpr);
        assert(Z3_get_ast_kind(builder->ctx, arrayElementExpr) ==
                   Z3_NUMERAL_AST &&
               "Evaluated expression has wrong sort");

        int arrayElementValue = 0;
        bool successGet = Z3_get_numeral_int(builder->ctx, arrayElementExpr,
                                             &arrayElementValue);
        assert(successGet && "failed to get value back");
        assert(arrayElementValue >= 0 && arrayElementValue <= 255 &&
               "Integer from model is out of range");
        data.push_back(arrayElementValue);
        Z3_dec_ref(builder->ctx, arrayElementExpr);
      }
      values->push_back(data);
    }

    Z3_model_dec_ref(builder->ctx, theModel);
    return SolverImpl::SOLVER_RUN_STATUS_SUCCESS_SOLVABLE;
  }
  case Z3_L_FALSE:
    hasSolution = false;
    return SolverImpl::SOLVER_RUN_STATUS_SUCCESS_UNSOLVABLE;
  case Z3_L_UNDEF: {
    ::Z3_string reason =
        ::Z3_solver_get_reason_unknown(builder->ctx, theSolver);
    if (strcmp(reason, "timeout") == 0 || strcmp(reason, "canceled") == 0) {
      return SolverImpl::SOLVER_RUN_STATUS_TIMEOUT;
    }
    if (strcmp(reason, "unknown") == 0) {
      return SolverImpl::SOLVER_RUN_STATUS_FAILURE;
    }
    klee_warning("Unexpected solver failure. Reason is \"%s,\"\n", reason);
    abort();
  }
  default:
    llvm_unreachable("unhandled Z3 result");
  }
}

SolverImpl::SolverRunStatus Z3SolverImpl::getOperationStatusCode() {
  return runStatusCode;
}

void Z3SolverImpl::getModel(Z3Builder *builder, Z3_solver solver,
                            const std::vector<const Array *> &objects,
                            std::vector<std::vector<unsigned char> > &values) {
  Z3_model m = Z3_solver_get_model(builder->ctx, solver);

  values.reserve(objects.size());
  for (std::vector<const Array *>::const_iterator it = objects.begin(),
                                                  ie = objects.end();
       it != ie; ++it) {
    const Array *array = *it;
    std::vector<unsigned char> data;

    data.reserve(array->size);
    for (unsigned offset = 0; offset < array->size; offset++) {
      Z3_ast counter;
      Z3_ast initial_read =
          Z3_mk_bv2int(builder->ctx, builder->getInitialRead(array, offset), 0);
      Z3_model_eval(builder->ctx, m, initial_read, Z3_TRUE, &counter);
      int val = 0;
      Z3_get_numeral_int(builder->ctx, counter, &val);
      data.push_back(val);
    }

    values.push_back(data);
  }
}

SolverImpl::SolverRunStatus Z3SolverImpl::runAndGetCex(
    const Query &query, Z3Builder *builder, Z3_solver the_solver, Z3_ast q,
    const std::vector<const Array *> &objects,
    std::vector<std::vector<unsigned char> > &values, bool &hasSolution) {
  SolverRunStatus ret = SOLVER_RUN_STATUS_FAILURE;
  hasSolution = false;

  Z3_solver_assert(builder->ctx, the_solver, Z3_mk_not(builder->ctx, q));

  // llvm::errs() << "Solving: " << Z3_solver_to_string(builder->ctx,
  // the_solver)
  //              << "\n";

  switch (Z3_solver_check(builder->ctx, the_solver)) {
  case Z3_L_TRUE: {
    // The assertion is satisfiable (see Z3 API manual)
    getModel(builder, the_solver, objects, values);
    hasSolution = true;
    ret = SOLVER_RUN_STATUS_SUCCESS_SOLVABLE;
    break;
  }
  case Z3_L_FALSE: {
    unsatCore.clear();
    unsatCore = getUnsatCoreVector(query, builder, the_solver);
    ret = SOLVER_RUN_STATUS_SUCCESS_UNSOLVABLE;
    break;
  }
  default: { break; }
  }

  return ret;
}

std::vector<ref<Expr> >
Z3SolverImpl::getUnsatCoreVector(const Query &query, const Z3Builder *builder,
                                 const Z3_solver solver) {
  std::vector<ref<Expr> > local_unsat_core;
  Z3_ast_vector r = Z3_solver_get_unsat_core(builder->ctx, solver);
  for (unsigned int i = 0; i < Z3_ast_vector_size(builder->ctx, r); i++) {
    Z3_ast temp = Z3_ast_vector_get(builder->ctx, r, i);
    size_t constraint_index = 1;
    for (ConstraintManager::const_iterator it = query.constraints.begin(),
                                           ie = query.constraints.end();
         it != ie; ++it) {
      std::ostringstream convert;
      convert << constraint_index;
      std::string compare = "|" + convert.str() + "|";
      if (Z3_ast_to_string(builder->ctx, temp) == compare) {
        local_unsat_core.push_back(*it);
        break;
      }
      constraint_index++;
    }
  }
  return local_unsat_core;
}

std::vector<ref<Expr> > Z3SolverImpl::getUnsatCore() { return unsatCore; }
}
#endif // ENABLE_Z3
