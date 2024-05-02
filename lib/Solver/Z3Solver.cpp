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

#include "Z3Solver.h"
#include "Z3Builder.h"

#include "klee/Expr/Constraints.h"
#include "klee/Expr/Assignment.h"
#include "klee/Expr/ExprUtil.h"
#include "klee/Solver/Solver.h"
#include "klee/Solver/SolverImpl.h"
#include "klee/Solver/SolverCmdLine.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"

#include <memory>

namespace {
// NOTE: Very useful for debugging Z3 behaviour. These files can be given to
// the z3 binary to replay all Z3 API calls using its `-log` option.
llvm::cl::opt<std::string> Z3LogInteractionFile(
    "debug-z3-log-api-interaction", llvm::cl::init(""),
    llvm::cl::desc("Log API interaction with Z3 to the specified path"),
    llvm::cl::cat(klee::SolvingCat));

llvm::cl::opt<std::string> Z3QueryDumpFile(
    "debug-z3-dump-queries", llvm::cl::init(""),
    llvm::cl::desc("Dump Z3's representation of the query to the specified path"),
    llvm::cl::cat(klee::SolvingCat));

llvm::cl::opt<bool> Z3ValidateModels(
    "debug-z3-validate-models", llvm::cl::init(false),
    llvm::cl::desc("When generating Z3 models validate these against the query"),
    llvm::cl::cat(klee::SolvingCat));

llvm::cl::opt<bool> GlobalIncrementalBaseline(
    "global-incremental-baseline", llvm::cl::init(false),
    llvm::cl::desc("Use a global Solver2"),
    llvm::cl::cat(klee::SolvingCat));

llvm::cl::opt<bool> IncrementalBaseline(
    "incremental-baseline", llvm::cl::init(false),
    llvm::cl::desc("Use a local Solver2"),
    llvm::cl::cat(klee::SolvingCat));

llvm::cl::opt<unsigned>
    Z3VerbosityLevel("debug-z3-verbosity", llvm::cl::init(0),
                     llvm::cl::desc("Z3 verbosity level (default=0)"),
                     llvm::cl::cat(klee::SolvingCat));
}

#include "llvm/Support/ErrorHandling.h"

namespace klee {

class Z3SolverImpl : public SolverImpl {
private:
  std::vector<ConstraintSet> assertionStack;
  ConstraintSet ppStack;
  Z3_solver z3Solver;
  std::unique_ptr<Z3Builder> builder;
  time::Span timeout;
  SolverRunStatus runStatusCode;
  std::unique_ptr<llvm::raw_fd_ostream> dumpedQueriesFile;
  ::Z3_params solverParameters;
  // Parameter symbols
  ::Z3_symbol timeoutParamStrSymbol;

  bool internalRunSolverBasicStack(const Query &,
                                    const std::vector<const Array *> *,
                                    std::vector<std::vector<unsigned char> > *,
                                    bool &hasSolution);
  bool internalRunSolverLcpPp(const Query &,
                                    const std::vector<const Array *> *,
                                    std::vector<std::vector<unsigned char> > *,
                                    bool &hasSolution);
  bool internalRunSolverGlobalBaseline(const Query &,
                                       const std::vector<const Array *> *,
                                       std::vector<std::vector<unsigned char> > *,
                                       bool &hasSolution);
  bool internalRunSolver(const Query &,
                         const std::vector<const Array *> *objects,
                         std::vector<std::vector<unsigned char> > *values,
                         bool &hasSolution);
  bool validateZ3Model(::Z3_solver &theSolver, ::Z3_model &theModel);

public:
  Z3SolverImpl();
  ~Z3SolverImpl();

  std::string getConstraintLog(const Query &) override;
  void setCoreSolverTimeout(time::Span _timeout) {
    timeout = _timeout;

    auto timeoutInMilliSeconds = static_cast<unsigned>((timeout.toMicroseconds() / 1000));
    if (!timeoutInMilliSeconds)
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

  void push();
  void pop();
};

Z3SolverImpl::Z3SolverImpl()
    : builder(new Z3Builder(
          /*autoClearConstructCache=*/false,
          /*z3LogInteractionFileArg=*/Z3LogInteractionFile.size() > 0
              ? Z3LogInteractionFile.c_str()
              : NULL)),
      runStatusCode(SOLVER_RUN_STATUS_FAILURE) {
  assert(builder && "unable to create Z3Builder");
  solverParameters = Z3_mk_params(builder->ctx);
  Z3_params_inc_ref(builder->ctx, solverParameters);
  timeoutParamStrSymbol = Z3_mk_string_symbol(builder->ctx, "timeout");
  setCoreSolverTimeout(timeout); // TODO: Set incremental solver timeout?

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

  if (BasicStackSolver || GlobalIncrementalBaseline || LcpPpSolver) {
    z3Solver = Z3_mk_solver(builder->ctx);
    Z3_solver_inc_ref(builder->ctx, z3Solver);
    Z3_solver_set_params(builder->ctx, z3Solver, solverParameters);
  }
}

Z3SolverImpl::~Z3SolverImpl() {
  if (BasicStackSolver || GlobalIncrementalBaseline || LcpPpSolver) {
    Z3_solver_dec_ref(builder->ctx, z3Solver);
  }
  Z3_params_dec_ref(builder->ctx, solverParameters);
}

Z3Solver::Z3Solver() : Solver(std::make_unique<Z3SolverImpl>()) {}

std::string Z3Solver::getConstraintLog(const Query &query) {
  return impl->getConstraintLog(query);
}

void Z3Solver::setCoreSolverTimeout(time::Span timeout) {
  impl->setCoreSolverTimeout(timeout);
}

std::string Z3SolverImpl::getConstraintLog(const Query &query) {
  std::vector<Z3ASTHandle> assumptions;
  // We use a different builder here because we don't want to interfere
  // with the solver's builder because it may change the solver builder's
  // cache.
  // NOTE: The builder does not set `z3LogInteractionFile` to avoid conflicting
  // with whatever the solver's builder is set to do.
  Z3Builder temp_builder(/*autoClearConstructCache=*/false,
                         /*z3LogInteractionFile=*/NULL);
  ConstantArrayFinder constant_arrays_in_query;
  for (auto const &constraint : query.constraints) {
    assumptions.push_back(temp_builder.construct(constraint));
    constant_arrays_in_query.visit(constraint);
  }

  // KLEE Queries are validity queries i.e.
  // ∀ X Constraints(X) → query(X)
  // but Z3 works in terms of satisfiability so instead we ask the
  // the negation of the equivalent i.e.
  // ∃ X Constraints(X) ∧ ¬ query(X)
  Z3ASTHandle formula = Z3ASTHandle(
      Z3_mk_not(temp_builder.ctx, temp_builder.construct(query.expr)),
      temp_builder.ctx);
  constant_arrays_in_query.visit(query.expr);

  for (auto const &constant_array : constant_arrays_in_query.results) {
    assert(temp_builder.constant_array_assertions.count(constant_array) == 1 &&
           "Constant array found in query, but not handled by Z3Builder");
    for (auto const &arrayIndexValueExpr :
         temp_builder.constant_array_assertions[constant_array]) {
      assumptions.push_back(arrayIndexValueExpr);
    }
  }

  std::vector<::Z3_ast> raw_assumptions{assumptions.cbegin(),
                                        assumptions.cend()};
  ::Z3_string result = Z3_benchmark_to_smtlib_string(
      temp_builder.ctx,
      /*name=*/"Emited by klee::Z3SolverImpl::getConstraintLog()",
      /*logic=*/"",
      /*status=*/"unknown",
      /*attributes=*/"",
      /*num_assumptions=*/raw_assumptions.size(),
      /*assumptions=*/raw_assumptions.size() ? raw_assumptions.data() : nullptr,
      /*formula=*/formula);

  // We need to trigger a dereference before the `temp_builder` gets destroyed.
  // We do this indirectly by emptying `assumptions` and assigning to
  // `formula`.
  raw_assumptions.clear();
  assumptions.clear();
  formula = Z3ASTHandle(NULL, temp_builder.ctx);

  return {result};
}

bool Z3SolverImpl::computeTruth(const Query &query, bool &isValid) {
  bool hasSolution = false; // to remove compiler warning
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
  if (BasicStackSolver) {
    return internalRunSolverBasicStack(query, objects, values, hasSolution);
  }

  if (LcpPpSolver) {
    return internalRunSolverLcpPp(query, objects, values, hasSolution);
  }

  if (GlobalIncrementalBaseline) {
    return internalRunSolverGlobalBaseline(query, objects, values, hasSolution);
  }

  TimerStatIncrementer t(stats::queryTime);
  // NOTE: Z3 will switch to using a slower solver internally if push/pop are
  // used so for now it is likely that creating a new solver each time is the
  // right way to go until Z3 changes its behaviour.
  //
  // TODO: Investigate using a custom tactic as described in
  // https://github.com/klee/klee/issues/653
  Z3_solver theSolver = Z3_mk_solver(builder->ctx);

  if (IncrementalBaseline) {
    // Activate Solver2 with a push - no need to pop since the solver
    // is local.
    Z3_solver_push(builder->ctx, theSolver);
  }

  Z3_solver_inc_ref(builder->ctx, theSolver);
  Z3_solver_set_params(builder->ctx, theSolver, solverParameters);

  runStatusCode = SOLVER_RUN_STATUS_FAILURE;

  ConstantArrayFinder constant_arrays_in_query;
  for (auto const &constraint : query.constraints) {
    Z3_solver_assert(builder->ctx, theSolver, builder->construct(constraint));
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
      Z3_solver_assert(builder->ctx, theSolver, arrayIndexValueExpr);
    }
  }

  // KLEE Queries are validity queries i.e.
  // ∀ X Constraints(X) → query(X)
  // but Z3 works in terms of satisfiability so instead we ask the
  // negation of the equivalent i.e.
  // ∃ X Constraints(X) ∧ ¬ query(X)
  Z3_solver_assert(
      builder->ctx, theSolver,
      Z3ASTHandle(Z3_mk_not(builder->ctx, z3QueryExpr), builder->ctx));

  if (dumpedQueriesFile) {
    *dumpedQueriesFile << "; start Z3 query\n";
    *dumpedQueriesFile << Z3_solver_to_string(builder->ctx, theSolver);
    *dumpedQueriesFile << "(check-sat)\n";
    *dumpedQueriesFile << "(reset)\n";
    *dumpedQueriesFile << "; end Z3 query\n\n";
    dumpedQueriesFile->flush();
  }

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
  if (runStatusCode == SolverImpl::SOLVER_RUN_STATUS_INTERRUPTED) {
    raise(SIGINT);
  }
  return false; // failed
}

bool Z3SolverImpl::internalRunSolverGlobalBaseline(
    const Query &query, const std::vector<const Array *> *objects,
    std::vector<std::vector<unsigned char> > *values, bool &hasSolution) {
  runStatusCode = SOLVER_RUN_STATUS_FAILURE;
  TimerStatIncrementer t(stats::queryTime);

  Z3_solver_push(builder->ctx, z3Solver); 

  ConstantArrayFinder constant_arrays_in_query;
  for (auto const &constraint : query.constraints) {
    Z3_solver_assert(builder->ctx, z3Solver, builder->construct(constraint));
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
      Z3_solver_assert(builder->ctx, z3Solver, arrayIndexValueExpr);
    }
  }

  // KLEE Queries are validity queries i.e.
  // ∀ X Constraints(X) → query(X)
  // but Z3 works in terms of satisfiability so instead we ask the
  // negation of the equivalent i.e.
  // ∃ X Constraints(X) ∧ ¬ query(X)
  Z3_solver_assert(
      builder->ctx, z3Solver,
      Z3ASTHandle(Z3_mk_not(builder->ctx, z3QueryExpr), builder->ctx));

  if (dumpedQueriesFile) {
    *dumpedQueriesFile << "; start Z3 query\n";
    *dumpedQueriesFile << Z3_solver_to_string(builder->ctx, z3Solver);
    *dumpedQueriesFile << "(check-sat)\n";
    *dumpedQueriesFile << "(reset)\n";
    *dumpedQueriesFile << "; end Z3 query\n\n";
    dumpedQueriesFile->flush();
  }

  ::Z3_lbool satisfiable = Z3_solver_check(builder->ctx, z3Solver);
  runStatusCode = handleSolverResponse(z3Solver, satisfiable, objects, values,
                                       hasSolution);

  // Clear the builder's cache to prevent memory usage exploding.
  // By using ``autoClearConstructCache=false`` and clearning now
  // we allow Z3_ast expressions to be shared from an entire
  // ``Query`` rather than only sharing within a single call to
  // ``builder->construct()``.
  builder->clearConstructCache();

  Z3_solver_pop(builder->ctx, z3Solver, 1);

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

bool Z3SolverImpl::internalRunSolverBasicStack(
    const Query &query, const std::vector<const Array *> *objects,
    std::vector<std::vector<unsigned char> > *values, bool &hasSolution) {
  runStatusCode = SOLVER_RUN_STATUS_FAILURE;
  TimerStatIncrementer t(stats::queryTime);

  auto query_it = query.constraints.begin();
  if (Verbose) {
    klee_warning("Debugging assertion stack");
  }
  for (auto const &level : assertionStack) {
    if (Verbose) {
      klee_warning("%d ", level.size());
    }
    auto stack_it = level.begin();
    while (stack_it != level.end() && query_it != query.constraints.end() && !(*stack_it)->compare(*(*query_it))) {
      ++stack_it;
      ++query_it;
    }
    if (stack_it != level.end()) {
      klee_error("Old constraint set is not prefix of current one! Have you disabled optimiations?");
    }
  }
  if (Verbose) {
    klee_warning("Done debugging assertion stack\n");
  }

  { // Add the remaining query constraints.
    ConstantArrayFinder constant_arrays_in_query;
    while (query_it != query.constraints.end()) {
      assertionStack.back().push_back(*query_it);
      Z3_solver_assert(builder->ctx, z3Solver, builder->construct(*query_it));
      constant_arrays_in_query.visit(*query_it);
      ++query_it;
    }
    // It's okay to do these all in one go after (due to set semantics, it'll be good).
    // Think about the DFS pushing and popping to see why. No need to add to the stack.
    for (auto const &constant_array : constant_arrays_in_query.results) {
      assert(builder->constant_array_assertions.count(constant_array) == 1 &&
              "Constant array found in query, but not handled by Z3Builder");
      for (auto const &arrayIndexValueExpr :
            builder->constant_array_assertions[constant_array]) {
        Z3_solver_assert(builder->ctx, z3Solver, arrayIndexValueExpr);
      }
    }
  }

  ++stats::solverQueries;
  if (objects)
    ++stats::queryCounterexamples;

  // Push a level for constraints related to the query expression:
  // TODO: Can we benefet from incrementality over this?
  // TODO: Does this (or rather the corresponding pop) discard important lemmas learned?
  Z3_solver_push(builder->ctx, z3Solver);
  Z3ASTHandle z3QueryExpr =
      Z3ASTHandle(builder->construct(query.expr), builder->ctx);

  ConstantArrayFinder constant_arrays_in_query;
  constant_arrays_in_query.visit(query.expr);
  for (auto const &constant_array : constant_arrays_in_query.results) {
    assert(builder->constant_array_assertions.count(constant_array) == 1 &&
           "Constant array found in query, but not handled by Z3Builder");
    for (auto const &arrayIndexValueExpr :
         builder->constant_array_assertions[constant_array]) {
      Z3_solver_assert(builder->ctx, z3Solver, arrayIndexValueExpr);
    }
  }

  // KLEE Queries are validity queries i.e.
  // ∀ X Constraints(X) → query(X)
  // but Z3 works in terms of satisfiability so instead we ask the
  // negation of the equivalent i.e.
  // ∃ X Constraints(X) ∧ ¬ query(X)
  Z3_solver_assert(
      builder->ctx, z3Solver,
      Z3ASTHandle(Z3_mk_not(builder->ctx, z3QueryExpr), builder->ctx));

  if (dumpedQueriesFile) {
    *dumpedQueriesFile << "; start Z3 query\n";
    *dumpedQueriesFile << Z3_solver_to_string(builder->ctx, z3Solver);
    *dumpedQueriesFile << "(check-sat)\n";
    *dumpedQueriesFile << "(reset)\n";
    *dumpedQueriesFile << "; end Z3 query\n\n";
    dumpedQueriesFile->flush();
  }

  ::Z3_lbool satisfiable = Z3_solver_check(builder->ctx, z3Solver);
  runStatusCode = handleSolverResponse(z3Solver, satisfiable, objects, values,
                                       hasSolution);

  // Clear the builder's cache to prevent memory usage exploding.
  // By using ``autoClearConstructCache=false`` and clearning now
  // we allow Z3_ast expressions to be shared from an entire
  // ``Query`` rather than only sharing within a single call to
  // ``builder->construct()``.
  builder->clearConstructCache();

  // Pop the level relating to the query expression:
  Z3_solver_pop(builder->ctx, z3Solver, 1);
  

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

bool Z3SolverImpl::internalRunSolverLcpPp(
    const Query &query, const std::vector<const Array *> *objects,
    std::vector<std::vector<unsigned char> > *values, bool &hasSolution) {
  runStatusCode = SOLVER_RUN_STATUS_FAILURE;
  TimerStatIncrementer t(stats::queryTime);

  // TODO: Reduce duplication with STPSolverImpl::computeInitialValues.
  auto stack_it = ppStack.begin();
  auto query_it = query.constraints.begin();
  // LCP between the assertion stack and the query constraints.
  while (stack_it != ppStack.end() && query_it != query.constraints.end() && !(*stack_it)->compare(*(*query_it))) {
    ++stack_it;
    ++query_it;
  }
  // Pop off extra constraints from stack.
  size_t pops = std::distance(stack_it, ppStack.end());
  for (size_t i = 0; i < pops; ++i) {
    Z3_solver_pop(builder->ctx, z3Solver, 1);
    ppStack.pop_back();
  }
  // Add the remaining query constraints.
  while (query_it != query.constraints.end()) {
    Z3_solver_push(builder->ctx, z3Solver);
    ppStack.push_back(*query_it);
    Z3_solver_assert(builder->ctx, z3Solver, builder->construct(*query_it));

    ConstantArrayFinder constant_arrays_in_query;
    constant_arrays_in_query.visit(*query_it);
    // Add constant array assertions, NB: at the same level, to benefit from
    // incrementality over them. Downside is that we may assert the same
    // constraint multiple times.
    for (auto const &constant_array : constant_arrays_in_query.results) {
      assert(builder->constant_array_assertions.count(constant_array) == 1 &&
              "Constant array found in query, but not handled by Z3Builder");
      for (auto const &arrayIndexValueExpr :
            builder->constant_array_assertions[constant_array]) {
        Z3_solver_assert(builder->ctx, z3Solver, arrayIndexValueExpr);
      }
    }

    ++query_it;
  }

  ++stats::solverQueries;
  if (objects)
    ++stats::queryCounterexamples;

  // We don't persist the negation of the query expression to the assertion stack;
  // it is unintuitive that negation would aid future constraint sets.
  // Push a level for constraints related to the query expression:
  Z3_solver_push(builder->ctx, z3Solver);
  Z3ASTHandle z3QueryExpr =
      Z3ASTHandle(builder->construct(query.expr), builder->ctx);

  ConstantArrayFinder constant_arrays_in_query;
  constant_arrays_in_query.visit(query.expr);
  for (auto const &constant_array : constant_arrays_in_query.results) {
    assert(builder->constant_array_assertions.count(constant_array) == 1 &&
           "Constant array found in query, but not handled by Z3Builder");
    for (auto const &arrayIndexValueExpr :
         builder->constant_array_assertions[constant_array]) {
      Z3_solver_assert(builder->ctx, z3Solver, arrayIndexValueExpr);
    }
  }

  // KLEE Queries are validity queries i.e.
  // ∀ X Constraints(X) → query(X)
  // but Z3 works in terms of satisfiability so instead we ask the
  // negation of the equivalent i.e.
  // ∃ X Constraints(X) ∧ ¬ query(X)
  Z3_solver_assert(
      builder->ctx, z3Solver,
      Z3ASTHandle(Z3_mk_not(builder->ctx, z3QueryExpr), builder->ctx));

  if (dumpedQueriesFile) {
    *dumpedQueriesFile << "; start Z3 query\n";
    *dumpedQueriesFile << Z3_solver_to_string(builder->ctx, z3Solver);
    *dumpedQueriesFile << "(check-sat)\n";
    *dumpedQueriesFile << "(reset)\n";
    *dumpedQueriesFile << "; end Z3 query\n\n";
    dumpedQueriesFile->flush();
  }

  ::Z3_lbool satisfiable = Z3_solver_check(builder->ctx, z3Solver);
  runStatusCode = handleSolverResponse(z3Solver, satisfiable, objects, values,
                                       hasSolution);

  // Clear the builder's cache to prevent memory usage exploding.
  // By using ``autoClearConstructCache=false`` and clearning now
  // we allow Z3_ast expressions to be shared from an entire
  // ``Query`` rather than only sharing within a single call to
  // ``builder->construct()``.
  builder->clearConstructCache();

  // Pop the level relating to the query expression:
  Z3_solver_pop(builder->ctx, z3Solver, 1);

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

        __attribute__((unused))
        bool successfulEval =
            Z3_model_eval(builder->ctx, theModel, initial_read,
                          /*model_completion=*/true, &arrayElementExpr);
        assert(successfulEval && "Failed to evaluate model");
        Z3_inc_ref(builder->ctx, arrayElementExpr);
        assert(Z3_get_ast_kind(builder->ctx, arrayElementExpr) ==
                   Z3_NUMERAL_AST &&
               "Evaluated expression has wrong sort");

        int arrayElementValue = 0;
        __attribute__((unused))
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

bool Z3SolverImpl::validateZ3Model(::Z3_solver &theSolver, ::Z3_model &theModel) {
  bool success = true;
  ::Z3_ast_vector constraints =
      Z3_solver_get_assertions(builder->ctx, theSolver);
  Z3_ast_vector_inc_ref(builder->ctx, constraints);

  unsigned size = Z3_ast_vector_size(builder->ctx, constraints);

  for (unsigned index = 0; index < size; ++index) {
    Z3ASTHandle constraint = Z3ASTHandle(
        Z3_ast_vector_get(builder->ctx, constraints, index), builder->ctx);

    ::Z3_ast rawEvaluatedExpr;
    __attribute__((unused))
    bool successfulEval =
        Z3_model_eval(builder->ctx, theModel, constraint,
                      /*model_completion=*/true, &rawEvaluatedExpr);
    assert(successfulEval && "Failed to evaluate model");

    // Use handle to do ref-counting.
    Z3ASTHandle evaluatedExpr(rawEvaluatedExpr, builder->ctx);

    Z3SortHandle sort =
        Z3SortHandle(Z3_get_sort(builder->ctx, evaluatedExpr), builder->ctx);
    assert(Z3_get_sort_kind(builder->ctx, sort) == Z3_BOOL_SORT &&
           "Evaluated expression has wrong sort");

    Z3_lbool evaluatedValue =
        Z3_get_bool_value(builder->ctx, evaluatedExpr);
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
    llvm::errs() << "Solver state:\n" << Z3_solver_to_string(builder->ctx, theSolver) << "\n";
    llvm::errs() << "Model:\n" << Z3_model_to_string(builder->ctx, theModel) << "\n";
  }

  Z3_ast_vector_dec_ref(builder->ctx, constraints);
  return success;
}

SolverImpl::SolverRunStatus Z3SolverImpl::getOperationStatusCode() {
  return runStatusCode;
}

void Z3SolverImpl::push() {
  Z3_solver_push(builder->ctx, z3Solver);
  assertionStack.emplace_back();
}

void Z3SolverImpl::pop() {
  Z3_solver_pop(builder->ctx, z3Solver, 1);
  assertionStack.pop_back();
}

void Z3Solver::push() {
  if (!BasicStackSolver) {
    klee_error("Non incremental solver used in incremental mode");
  }

  static_cast<Z3SolverImpl *>(impl.get())->push();
}

void Z3Solver::pop() {
  if (!BasicStackSolver) {
    klee_error("Non incremental solver used in incremental mode");
  }

  static_cast<Z3SolverImpl *>(impl.get())->pop();
}

}
#endif // ENABLE_Z3
