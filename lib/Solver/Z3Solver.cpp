//===-- Z3Solver.cpp -------------------------------------------*- C++ -*-====//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Config/config.h"

#ifdef ENABLE_Z3

#include "Z3BitvectorBuilder.h"
#include "Z3Builder.h"
#include "Z3CoreBuilder.h"
#include "Z3Solver.h"

#include "klee/ADT/Incremental.h"
#include "klee/ADT/SparseStorage.h"
#include "klee/Expr/Assignment.h"
#include "klee/Expr/Constraints.h"
#include "klee/Expr/ExprUtil.h"
#include "klee/Solver/Solver.h"
#include "klee/Solver/SolverImpl.h"
#include "klee/Support/ErrorHandling.h"
#include "klee/Support/FileHandling.h"
#include "klee/Support/OptionCategories.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"

#include <csignal>

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

using ConstraintFrames = inc_vector<ref<Expr>>;
using ExprIncMap =
    inc_umap<Z3ASTHandle, ref<Expr>, Z3ASTHandleHash, Z3ASTHandleCmp>;
using Z3ASTIncMap =
    inc_umap<Z3ASTHandle, Z3ASTHandle, Z3ASTHandleHash, Z3ASTHandleCmp>;
using ExprIncSet =
    inc_uset<ref<Expr>, klee::util::ExprHash, klee::util::ExprCmp>;
using Z3ASTIncSet = inc_uset<Z3ASTHandle, Z3ASTHandleHash, Z3ASTHandleCmp>;

void dump(const ConstraintFrames &frames) {
  llvm::errs() << "frame sizes:";
  for (auto size : frames.frame_sizes) {
    llvm::errs() << " " << size;
  }
  llvm::errs() << "\n";
  llvm::errs() << "frames:\n";
  for (auto &x : frames.v) {
    llvm::errs() << x->toString() << "\n";
  }
}

class ConstraintQuery {
private:
  // this should be used when only query is needed, se comment below
  ref<Expr> expr;

public:
  // KLEE Queries are validity queries i.e.
  // ∀ X Constraints(X) → query(X)
  // but Z3 works in terms of satisfiability so instead we ask the
  // negation of the equivalent i.e.
  // ∃ X Constraints(X) ∧ ¬ query(X)
  // so this `constraints` field contains: Constraints(X) ∧ ¬ query(X)
  ConstraintFrames constraints;

  explicit ConstraintQuery() {}

  explicit ConstraintQuery(const ConstraintFrames &frames, const ref<Expr> &e)
      : expr(e), constraints(frames) {}
  explicit ConstraintQuery(ConstraintFrames &&frames, ref<Expr> &&e)
      : expr(std::move(e)), constraints(std::move(frames)) {}

  explicit ConstraintQuery(const Query &q, bool incremental) : expr(q.expr) {
    if (incremental) {
      for (auto &constraint : q.constraints.cs()) {
        constraints.v.push_back(constraint);
        constraints.push();
      }
    } else {
      const auto &other = q.constraints.cs();
      constraints.v.reserve(other.size());
      constraints.v.insert(constraints.v.end(), other.begin(), other.end());
    }
    if (q.expr->getWidth() == Expr::Bool && !q.expr->isFalse())
      constraints.v.push_back(NotExpr::create(q.expr));
  }

  size_t size() const { return constraints.v.size(); }

  ref<Expr> getOriginalQueryExpr() const { return expr; }

  ConstraintQuery withFalse() const {
    return ConstraintQuery(ConstraintFrames(constraints), Expr::createFalse());
  }

  std::vector<const Array *> gatherArrays() const {
    std::vector<const Array *> arrays;
    findObjects(constraints.v.begin(), constraints.v.end(), arrays);
    return arrays;
  }
};

enum class ObjectAssignment {
  NotNeeded,
  NeededForObjectsFromEnv,
  NeededForObjectsFromQuery
};

struct Z3SolverEnv {
  using arr_vec = std::vector<const Array *>;
  inc_vector<const Array *> objects;
  arr_vec objectsForGetModel;
  inc_vector<Z3ASTHandle> z3_ast_expr_constraints;
  ExprIncMap z3_ast_expr_to_klee_expr;
  Z3ASTIncMap expr_to_track;
  inc_umap<const Array *, ExprHashSet> usedArrayBytes;
  ExprIncSet symbolicObjects;

  explicit Z3SolverEnv(){};
  explicit Z3SolverEnv(const arr_vec &objects);

  void pop(size_t popSize);
  void push();
  void clear();

  const arr_vec *getObjectsForGetModel(ObjectAssignment oa) const;
};

Z3SolverEnv::Z3SolverEnv(const std::vector<const Array *> &objects)
    : objectsForGetModel(objects) {}

void Z3SolverEnv::pop(size_t popSize) {
  if (popSize == 0)
    return;
  objects.pop(popSize);
  objectsForGetModel.clear();
  z3_ast_expr_constraints.pop(popSize);
  z3_ast_expr_to_klee_expr.pop(popSize);
  expr_to_track.pop(popSize);
  usedArrayBytes.pop(popSize);
  symbolicObjects.pop(popSize);
}

void Z3SolverEnv::push() {
  objects.push();
  z3_ast_expr_constraints.push();
  z3_ast_expr_to_klee_expr.push();
  expr_to_track.push();
  usedArrayBytes.push();
  symbolicObjects.push();
}

void Z3SolverEnv::clear() {
  objects.clear();
  objectsForGetModel.clear();
  z3_ast_expr_constraints.clear();
  z3_ast_expr_to_klee_expr.clear();
  expr_to_track.clear();
  usedArrayBytes.clear();
  symbolicObjects.clear();
}

const Z3SolverEnv::arr_vec *
Z3SolverEnv::getObjectsForGetModel(ObjectAssignment oa) const {
  switch (oa) {
  case ObjectAssignment::NotNeeded:
    return nullptr;
  case ObjectAssignment::NeededForObjectsFromEnv:
    return &objectsForGetModel;
  case ObjectAssignment::NeededForObjectsFromQuery:
    return &objects.v;
  }
}

class Z3SolverImpl : public SolverImpl {
protected:
  std::unique_ptr<Z3Builder> builder;
  ::Z3_params solverParameters;

private:
  Z3BuilderType builderType;
  time::Span timeout;
  SolverImpl::SolverRunStatus runStatusCode;
  std::unique_ptr<llvm::raw_fd_ostream> dumpedQueriesFile;
  // Parameter symbols
  ::Z3_symbol timeoutParamStrSymbol;
  ::Z3_symbol unsatCoreParamStrSymbol;

  bool internalRunSolver(const ConstraintQuery &query, Z3SolverEnv &env,
                         ObjectAssignment needObjects,
                         std::vector<SparseStorage<unsigned char>> *values,
                         ValidityCore *validityCore, bool &hasSolution);

  bool validateZ3Model(::Z3_solver &theSolver, ::Z3_model &theModel);

  SolverImpl::SolverRunStatus
  handleSolverResponse(::Z3_solver theSolver, ::Z3_lbool satisfiable,
                       const Z3SolverEnv &env, ObjectAssignment needObjects,
                       std::vector<SparseStorage<unsigned char>> *values,
                       bool &hasSolution);

protected:
  Z3SolverImpl(Z3BuilderType type);
  ~Z3SolverImpl();

  virtual Z3_solver initNativeZ3(const ConstraintQuery &query,
                                 Z3ASTIncSet &assertions) = 0;
  virtual void deinitNativeZ3(Z3_solver theSolver) = 0;
  virtual void push(Z3_context c, Z3_solver s) = 0;

  bool computeTruth(const ConstraintQuery &, Z3SolverEnv &env, bool &isValid);
  bool computeValue(const ConstraintQuery &, Z3SolverEnv &env,
                    ref<Expr> &result);
  bool computeInitialValues(const ConstraintQuery &, Z3SolverEnv &env,
                            std::vector<SparseStorage<unsigned char>> &values,
                            bool &hasSolution);
  bool check(const ConstraintQuery &query, Z3SolverEnv &env,
             ref<SolverResponse> &result);
  bool computeValidityCore(const ConstraintQuery &query, Z3SolverEnv &env,
                           ValidityCore &validityCore, bool &isValid);

public:
  char *getConstraintLog(const Query &) final;
  SolverImpl::SolverRunStatus getOperationStatusCode() final;
  void setCoreSolverTimeout(time::Span _timeout) final {
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

  // pass virtual functions to children
  using SolverImpl::check;
  using SolverImpl::computeInitialValues;
  using SolverImpl::computeTruth;
  using SolverImpl::computeValidityCore;
  using SolverImpl::computeValue;
};

void deleteNativeZ3(Z3_context ctx, Z3_solver theSolver) {
  Z3_solver_dec_ref(ctx, theSolver);
}

Z3SolverImpl::Z3SolverImpl(Z3BuilderType type)
    : builderType(type), runStatusCode(SolverImpl::SOLVER_RUN_STATUS_FAILURE) {
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

bool Z3SolverImpl::computeTruth(const ConstraintQuery &query, Z3SolverEnv &env,
                                bool &isValid) {
  bool hasSolution = false; // to remove compiler warning
  bool status = internalRunSolver(query, /*env=*/env,
                                  ObjectAssignment::NotNeeded, /*values=*/NULL,
                                  /*validityCore=*/NULL, hasSolution);
  isValid = !hasSolution;
  return status;
}

bool Z3SolverImpl::computeValue(const ConstraintQuery &query, Z3SolverEnv &env,
                                ref<Expr> &result) {
  std::vector<SparseStorage<unsigned char>> values;
  bool hasSolution;

  // Find the object used in the expression, and compute an assignment
  // for them.
  findSymbolicObjects(query.getOriginalQueryExpr(), env.objectsForGetModel);
  if (!computeInitialValues(query.withFalse(), env, values, hasSolution))
    return false;
  assert(hasSolution && "state has invalid constraint set");

  // Evaluate the expression with the computed assignment.
  Assignment a(env.objectsForGetModel, values);
  result = a.evaluate(query.getOriginalQueryExpr());

  return true;
}

bool Z3SolverImpl::computeInitialValues(
    const ConstraintQuery &query, Z3SolverEnv &env,
    std::vector<SparseStorage<unsigned char>> &values, bool &hasSolution) {
  return internalRunSolver(query, env,
                           ObjectAssignment::NeededForObjectsFromEnv, &values,
                           /*validityCore=*/NULL, hasSolution);
}

bool Z3SolverImpl::check(const ConstraintQuery &query, Z3SolverEnv &env,
                         ref<SolverResponse> &result) {
  std::vector<SparseStorage<unsigned char>> values;
  ValidityCore validityCore;
  bool hasSolution = false;
  bool status =
      internalRunSolver(query, env, ObjectAssignment::NeededForObjectsFromQuery,
                        &values, &validityCore, hasSolution);
  if (status) {
    result = hasSolution
                 ? (SolverResponse *)new InvalidResponse(env.objects.v, values)
                 : (SolverResponse *)new ValidResponse(validityCore);
  }
  return status;
}

bool Z3SolverImpl::computeValidityCore(const ConstraintQuery &query,
                                       Z3SolverEnv &env,
                                       ValidityCore &validityCore,
                                       bool &isValid) {
  bool hasSolution = false; // to remove compiler warning
  bool status =
      internalRunSolver(query, /*env=*/env, ObjectAssignment::NotNeeded,
                        /*values=*/NULL, &validityCore, hasSolution);
  isValid = !hasSolution;
  return status;
}

bool Z3SolverImpl::internalRunSolver(
    const ConstraintQuery &query, Z3SolverEnv &env,
    ObjectAssignment needObjects,
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

  runStatusCode = SolverImpl::SOLVER_RUN_STATUS_FAILURE;

  std::unordered_set<const Array *> all_constant_arrays_in_query;
  Z3ASTIncSet exprs;

  for (size_t i = 0; i < query.constraints.framesSize();
       i++, env.push(), exprs.push()) {
    ConstantArrayFinder constant_arrays_in_query;
    env.symbolicObjects.insert(query.constraints.begin(i),
                               query.constraints.end(i));
    // FIXME: findSymbolicObjects template does not support inc_uset::iterator
    //  findSymbolicObjects(env.symbolicObjects.begin(-1),
    //  env.symbolicObjects.end(-1), env.objects.v);
    std::vector<ref<Expr>> tmp(env.symbolicObjects.begin(-1),
                               env.symbolicObjects.end(-1));
    findSymbolicObjects(tmp.begin(), tmp.end(), env.objects.v);
    for (auto cs_it = query.constraints.begin(i),
              cs_ite = query.constraints.end(i);
         cs_it != cs_ite; cs_it++) {
      const auto &constraint = *cs_it;
      Z3ASTHandle z3Constraint = builder->construct(constraint);
      if (ProduceUnsatCore && validityCore) {
        Z3ASTHandle p = builder->buildFreshBoolConst();
        env.z3_ast_expr_to_klee_expr.insert({p, constraint});
        env.z3_ast_expr_constraints.v.push_back(p);
        env.expr_to_track[z3Constraint] = p;
      }

      exprs.insert(z3Constraint);

      constant_arrays_in_query.visit(constraint);

      std::vector<ref<ReadExpr>> reads;
      findReads(constraint, true, reads);
      for (const auto &readExpr : reads) {
        auto readFromArray = readExpr->updates.root;
        assert(readFromArray);
        env.usedArrayBytes[readFromArray].insert(readExpr->index);
      }
    }

    for (auto constant_array : constant_arrays_in_query.results) {
      // assert(builder->constant_array_assertions.count(constant_array) == 1 &&
      //        "Constant array found in query, but not handled by Z3Builder");
      if (all_constant_arrays_in_query.count(constant_array))
        continue;
      all_constant_arrays_in_query.insert(constant_array);
      const auto &cas = builder->constant_array_assertions[constant_array];
      exprs.insert(cas.begin(), cas.end());
    }

    // Assert an generated side constraints we have to this last so that all
    // other constraints have been traversed so we have all the side constraints
    // needed.
    exprs.insert(builder->sideConstraints.begin(),
                 builder->sideConstraints.end());
  }
  exprs.pop(1); // drop last empty frame

  ++stats::solverQueries;
  if (!env.objects.v.empty())
    ++stats::queryCounterexamples;

  Z3_solver theSolver = initNativeZ3(query, exprs);

  for (size_t i = 0; i < exprs.framesSize(); i++) {
    push(builder->ctx, theSolver);
    for (auto it = exprs.begin(i), ie = exprs.end(i); it != ie; ++it) {
      Z3ASTHandle expr = *it;
      if (env.expr_to_track.count(expr)) {
        Z3_solver_assert_and_track(builder->ctx, theSolver, expr,
                                   env.expr_to_track[expr]);
      } else {
        Z3_solver_assert(builder->ctx, theSolver, expr);
      }
    }
  }
  assert(!Z3_solver_get_num_scopes(builder->ctx, theSolver) ||
         Z3_solver_get_num_scopes(builder->ctx, theSolver) + 1 ==
             env.objects.framesSize());

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

  ::Z3_lbool satisfiable = Z3_solver_check(builder->ctx, theSolver);
  runStatusCode = handleSolverResponse(theSolver, satisfiable, env, needObjects,
                                       values, hasSolution);
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

    for (const auto &z3_constraint : env.z3_ast_expr_constraints.v) {
      if (z3_ast_expr_unsat_core.find(z3_constraint) !=
          z3_ast_expr_unsat_core.end()) {
        ref<Expr> constraint = env.z3_ast_expr_to_klee_expr[z3_constraint];
        unsatCore.insert(constraint);
      }
    }
    assert(validityCore && "validityCore cannot be nullptr");
    *validityCore = ValidityCore(unsatCore, query.getOriginalQueryExpr());

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

  deinitNativeZ3(theSolver);

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
    ::Z3_solver theSolver, ::Z3_lbool satisfiable, const Z3SolverEnv &env,
    ObjectAssignment needObjects,
    std::vector<SparseStorage<unsigned char>> *values, bool &hasSolution) {
  switch (satisfiable) {
  case Z3_L_TRUE: {
    hasSolution = true;
    auto objects = env.getObjectsForGetModel(needObjects);
    if (!objects) {
      // No assignment is needed
      assert(!values);
      return SolverImpl::SOLVER_RUN_STATUS_SUCCESS_SOLVABLE;
    }
    assert(values && "values cannot be nullptr");
    ::Z3_model theModel = Z3_solver_get_model(builder->ctx, theSolver);
    assert(theModel && "Failed to retrieve model");
    Z3_model_inc_ref(builder->ctx, theModel);
    values->reserve(objects->size());
    for (auto array : *objects) {
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

      if (env.usedArrayBytes.count(array)) {
        std::unordered_set<uint64_t> offsetValues;
        for (const ref<Expr> &offsetExpr : env.usedArrayBytes.at(array)) {
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

class Z3NonIncSolverImpl final : public Z3SolverImpl {
private:
public:
  Z3NonIncSolverImpl(Z3BuilderType type) : Z3SolverImpl(type) {}

  /// implementation of Z3SolverImpl interface
  Z3_solver initNativeZ3(const ConstraintQuery &query,
                         Z3ASTIncSet &assertions) override {
    Z3_solver theSolver = nullptr;
    auto arrays = query.gatherArrays();
    bool forceTactic = true;
    for (auto array : arrays) {
      if (isa<ConstantSource>(array->source) &&
          !isa<ConstantExpr>(array->size)) {
        forceTactic = false;
        break;
      }
    }

    auto ctx = builder->ctx;
    if (forceTactic) {
      Z3_probe probe = Z3_mk_probe(ctx, "is-qfaufbv");
      Z3_probe_inc_ref(ctx, probe);
      Z3_goal goal = Z3_mk_goal(ctx, false, false, false);
      Z3_goal_inc_ref(ctx, goal);

      for (auto constraint : assertions)
        Z3_goal_assert(ctx, goal, constraint);
      if (Z3_probe_apply(ctx, probe, goal))
        theSolver =
            Z3_mk_solver_for_logic(ctx, Z3_mk_string_symbol(ctx, "QF_AUFBV"));
      Z3_goal_dec_ref(ctx, goal);
      Z3_probe_dec_ref(ctx, probe);
    }
    if (!theSolver)
      theSolver = Z3_mk_solver(ctx);
    Z3_solver_inc_ref(ctx, theSolver);
    Z3_solver_set_params(ctx, theSolver, solverParameters);
    return theSolver;
  }
  void deinitNativeZ3(Z3_solver theSolver) override {
    deleteNativeZ3(builder->ctx, theSolver);
  }
  void push(Z3_context c, Z3_solver s) override {}

  /// implementation of the SolverImpl interface
  bool computeTruth(const Query &query, bool &isValid) override {
    Z3SolverEnv env;
    return Z3SolverImpl::computeTruth(ConstraintQuery(query, false), env,
                                      isValid);
  }
  bool computeValue(const Query &query, ref<Expr> &result) override {
    Z3SolverEnv env;
    return Z3SolverImpl::computeValue(ConstraintQuery(query, false), env,
                                      result);
  }
  bool computeInitialValues(const Query &query,
                            const std::vector<const Array *> &objects,
                            std::vector<SparseStorage<unsigned char>> &values,
                            bool &hasSolution) override {
    Z3SolverEnv env(objects);
    return Z3SolverImpl::computeInitialValues(ConstraintQuery(query, false),
                                              env, values, hasSolution);
  }
  bool check(const Query &query, ref<SolverResponse> &result) override {
    Z3SolverEnv env;
    return Z3SolverImpl::check(ConstraintQuery(query, false), env, result);
  }
  bool computeValidityCore(const Query &query, ValidityCore &validityCore,
                           bool &isValid) override {
    Z3SolverEnv env;
    return Z3SolverImpl::computeValidityCore(ConstraintQuery(query, false), env,
                                             validityCore, isValid);
  }
  void notifyStateTermination(std::uint32_t id) override {}
};

Z3Solver::Z3Solver(Z3BuilderType type)
    : Solver(std::make_unique<Z3NonIncSolverImpl>(type)) {}

struct ConstraintDistance {
  size_t toPopSize = 0;
  ConstraintQuery toPush;

  explicit ConstraintDistance() {}
  ConstraintDistance(const ConstraintQuery &q) : toPush(q) {}
  explicit ConstraintDistance(size_t toPopSize, const ConstraintQuery &q)
      : toPopSize(toPopSize), toPush(q) {}

  size_t getDistance() const { return toPopSize + toPush.size(); }

  bool isOnlyPush() const { return toPopSize == 0; }

  void dump() const {
    llvm::errs() << "ConstraintDistance: pop: " << toPopSize << "; push:\n";
    klee::dump(toPush.constraints);
  }
};

class Z3IncNativeSolver {
private:
  Z3_solver nativeSolver = nullptr;
  Z3_context ctx;
  Z3_params solverParameters;
  /// underlying solver frames
  /// saved only for calculating distances from next queries
  ConstraintFrames frames;

  void pop(size_t popSize);
  void push();

public:
  Z3SolverEnv env;
  std::uint32_t stateID = 0;
  bool isRecycled = false;

  Z3IncNativeSolver(Z3_context ctx, Z3_params solverParameters)
      : ctx(ctx), solverParameters(solverParameters) {}
  ~Z3IncNativeSolver();

  void clear();

  void distance(const ConstraintQuery &query, ConstraintDistance &delta) const;

  void popPush(ConstraintDistance &delta);

  Z3_solver getOrInit();

  bool isConsistent() const {
    auto num_scopes =
        nativeSolver ? Z3_solver_get_num_scopes(ctx, nativeSolver) : 0;
    bool consistentWithZ3 = num_scopes + 1 == frames.framesSize();
    assert(consistentWithZ3);
    bool constistentItself = frames.framesSize() == env.objects.framesSize();
    assert(constistentItself);
    return consistentWithZ3 && constistentItself;
  }

  void dump() const { ::klee::dump(frames); }
};

void Z3IncNativeSolver::pop(size_t popSize) {
  if (!nativeSolver || !popSize)
    return;
  Z3_solver_pop(ctx, nativeSolver, popSize);
}

void Z3IncNativeSolver::popPush(ConstraintDistance &delta) {
  env.pop(delta.toPopSize);
  pop(delta.toPopSize);
  frames.pop(delta.toPopSize);
  frames.extend(delta.toPush.constraints);
}

Z3_solver Z3IncNativeSolver::getOrInit() {
  if (nativeSolver == nullptr) {
    nativeSolver = Z3_mk_solver(ctx);
    Z3_solver_inc_ref(ctx, nativeSolver);
    Z3_solver_set_params(ctx, nativeSolver, solverParameters);
  }
  return nativeSolver;
}

Z3IncNativeSolver::~Z3IncNativeSolver() {
  if (nativeSolver != nullptr)
    deleteNativeZ3(ctx, nativeSolver);
}

void Z3IncNativeSolver::clear() {
  if (!nativeSolver)
    return;
  env.clear();
  frames.clear();
  Z3_solver_reset(ctx, nativeSolver);
  isRecycled = false;
}

void Z3IncNativeSolver::distance(const ConstraintQuery &query,
                                 ConstraintDistance &delta) const {
  auto sit = frames.v.begin();
  auto site = frames.v.end();
  auto qit = query.constraints.v.begin();
  auto qite = query.constraints.v.end();
  auto it = frames.begin();
  auto ite = frames.end();
  size_t intersect = 0;
  for (; it != ite && sit != site && qit != qite && *sit == *qit; it++) {
    size_t frame_size = *it;
    for (size_t i = 0;
         i < frame_size && sit != site && qit != qite && *sit == *qit;
         i++, sit++, qit++, intersect++) {
    }
  }
  for (; sit != site && qit != qite && *sit == *qit;
       sit++, qit++, intersect++) {
  }
  size_t toPop, extraTakeFromOther;
  ConstraintFrames d;
  if (sit == site) { // solver frames ended
    toPop = 0;
    extraTakeFromOther = 0;
  } else {
    frames.takeBefore(intersect, toPop, extraTakeFromOther);
  }
  query.constraints.takeAfter(intersect - extraTakeFromOther, d);
  ConstraintQuery q(std::move(d), query.getOriginalQueryExpr());
  delta = ConstraintDistance(toPop, std::move(q));
}

class Z3TreeSolverImpl final : public Z3SolverImpl {
private:
  using solvers_ty = std::vector<std::unique_ptr<Z3IncNativeSolver>>;
  using solvers_it = solvers_ty::iterator;

  const size_t maxSolvers;
  std::unique_ptr<Z3IncNativeSolver> currentSolver = nullptr;
  solvers_ty solvers;

  void findSuitableSolver(const ConstraintQuery &query,
                          ConstraintDistance &delta);
  void setSolver(solvers_it &it, bool recycle = false);
  ConstraintQuery prepare(const Query &q);

public:
  Z3TreeSolverImpl(Z3BuilderType type, size_t maxSolvers)
      : Z3SolverImpl(type), maxSolvers(maxSolvers){};

  /// implementation of Z3SolverImpl interface
  Z3_solver initNativeZ3(const ConstraintQuery &query,
                         Z3ASTIncSet &assertions) override {
    return currentSolver->getOrInit();
  }
  void deinitNativeZ3(Z3_solver theSolver) override {
    assert(currentSolver->isConsistent());
    solvers.push_back(std::move(currentSolver));
  }
  void push(Z3_context c, Z3_solver s) override { Z3_solver_push(c, s); }

  /// implementation of the SolverImpl interface
  bool computeTruth(const Query &query, bool &isValid) override;
  bool computeValue(const Query &query, ref<Expr> &result) override;
  bool computeInitialValues(const Query &query,
                            const std::vector<const Array *> &objects,
                            std::vector<SparseStorage<unsigned char>> &values,
                            bool &hasSolution) override;
  bool check(const Query &query, ref<SolverResponse> &result) override;
  bool computeValidityCore(const Query &query, ValidityCore &validityCore,
                           bool &isValid) override;

  void notifyStateTermination(std::uint32_t id) override;
};

void Z3TreeSolverImpl::setSolver(solvers_it &it, bool recycle) {
  assert(it != solvers.end());
  currentSolver = std::move(*it);
  solvers.erase(it);
  currentSolver->isRecycled = false;
  if (recycle)
    currentSolver->clear();
}

void Z3TreeSolverImpl::findSuitableSolver(const ConstraintQuery &query,
                                          ConstraintDistance &delta) {
  ConstraintDistance min_delta;
  auto min_distance = std::numeric_limits<size_t>::max();
  auto min_it = solvers.end();
  auto free_it = solvers.end();
  for (auto it = solvers.begin(), ite = min_it; it != ite; it++) {
    if ((*it)->isRecycled)
      free_it = it;
    (*it)->distance(query, delta);
    if (delta.isOnlyPush()) {
      setSolver(it);
      return;
    }
    auto distance = delta.getDistance();
    if (distance < min_distance) {
      min_delta = delta;
      min_distance = distance;
      min_it = it;
    }
  }
  if (solvers.size() < maxSolvers) {
    delta = ConstraintDistance(query);
    if (delta.getDistance() < min_distance) {
      // it is cheaper to create new solver
      if (free_it == solvers.end())
        currentSolver =
            std::make_unique<Z3IncNativeSolver>(builder->ctx, solverParameters);
      else
        setSolver(free_it, /*recycle=*/true);
      return;
    }
  }
  assert(min_it != solvers.end());
  delta = min_delta;
  setSolver(min_it);
}

ConstraintQuery Z3TreeSolverImpl::prepare(const Query &q) {
  ConstraintDistance delta;
  ConstraintQuery query(q, true);
  findSuitableSolver(query, delta);
  assert(currentSolver->isConsistent());
  currentSolver->stateID = q.id;
  currentSolver->popPush(delta);
  return delta.toPush;
}

bool Z3TreeSolverImpl::computeTruth(const Query &query, bool &isValid) {
  auto q = prepare(query);
  return Z3SolverImpl::computeTruth(q, currentSolver->env, isValid);
}

bool Z3TreeSolverImpl::computeValue(const Query &query, ref<Expr> &result) {
  auto q = prepare(query);
  return Z3SolverImpl::computeValue(q, currentSolver->env, result);
}

bool Z3TreeSolverImpl::computeInitialValues(
    const Query &query, const std::vector<const Array *> &objects,
    std::vector<SparseStorage<unsigned char>> &values, bool &hasSolution) {
  auto q = prepare(query);
  currentSolver->env.objectsForGetModel = objects;
  return Z3SolverImpl::computeInitialValues(q, currentSolver->env, values,
                                            hasSolution);
}

bool Z3TreeSolverImpl::check(const Query &query, ref<SolverResponse> &result) {
  auto q = prepare(query);
  return Z3SolverImpl::check(q, currentSolver->env, result);
}

bool Z3TreeSolverImpl::computeValidityCore(const Query &query,
                                           ValidityCore &validityCore,
                                           bool &isValid) {
  auto q = prepare(query);
  return Z3SolverImpl::computeValidityCore(q, currentSolver->env, validityCore,
                                           isValid);
}

void Z3TreeSolverImpl::notifyStateTermination(std::uint32_t id) {
  for (auto &s : solvers)
    if (s->stateID == id)
      s->isRecycled = true;
}

Z3TreeSolver::Z3TreeSolver(Z3BuilderType type, unsigned maxSolvers)
    : Solver(std::make_unique<Z3TreeSolverImpl>(type, maxSolvers)) {}

} // namespace klee
#endif // ENABLE_Z3
