//===-- Z3Solver.cpp -------------------------------------------*- C++ -*-====//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Config/config.h"
#include "klee/Support/ErrorHandling.h"
#include "klee/Support/FileHandling.h"
#include "klee/Support/OptionCategories.h"

#include <csignal>

#ifdef ENABLE_Z3

#include "Z3BitvectorBuilder.h"
#include "Z3Builder.h"
#include "Z3CoreBuilder.h"
#include "Z3Solver.h"

#include "klee/ADT/SparseStorage.h"
#include "klee/Expr/Assignment.h"
#include "klee/Expr/Constraints.h"
#include "klee/Expr/ExprUtil.h"
#include "klee/Solver/Solver.h"
#include "klee/Solver/SolverImpl.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"

#include <memory>
#include <unordered_map>

namespace {
// NOTE: Very useful for debugging Z3 behaviour. These files can be given to
// the z3 binary to replay all Z3 API calls using its `-log` option.
llvm::cl::opt<std::string> Z3LogInteractionFile(
    "debug-z3-log-api-interaction", llvm::cl::init(""),
    llvm::cl::desc("Log API interaction with Z3 to the specified path"),
    llvm::cl::cat(klee::SolvingCat));

llvm::cl::opt<std::string> Z3QueryDumpFile(
    "debug-z3-dump-queries", llvm::cl::init(""),
    llvm::cl::desc(
        "Dump Z3's representation of the query to the specified path"),
    llvm::cl::cat(klee::SolvingCat));

llvm::cl::opt<bool> Z3ValidateModels(
    "debug-z3-validate-models", llvm::cl::init(false),
    llvm::cl::desc(
        "When generating Z3 models validate these against the query"),
    llvm::cl::cat(klee::SolvingCat));

llvm::cl::opt<unsigned>
    Z3VerbosityLevel("debug-z3-verbosity", llvm::cl::init(0),
                     llvm::cl::desc("Z3 verbosity level (default=0)"),
                     llvm::cl::cat(klee::SolvingCat));
} // namespace

#include "klee/Support/CompilerWarning.h"
DISABLE_WARNING_PUSH
DISABLE_WARNING_DEPRECATED_DECLARATIONS
#include "llvm/Support/ErrorHandling.h"
DISABLE_WARNING_POP

namespace klee {

class Z3SolverImpl : public SolverImpl {
private:
  std::unique_ptr<Z3Builder> builder;
  Z3BuilderType builderType;
  time::Span timeout;
  SolverRunStatus runStatusCode;
  std::unique_ptr<llvm::raw_fd_ostream> dumpedQueriesFile;
  ::Z3_params solverParameters;
  // Parameter symbols
  ::Z3_symbol timeoutParamStrSymbol;
  ::Z3_symbol unsatCoreParamStrSymbol;

  bool internalRunSolver(const Query &,
                         const std::vector<const Array *> *objects,
                         std::vector<SparseStorage<unsigned char>> *values,
                         ValidityCore *validityCore, bool &hasSolution);
  bool validateZ3Model(::Z3_solver &theSolver, ::Z3_model &theModel);

public:
  Z3SolverImpl(Z3BuilderType type);
  ~Z3SolverImpl();

  char *getConstraintLog(const Query &);
  void setCoreSolverTimeout(time::Span _timeout) {
    timeout = _timeout;

    auto timeoutInMilliSeconds =
        static_cast<unsigned>((timeout.toMicroseconds() / 1000));
    if (!timeoutInMilliSeconds)
      timeoutInMilliSeconds = UINT_MAX;
    Z3_params_set_uint(builder->ctx, solverParameters, timeoutParamStrSymbol,
                       timeoutInMilliSeconds);
  }
  void enableUnsatCore() {
    Z3_params_set_bool(builder->ctx, solverParameters, unsatCoreParamStrSymbol,
                       Z3_TRUE);
  }
  void disableUnsatCore() {
    Z3_params_set_bool(builder->ctx, solverParameters, unsatCoreParamStrSymbol,
                       Z3_FALSE);
  }

  bool computeTruth(const Query &, bool &isValid);
  bool computeValue(const Query &, ref<Expr> &result);
  bool computeInitialValues(const Query &,
                            const std::vector<const Array *> &objects,
                            std::vector<SparseStorage<unsigned char>> &values,
                            bool &hasSolution);
  bool check(const Query &query, ref<SolverResponse> &result);
  bool computeValidityCore(const Query &query, ValidityCore &validityCore,
                           bool &isValid);
  SolverRunStatus handleSolverResponse(
      ::Z3_solver theSolver, ::Z3_lbool satisfiable,
      const std::vector<const Array *> *objects,
      std::vector<SparseStorage<unsigned char>> *values,
      const std::unordered_map<const Array *, ExprHashSet> &usedArrayBytes,
      bool &hasSolution);
  SolverRunStatus getOperationStatusCode();
};

Z3SolverImpl::Z3SolverImpl(Z3BuilderType type)
    : builderType(type), runStatusCode(SOLVER_RUN_STATUS_FAILURE) {
  switch (type) {
  case KLEE_CORE:
    builder = std::unique_ptr<Z3Builder>(new Z3CoreBuilder(
        /*autoClearConstructCache=*/false,
        /*z3LogInteractionFile=*/!Z3LogInteractionFile.empty()
            ? Z3LogInteractionFile.c_str()
            : nullptr));
    break;
  case KLEE_BITVECTOR:
    builder = std::unique_ptr<Z3Builder>(new Z3BitvectorBuilder(
        /*autoClearConstructCache=*/false,
        /*z3LogInteractionFile=*/!Z3LogInteractionFile.empty()
            ? Z3LogInteractionFile.c_str()
            : nullptr));
    break;
  }
  assert(builder && "unable to create Z3Builder");
  solverParameters = Z3_mk_params(builder->ctx);
  Z3_params_inc_ref(builder->ctx, solverParameters);
  timeoutParamStrSymbol = Z3_mk_string_symbol(builder->ctx, "timeout");
  setCoreSolverTimeout(timeout);
  unsatCoreParamStrSymbol = Z3_mk_string_symbol(builder->ctx, "unsat_core");

  // HACK: This changes Z3's handling of the `to_ieee_bv` function so that
  // we get a signal bit pattern interpretation for NaN. At the time of writing
  // without this option Z3 sometimes generates models which don't satisfy the
  // original constraints.
  //
  // See https://github.com/Z3Prover/z3/issues/740 .
  // https://github.com/Z3Prover/z3/issues/507
  Z3_global_param_set("rewriter.hi_fp_unspecified", "true");

  if (!Z3QueryDumpFile.empty()) {
    std::string error;
    dumpedQueriesFile = klee_open_output_file(Z3QueryDumpFile, error);
    if (!dumpedQueriesFile) {
      klee_error("Error creating file for dumping Z3 queries: %s",
                 error.c_str());
    }
    klee_message("Dumping Z3 queries to \"%s\"", Z3QueryDumpFile.c_str());
  }

  // Set verbosity
  if (Z3VerbosityLevel > 0) {
    std::string underlyingString;
    llvm::raw_string_ostream ss(underlyingString);
    ss << Z3VerbosityLevel;
    ss.flush();
    Z3_global_param_set("verbose", underlyingString.c_str());
  }
}

Z3SolverImpl::~Z3SolverImpl() {
  Z3_params_dec_ref(builder->ctx, solverParameters);
}

Z3Solver::Z3Solver(Z3BuilderType type)
    : Solver(std::make_unique<Z3SolverImpl>(type)) {}

char *Z3Solver::getConstraintLog(const Query &query) {
  return impl->getConstraintLog(query);
}

void Z3Solver::setCoreSolverTimeout(time::Span timeout) {
  impl->setCoreSolverTimeout(timeout);
}

char *Z3SolverImpl::getConstraintLog(const Query &query) {
  std::vector<Z3ASTHandle> assumptions;
  // We use a different builder here because we don't want to interfere
  // with the solver's builder because it may change the solver builder's
  // cache.
  // NOTE: The builder does not set `z3LogInteractionFile` to avoid conflicting
  // with whatever the solver's builder is set to do.
  std::unique_ptr<Z3Builder> temp_builder;
  switch (builderType) {
  case KLEE_CORE:
    temp_builder = std::make_unique<Z3CoreBuilder>(
        /*autoClearConstructCache=*/false,
        /*z3LogInteractionFile=*/nullptr);
    break;
  case KLEE_BITVECTOR:
    temp_builder = std::make_unique<Z3BitvectorBuilder>(
        /*autoClearConstructCache=*/false,
        /*z3LogInteractionFile=*/nullptr);
    break;
  }
  ConstantArrayFinder constant_arrays_in_query;
  for (auto const &constraint : query.constraints.cs()) {
    assumptions.push_back(temp_builder->construct(constraint));
    constant_arrays_in_query.visit(constraint);
  }

  // KLEE Queries are validity queries i.e.
  // ∀ X Constraints(X) → query(X)
  // but Z3 works in terms of satisfiability so instead we ask the
  // the negation of the equivalent i.e.
  // ∃ X Constraints(X) ∧ ¬ query(X)
  Z3ASTHandle formula = Z3ASTHandle(
      Z3_mk_not(temp_builder->ctx, temp_builder->construct(query.expr)),
      temp_builder->ctx);
  constant_arrays_in_query.visit(query.expr);

  for (auto const &constant_array : constant_arrays_in_query.results) {
    assert(temp_builder->constant_array_assertions.count(constant_array) == 1 &&
           "Constant array found in query, but not handled by Z3Builder");
    for (auto const &arrayIndexValueExpr :
         temp_builder->constant_array_assertions[constant_array]) {
      assumptions.push_back(arrayIndexValueExpr);
    }
  }

  ::Z3_ast *assumptionsArray = NULL;
  int numAssumptions = assumptions.size();
  if (numAssumptions) {
    assumptionsArray = (::Z3_ast *)malloc(sizeof(::Z3_ast) * numAssumptions);
    for (int index = 0; index < numAssumptions; ++index) {
      assumptionsArray[index] = (::Z3_ast)assumptions[index];
    }
  }

  ::Z3_string result = Z3_benchmark_to_smtlib_string(
      temp_builder->ctx,
      /*name=*/"Emited by klee::Z3SolverImpl::getConstraintLog()",
      /*logic=*/"",
      /*status=*/"unknown",
      /*attributes=*/"",
      /*num_assumptions=*/numAssumptions,
      /*assumptions=*/assumptionsArray,
      /*formula=*/formula);

  if (numAssumptions)
    free(assumptionsArray);

  // We need to trigger a dereference before the `temp_builder` gets destroyed.
  // We do this indirectly by emptying `assumptions` and assigning to
  // `formula`.
  assumptions.clear();
  formula = Z3ASTHandle(NULL, temp_builder->ctx);
  // Client is responsible for freeing the returned C-string
  return strdup(result);
}

bool Z3SolverImpl::computeTruth(const Query &query, bool &isValid) {
  bool hasSolution = false; // to remove compiler warning
  bool status = internalRunSolver(query, /*objects=*/NULL, /*values=*/NULL,
                                  /*validityCore=*/NULL, hasSolution);
  isValid = !hasSolution;
  return status;
}

bool Z3SolverImpl::computeValue(const Query &query, ref<Expr> &result) {
  std::vector<const Array *> objects;
  std::vector<SparseStorage<unsigned char>> values;
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
    std::vector<SparseStorage<unsigned char>> &values, bool &hasSolution) {
  return internalRunSolver(query, &objects, &values, /*validityCore=*/NULL,
                           hasSolution);
}

bool Z3SolverImpl::check(const Query &query, ref<SolverResponse> &result) {
  ExprHashSet expressions;
  expressions.insert(query.constraints.cs().begin(),
                     query.constraints.cs().end());
  expressions.insert(query.expr);

  std::vector<const Array *> objects;
  findSymbolicObjects(expressions.begin(), expressions.end(), objects);
  std::vector<SparseStorage<unsigned char>> values;

  ValidityCore validityCore;

  bool hasSolution = false;

  bool status =
      internalRunSolver(query, &objects, &values, &validityCore, hasSolution);
  if (status) {
    result = hasSolution
                 ? (SolverResponse *)new InvalidResponse(objects, values)
                 : (SolverResponse *)new ValidResponse(validityCore);
  }
  return status;
}

bool Z3SolverImpl::computeValidityCore(const Query &query,
                                       ValidityCore &validityCore,
                                       bool &isValid) {
  bool hasSolution = false; // to remove compiler warning
  bool status = internalRunSolver(query, /*objects=*/NULL, /*values=*/NULL,
                                  &validityCore, hasSolution);
  isValid = !hasSolution;
  return status;
}

bool Z3SolverImpl::internalRunSolver(
    const Query &query, const std::vector<const Array *> *objects,
    std::vector<SparseStorage<unsigned char>> *values,
    ValidityCore *validityCore, bool &hasSolution) {

  if (ProduceUnsatCore && validityCore) {
    enableUnsatCore();
  } else {
    disableUnsatCore();
  }

  TimerStatIncrementer t(stats::queryTime);
  // NOTE: Z3 will switch to using a slower solver internally if push/pop are
  // used so for now it is likely that creating a new solver each time is the
  // right way to go until Z3 changes its behaviour.
  //
  // TODO: Investigate using a custom tactic as described in
  // https://github.com/klee/klee/issues/653

  Z3_goal goal = Z3_mk_goal(builder->ctx, false, false, false);
  Z3_goal_inc_ref(builder->ctx, goal);

  // TODO: make a RAII
  Z3_probe probe = Z3_mk_probe(builder->ctx, "is-qfaufbv");
  Z3_probe_inc_ref(builder->ctx, probe);

  runStatusCode = SOLVER_RUN_STATUS_FAILURE;

  ConstantArrayFinder constant_arrays_in_query;
  std::vector<Z3ASTHandle> z3_ast_expr_constraints;
  std::unordered_map<Z3ASTHandle, ref<Expr>, Z3ASTHandleHash, Z3ASTHandleCmp>
      z3_ast_expr_to_klee_expr;

  std::unordered_map<Z3ASTHandle, Z3ASTHandle, Z3ASTHandleHash, Z3ASTHandleCmp>
      expr_to_track;
  std::unordered_set<Z3ASTHandle, Z3ASTHandleHash, Z3ASTHandleCmp> exprs;

  for (auto const &constraint : query.constraints.cs()) {
    Z3ASTHandle z3Constraint = builder->construct(constraint);
    if (ProduceUnsatCore && validityCore) {
      Z3ASTHandle p = builder->buildFreshBoolConst();
      z3_ast_expr_to_klee_expr.insert({p, constraint});
      z3_ast_expr_constraints.push_back(p);
      expr_to_track[z3Constraint] = p;
    }

    Z3_goal_assert(builder->ctx, goal, z3Constraint);
    exprs.insert(z3Constraint);

    constant_arrays_in_query.visit(constraint);
  }
  ++stats::solverQueries;
  if (objects)
    ++stats::queryCounterexamples;

  Z3ASTHandle z3QueryExpr =
      Z3ASTHandle(builder->construct(query.expr), builder->ctx);
  constant_arrays_in_query.visit(query.expr);

  for (auto const &constant_array : constant_arrays_in_query.results) {
    assert(builder->constant_array_assertions.count(constant_array) == 1 &&
           "Constant array found in query, but not handled by Z3Builder");
    for (auto const &arrayIndexValueExpr :
         builder->constant_array_assertions[constant_array]) {
      Z3_goal_assert(builder->ctx, goal, arrayIndexValueExpr);
      exprs.insert(arrayIndexValueExpr);
    }
  }

  // KLEE Queries are validity queries i.e.
  // ∀ X Constraints(X) → query(X)
  // but Z3 works in terms of satisfiability so instead we ask the
  // negation of the equivalent i.e.
  // ∃ X Constraints(X) ∧ ¬ query(X)
  Z3ASTHandle z3NotQueryExpr =
      Z3ASTHandle(Z3_mk_not(builder->ctx, z3QueryExpr), builder->ctx);
  Z3_goal_assert(builder->ctx, goal, z3NotQueryExpr);

  // Assert an generated side constraints we have to this last so that all other
  // constraints have been traversed so we have all the side constraints needed.
  for (std::vector<Z3ASTHandle>::iterator it = builder->sideConstraints.begin(),
                                          ie = builder->sideConstraints.end();
       it != ie; ++it) {
    Z3ASTHandle sideConstraint = *it;
    Z3_goal_assert(builder->ctx, goal, sideConstraint);
    exprs.insert(sideConstraint);
  }

  std::vector<const Array *> arrays = query.gatherArrays();
  bool forceTactic = true;
  for (const Array *array : arrays) {
    if (isa<SymbolicSizeConstantSource>(array->source)) {
      forceTactic = false;
      break;
    }
  }

  Z3_solver theSolver;
  if (forceTactic && Z3_probe_apply(builder->ctx, probe, goal)) {
    theSolver = Z3_mk_solver_for_logic(
        builder->ctx, Z3_mk_string_symbol(builder->ctx, "QF_AUFBV"));
  } else {
    theSolver = Z3_mk_solver(builder->ctx);
  }
  Z3_solver_inc_ref(builder->ctx, theSolver);
  Z3_solver_set_params(builder->ctx, theSolver, solverParameters);

  for (std::unordered_set<Z3ASTHandle, Z3ASTHandleHash,
                          Z3ASTHandleCmp>::iterator it = exprs.begin(),
                                                    ie = exprs.end();
       it != ie; ++it) {
    Z3ASTHandle expr = *it;
    if (expr_to_track.count(expr)) {
      Z3_solver_assert_and_track(builder->ctx, theSolver, expr,
                                 expr_to_track[expr]);
    } else {
      Z3_solver_assert(builder->ctx, theSolver, expr);
    }
  }
  Z3_solver_assert(builder->ctx, theSolver, z3NotQueryExpr);

  if (dumpedQueriesFile) {
    *dumpedQueriesFile << "; start Z3 query\n";
    *dumpedQueriesFile << Z3_params_to_string(builder->ctx, solverParameters)
                       << "\n";
    *dumpedQueriesFile << Z3_solver_to_string(builder->ctx, theSolver);
    *dumpedQueriesFile << "(check-sat)\n";
    *dumpedQueriesFile << "(reset)\n";
    *dumpedQueriesFile << "; end Z3 query\n\n";
    dumpedQueriesFile->flush();
  }

  constraints_ty allConstraints = query.constraints.cs();
  allConstraints.insert(query.expr);
  std::unordered_map<const Array *, ExprHashSet> usedArrayBytes;
  for (auto constraint : allConstraints) {
    std::vector<ref<ReadExpr>> reads;
    findReads(constraint, true, reads);
    for (auto readExpr : reads) {
      const Array *readFromArray = readExpr->updates.root;
      assert(readFromArray);
      usedArrayBytes[readFromArray].insert(readExpr->index);
    }
  }

  ::Z3_lbool satisfiable = Z3_solver_check(builder->ctx, theSolver);
  runStatusCode = handleSolverResponse(theSolver, satisfiable, objects, values,
                                       usedArrayBytes, hasSolution);
  if (ProduceUnsatCore && validityCore && satisfiable == Z3_L_FALSE) {
    constraints_ty unsatCore;
    Z3_ast_vector z3_unsat_core =
        Z3_solver_get_unsat_core(builder->ctx, theSolver);
    Z3_ast_vector_inc_ref(builder->ctx, z3_unsat_core);

    unsigned size = Z3_ast_vector_size(builder->ctx, z3_unsat_core);
    std::unordered_set<Z3ASTHandle, Z3ASTHandleHash, Z3ASTHandleCmp>
        z3_ast_expr_unsat_core;

    for (unsigned index = 0; index < size; ++index) {
      Z3ASTHandle constraint = Z3ASTHandle(
          Z3_ast_vector_get(builder->ctx, z3_unsat_core, index), builder->ctx);
      z3_ast_expr_unsat_core.insert(constraint);
    }

    for (auto &z3_constraint : z3_ast_expr_constraints) {
      if (z3_ast_expr_unsat_core.find(z3_constraint) !=
          z3_ast_expr_unsat_core.end()) {
        ref<Expr> constraint = z3_ast_expr_to_klee_expr[z3_constraint];
        unsatCore.insert(constraint);
      }
    }
    assert(validityCore && "validityCore cannot be nullptr");
    *validityCore = ValidityCore(unsatCore, query.expr);

    Z3_ast_vector assertions =
        Z3_solver_get_assertions(builder->ctx, theSolver);
    Z3_ast_vector_inc_ref(builder->ctx, assertions);
    unsigned assertionsCount = Z3_ast_vector_size(builder->ctx, assertions);

    stats::validQueriesSize += assertionsCount;
    stats::validityCoresSize += size;
    ++stats::queryValidityCores;

    Z3_ast_vector_dec_ref(builder->ctx, z3_unsat_core);
    Z3_ast_vector_dec_ref(builder->ctx, assertions);
  }

  Z3_goal_dec_ref(builder->ctx, goal);
  Z3_probe_dec_ref(builder->ctx, probe);
  Z3_solver_dec_ref(builder->ctx, theSolver);

  // Clear the builder's cache to prevent memory usage exploding.
  // By using ``autoClearConstructCache=false`` and clearning now
  // we allow Z3_ast expressions to be shared from an entire
  // ``Query`` rather than only sharing within a single call to
  // ``builder->construct()``.
  builder->clearConstructCache();
  builder->clearSideConstraints();
  if (runStatusCode == SolverImpl::SOLVER_RUN_STATUS_SUCCESS_SOLVABLE ||
      runStatusCode == SolverImpl::SOLVER_RUN_STATUS_SUCCESS_UNSOLVABLE) {
    if (hasSolution) {
      ++stats::queriesInvalid;
    } else {
      ++stats::queriesValid;
    }
    return true; // success
  }
  if (runStatusCode == SolverImpl::SOLVER_RUN_STATUS_INTERRUPTED) {
    raise(SIGINT);
  }
  return false; // failed
}

SolverImpl::SolverRunStatus Z3SolverImpl::handleSolverResponse(
    ::Z3_solver theSolver, ::Z3_lbool satisfiable,
    const std::vector<const Array *> *objects,
    std::vector<SparseStorage<unsigned char>> *values,
    const std::unordered_map<const Array *, ExprHashSet> &usedArrayBytes,
    bool &hasSolution) {
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
      SparseStorage<unsigned char> data;

      ::Z3_ast arraySizeExpr;
      Z3_model_eval(builder->ctx, theModel, builder->construct(array->size),
                    Z3_TRUE, &arraySizeExpr);
      Z3_inc_ref(builder->ctx, arraySizeExpr);
      ::Z3_ast_kind arraySizeExprKind =
          Z3_get_ast_kind(builder->ctx, arraySizeExpr);
      assert(arraySizeExprKind == Z3_NUMERAL_AST &&
             "Evaluated size expression has wrong sort");
      uint64_t arraySize = 0;
      bool success =
          Z3_get_numeral_uint64(builder->ctx, arraySizeExpr, &arraySize);
      assert(success && "Failed to get size");

      data.resize(arraySize);
      if (usedArrayBytes.count(array)) {
        std::unordered_set<uint64_t> offsetValues;
        for (ref<Expr> offsetExpr : usedArrayBytes.at(array)) {
          ::Z3_ast arrayElementOffsetExpr;
          Z3_model_eval(builder->ctx, theModel, builder->construct(offsetExpr),
                        Z3_TRUE, &arrayElementOffsetExpr);
          Z3_inc_ref(builder->ctx, arrayElementOffsetExpr);
          ::Z3_ast_kind arrayElementOffsetExprKind =
              Z3_get_ast_kind(builder->ctx, arrayElementOffsetExpr);
          assert(arrayElementOffsetExprKind == Z3_NUMERAL_AST &&
                 "Evaluated size expression has wrong sort");
          uint64_t concretizedOffsetValue = 0;
          bool success = Z3_get_numeral_uint64(
              builder->ctx, arrayElementOffsetExpr, &concretizedOffsetValue);
          assert(success && "Failed to get size");
          offsetValues.insert(concretizedOffsetValue);
          Z3_dec_ref(builder->ctx, arrayElementOffsetExpr);
        }

        for (unsigned offset : offsetValues) {
          // We can't use Z3ASTHandle here so have to do ref counting manually
          ::Z3_ast arrayElementExpr;
          Z3ASTHandle initial_read = builder->getInitialRead(array, offset);

          __attribute__((unused)) bool successfulEval =
              Z3_model_eval(builder->ctx, theModel, initial_read,
                            /*model_completion=*/Z3_TRUE, &arrayElementExpr);
          assert(successfulEval && "Failed to evaluate model");
          Z3_inc_ref(builder->ctx, arrayElementExpr);
          assert(Z3_get_ast_kind(builder->ctx, arrayElementExpr) ==
                     Z3_NUMERAL_AST &&
                 "Evaluated expression has wrong sort");

          int arrayElementValue = 0;
          __attribute__((unused)) bool successGet = Z3_get_numeral_int(
              builder->ctx, arrayElementExpr, &arrayElementValue);
          assert(successGet && "failed to get value back");
          assert(arrayElementValue >= 0 && arrayElementValue <= 255 &&
                 "Integer from model is out of range");
          data.store(offset, arrayElementValue);
          Z3_dec_ref(builder->ctx, arrayElementExpr);
        }
      }

      Z3_dec_ref(builder->ctx, arraySizeExpr);
      values->push_back(data);
    }

    // Validate the model if requested
    if (Z3ValidateModels) {
      bool success = validateZ3Model(theSolver, theModel);
      if (!success)
        abort();
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
    if (strcmp(reason, "timeout") == 0 || strcmp(reason, "canceled") == 0 ||
        strcmp(reason, "(resource limits reached)") == 0) {
      return SolverImpl::SOLVER_RUN_STATUS_TIMEOUT;
    }
    if (strcmp(reason, "unknown") == 0) {
      return SolverImpl::SOLVER_RUN_STATUS_FAILURE;
    }
    if (strcmp(reason, "smt tactic failed to show goal to be sat/unsat") == 0) {
      return SolverImpl::SOLVER_RUN_STATUS_FAILURE;
    }
    if (strcmp(reason, "interrupted from keyboard") == 0) {
      return SolverImpl::SOLVER_RUN_STATUS_INTERRUPTED;
    }
    klee_warning("Unexpected solver failure. Reason is \"%s,\"\n", reason);
    abort();
  }
  default:
    llvm_unreachable("unhandled Z3 result");
  }
}

bool Z3SolverImpl::validateZ3Model(::Z3_solver &theSolver,
                                   ::Z3_model &theModel) {
  bool success = true;
  ::Z3_ast_vector constraints =
      Z3_solver_get_assertions(builder->ctx, theSolver);
  Z3_ast_vector_inc_ref(builder->ctx, constraints);

  unsigned size = Z3_ast_vector_size(builder->ctx, constraints);

  for (unsigned index = 0; index < size; ++index) {
    Z3ASTHandle constraint = Z3ASTHandle(
        Z3_ast_vector_get(builder->ctx, constraints, index), builder->ctx);

    ::Z3_ast rawEvaluatedExpr;
    __attribute__((unused)) bool successfulEval =
        Z3_model_eval(builder->ctx, theModel, constraint,
                      /*model_completion=*/true, &rawEvaluatedExpr);
    assert(successfulEval && "Failed to evaluate model");

    // Use handle to do ref-counting.
    Z3ASTHandle evaluatedExpr(rawEvaluatedExpr, builder->ctx);

    Z3SortHandle sort =
        Z3SortHandle(Z3_get_sort(builder->ctx, evaluatedExpr), builder->ctx);
    assert(Z3_get_sort_kind(builder->ctx, sort) == Z3_BOOL_SORT &&
           "Evaluated expression has wrong sort");

    Z3_lbool evaluatedValue = Z3_get_bool_value(builder->ctx, evaluatedExpr);
    if (evaluatedValue != Z3_L_TRUE) {
      llvm::errs() << "Validating model failed:\n"
                   << "The expression:\n";
      constraint.dump();
      llvm::errs() << "evaluated to \n";
      evaluatedExpr.dump();
      llvm::errs() << "But should be true\n";
      success = false;
    }
  }

  if (!success) {
    llvm::errs() << "Solver state:\n"
                 << Z3_solver_to_string(builder->ctx, theSolver) << "\n";
    llvm::errs() << "Model:\n"
                 << Z3_model_to_string(builder->ctx, theModel) << "\n";
  }

  Z3_ast_vector_dec_ref(builder->ctx, constraints);
  return success;
}

SolverImpl::SolverRunStatus Z3SolverImpl::getOperationStatusCode() {
  return runStatusCode;
}
} // namespace klee
#endif // ENABLE_Z3
