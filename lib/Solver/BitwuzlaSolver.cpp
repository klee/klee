
//===-- BitwuzlaSolver.cpp ---------------------------------------*-C++-*-====//
//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Config/config.h"

#ifdef ENABLE_BITWUZLA

#include "BitwuzlaBuilder.h"
#include "BitwuzlaSolver.h"

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
#include <functional>
#include <optional>

#include "bitwuzla/cpp/bitwuzla.h"

namespace {
// NOTE: Very useful for debugging Bitwuzla behaviour. These files can be given
// to the Bitwuzla binary to replay all Bitwuzla API calls using its `-log`
// option.

llvm::cl::opt<bool> BitwuzlaValidateModels(
    "debug-bitwuzla-validate-models", llvm::cl::init(false),
    llvm::cl::desc(
        "When generating Bitwuzla models validate these against the query"),
    llvm::cl::cat(klee::SolvingCat));

llvm::cl::opt<unsigned> BitwuzlaVerbosityLevel(
    "debug-bitwuzla-verbosity", llvm::cl::init(0),
    llvm::cl::desc("Bitwuzla verbosity level (default=0)"),
    llvm::cl::cat(klee::SolvingCat));
} // namespace

#include "klee/Support/CompilerWarning.h"
DISABLE_WARNING_PUSH
DISABLE_WARNING_DEPRECATED_DECLARATIONS
#include "llvm/Support/ErrorHandling.h"
DISABLE_WARNING_POP

namespace {
bool interrupted = false;

void signal_handler(int signum) { interrupted = true; }
} // namespace

namespace klee {

class BitwuzlaTerminator : public bitwuzla::Terminator {
private:
  uint64_t time_limit_micro;
  time::Point start;

  bool _isTimeout = false;

public:
  BitwuzlaTerminator(uint64_t);
  bool terminate() override;

  bool isTimeout() const { return _isTimeout; }
};

BitwuzlaTerminator::BitwuzlaTerminator(uint64_t time_limit_micro)
    : Terminator(), time_limit_micro(time_limit_micro),
      start(time::getWallTime()) {}

bool BitwuzlaTerminator::terminate() {
  time::Point end = time::getWallTime();
  if ((end - start).toMicroseconds() >= time_limit_micro) {
    _isTimeout = true;
    return true;
  }
  if (interrupted) {
    return true;
  }
  return false;
}

using ConstraintFrames = inc_vector<ref<Expr>>;
using ExprIncMap = inc_umap<Term, ref<Expr>>;
using BitwuzlaASTIncMap = inc_umap<Term, Term>;
using ExprIncSet =
    inc_uset<ref<Expr>, klee::util::ExprHash, klee::util::ExprCmp>;
using BitwuzlaASTIncSet = inc_uset<Term>;

extern void dump(const ConstraintFrames &);

class ConstraintQuery {
private:
  // this should be used when only query is needed, se comment below
  ref<Expr> expr;

public:
  // KLEE Queries are validity queries i.e.
  // ∀ X Constraints(X) → query(X)
  // but Bitwuzla works in terms of satisfiability so instead we ask the
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

struct BitwuzlaSolverEnv {
  using arr_vec = std::vector<const Array *>;
  inc_vector<const Array *> objects;
  arr_vec objectsForGetModel;
  ExprIncMap bitwuzla_ast_expr_to_klee_expr;
  BitwuzlaASTIncSet expr_to_track;
  inc_umap<const Array *, ExprHashSet> usedArrayBytes;
  ExprIncSet symbolicObjects;

  explicit BitwuzlaSolverEnv() = default;
  explicit BitwuzlaSolverEnv(const arr_vec &objects);

  void pop(size_t popSize);
  void push();
  void clear();

  const arr_vec *getObjectsForGetModel(ObjectAssignment oa) const;
};

BitwuzlaSolverEnv::BitwuzlaSolverEnv(const arr_vec &objects)
    : objectsForGetModel(objects) {}

void BitwuzlaSolverEnv::pop(size_t popSize) {
  if (popSize == 0)
    return;
  objects.pop(popSize);
  objectsForGetModel.clear();
  bitwuzla_ast_expr_to_klee_expr.pop(popSize);
  expr_to_track.pop(popSize);
  usedArrayBytes.pop(popSize);
  symbolicObjects.pop(popSize);
}

void BitwuzlaSolverEnv::push() {
  objects.push();
  bitwuzla_ast_expr_to_klee_expr.push();
  expr_to_track.push();
  usedArrayBytes.push();
  symbolicObjects.push();
}

void BitwuzlaSolverEnv::clear() {
  objects.clear();
  objectsForGetModel.clear();
  bitwuzla_ast_expr_to_klee_expr.clear();
  expr_to_track.clear();
  usedArrayBytes.clear();
  symbolicObjects.clear();
}

const BitwuzlaSolverEnv::arr_vec *
BitwuzlaSolverEnv::getObjectsForGetModel(ObjectAssignment oa) const {
  switch (oa) {
  case ObjectAssignment::NotNeeded:
    return nullptr;
  case ObjectAssignment::NeededForObjectsFromEnv:
    return &objectsForGetModel;
  case ObjectAssignment::NeededForObjectsFromQuery:
    return &objects.v;
  default:
    llvm_unreachable("unknown object assignment");
  }
}

class BitwuzlaSolverImpl : public SolverImpl {
protected:
  std::unique_ptr<BitwuzlaBuilder> builder;
  Options solverParameters;

private:
  time::Span timeout;
  SolverImpl::SolverRunStatus runStatusCode;

  bool internalRunSolver(const ConstraintQuery &query, BitwuzlaSolverEnv &env,
                         ObjectAssignment needObjects,
                         std::vector<SparseStorage<unsigned char>> *values,
                         ValidityCore *validityCore, bool &hasSolution);

  SolverImpl::SolverRunStatus handleSolverResponse(
      Bitwuzla &theSolver, Result satisfiable, const BitwuzlaSolverEnv &env,
      ObjectAssignment needObjects,
      std::vector<SparseStorage<unsigned char>> *values, bool &hasSolution);

protected:
  BitwuzlaSolverImpl();

  virtual Bitwuzla &initNativeBitwuzla(const ConstraintQuery &query,
                                       BitwuzlaASTIncSet &assertions) = 0;
  virtual void deinitNativeBitwuzla(Bitwuzla &theSolver) = 0;
  virtual void push(Bitwuzla &s) = 0;

  bool computeTruth(const ConstraintQuery &, BitwuzlaSolverEnv &env,
                    bool &isValid);
  bool computeValue(const ConstraintQuery &, BitwuzlaSolverEnv &env,
                    ref<Expr> &result);
  bool computeInitialValues(const ConstraintQuery &, BitwuzlaSolverEnv &env,
                            std::vector<SparseStorage<unsigned char>> &values,
                            bool &hasSolution);
  bool check(const ConstraintQuery &query, BitwuzlaSolverEnv &env,
             ref<SolverResponse> &result);
  bool computeValidityCore(const ConstraintQuery &query, BitwuzlaSolverEnv &env,
                           ValidityCore &validityCore, bool &isValid);

public:
  char *getConstraintLog(const Query &) final;
  SolverImpl::SolverRunStatus getOperationStatusCode() final;
  void setCoreSolverTimeout(time::Span _timeout) final { timeout = _timeout; }
  void enableUnsatCore() {
    solverParameters.set(Option::PRODUCE_UNSAT_CORES, true);
  }
  void disableUnsatCore() {
    solverParameters.set(Option::PRODUCE_UNSAT_CORES, false);
  }

  // pass virtual functions to children
  using SolverImpl::check;
  using SolverImpl::computeInitialValues;
  using SolverImpl::computeTruth;
  using SolverImpl::computeValidityCore;
  using SolverImpl::computeValue;
};

void deleteNativeBitwuzla(std::optional<Bitwuzla> &theSolver) {
  theSolver.reset();
}

BitwuzlaSolverImpl::BitwuzlaSolverImpl()
    : runStatusCode(SolverImpl::SOLVER_RUN_STATUS_FAILURE) {
  builder = std::unique_ptr<BitwuzlaBuilder>(new BitwuzlaBuilder(
      /*autoClearConstructCache=*/false));
  assert(builder && "unable to create BitwuzlaBuilder");

  solverParameters.set(Option::PRODUCE_MODELS, true);

  setCoreSolverTimeout(timeout);

  if (ProduceUnsatCore) {
    enableUnsatCore();
  } else {
    disableUnsatCore();
  }

  // Set verbosity
  if (BitwuzlaVerbosityLevel > 0) {
    solverParameters.set(Option::VERBOSITY, BitwuzlaVerbosityLevel);
  }

  if (BitwuzlaValidateModels) {
    solverParameters.set(Option::DBG_CHECK_MODEL, true);
  }
}

char *BitwuzlaSolverImpl::getConstraintLog(const Query &query) {
  std::stringstream outputLog;

  // We use a different builder here because we don't want to interfere
  // with the solver's builder because it may change the solver builder's
  // cache.
  std::unique_ptr<BitwuzlaBuilder> tempBuilder(new BitwuzlaBuilder(false));
  ConstantArrayFinder constant_arrays_in_query;

  std::function<std::unordered_set<Term>(const Term &)> constantsIn =
      [&constantsIn](const Term &term) {
        if (term.is_const()) {
          return std::unordered_set<Term>{term};
        }
        std::unordered_set<Term> symbols;
        for (const auto &child : term.children()) {
          for (const auto &childsSymbol : constantsIn(child)) {
            symbols.emplace(childsSymbol);
          }
        }
        return symbols;
      };

  std::function<void(const Term &)> declareLog =
      [&outputLog](const Term &term) {
        outputLog << "(declare-fun " << term.str(10) << " () "
                  << term.sort().str() << ")\n";
      };
  std::function<void(const Term &)> assertLog = [&outputLog](const Term &term) {
    outputLog << "(assert " << term.str(10) << ")\n";
  };

  std::vector<Term> assertions;
  std::unordered_set<Term> assertionSymbols;

  for (auto const &constraint : query.constraints.cs()) {
    Term constraintTerm = tempBuilder->construct(constraint);
    assertions.push_back(std::move(constraintTerm));
    constant_arrays_in_query.visit(constraint);
  }

  // KLEE Queries are validity queries i.e.
  // ∀ X Constraints(X) → query(X)
  // but Bitwuzla works in terms of satisfiability so instead we ask the
  // the negation of the equivalent i.e.
  // ∃ X Constraints(X) ∧ ¬ query(X)
  assertions.push_back(
      mk_term(Kind::NOT, {tempBuilder->construct(query.expr)}));
  constant_arrays_in_query.visit(query.expr);

  for (auto const &constant_array : constant_arrays_in_query.results) {
    for (auto const &arrayIndexValueExpr :
         tempBuilder->constant_array_assertions[constant_array]) {
      assertions.push_back(arrayIndexValueExpr);
    }
  }

  for (const Term &constraintTerm : assertions) {
    std::unordered_set<Term> constantsInConstraintTerm =
        constantsIn(constraintTerm);
    assertionSymbols.insert(constantsInConstraintTerm.begin(),
                            constantsInConstraintTerm.end());
  }

  for (const Term &symbol : assertionSymbols) {
    declareLog(symbol);
  }
  for (const Term &constraint : assertions) {
    assertLog(constraint);
  }

  outputLog << "(check-sat)\n";

  // Client is responsible for freeing the returned C-string
  return strdup(outputLog.str().c_str());
}

bool BitwuzlaSolverImpl::computeTruth(const ConstraintQuery &query,
                                      BitwuzlaSolverEnv &env, bool &isValid) {
  bool hasSolution = false; // to remove compiler warning
  bool status = internalRunSolver(query, /*env=*/env,
                                  ObjectAssignment::NotNeeded, /*values=*/NULL,
                                  /*validityCore=*/NULL, hasSolution);
  isValid = !hasSolution;
  return status;
}

bool BitwuzlaSolverImpl::computeValue(const ConstraintQuery &query,
                                      BitwuzlaSolverEnv &env,
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

bool BitwuzlaSolverImpl::computeInitialValues(
    const ConstraintQuery &query, BitwuzlaSolverEnv &env,
    std::vector<SparseStorage<unsigned char>> &values, bool &hasSolution) {
  return internalRunSolver(query, env,
                           ObjectAssignment::NeededForObjectsFromEnv, &values,
                           /*validityCore=*/NULL, hasSolution);
}

bool BitwuzlaSolverImpl::check(const ConstraintQuery &query,
                               BitwuzlaSolverEnv &env,
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

bool BitwuzlaSolverImpl::computeValidityCore(const ConstraintQuery &query,
                                             BitwuzlaSolverEnv &env,
                                             ValidityCore &validityCore,
                                             bool &isValid) {
  bool hasSolution = false; // to remove compiler warning
  bool status =
      internalRunSolver(query, /*env=*/env, ObjectAssignment::NotNeeded,
                        /*values=*/NULL, &validityCore, hasSolution);
  isValid = !hasSolution;
  return status;
}

bool BitwuzlaSolverImpl::internalRunSolver(
    const ConstraintQuery &query, BitwuzlaSolverEnv &env,
    ObjectAssignment needObjects,
    std::vector<SparseStorage<unsigned char>> *values,
    ValidityCore *validityCore, bool &hasSolution) {
  TimerStatIncrementer t(stats::queryTime);
  runStatusCode = SolverImpl::SOLVER_RUN_STATUS_FAILURE;

  std::unordered_set<const Array *> all_constant_arrays_in_query;
  BitwuzlaASTIncSet exprs;

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
      Term bitwuzlaConstraint = builder->construct(constraint);
      if (ProduceUnsatCore && validityCore) {
        env.bitwuzla_ast_expr_to_klee_expr.insert(
            {bitwuzlaConstraint, constraint});
        env.expr_to_track.insert(bitwuzlaConstraint);
      }

      exprs.insert(bitwuzlaConstraint);

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

  // Prepare signal handler and terminator fot bitwuzla
  auto timeoutInMicroSeconds = static_cast<uint64_t>(timeout.toMicroseconds());
  if (!timeoutInMicroSeconds)
    timeoutInMicroSeconds = UINT_MAX;
  BitwuzlaTerminator terminator(timeoutInMicroSeconds);

  struct sigaction action {};
  struct sigaction old_action {};

  action.sa_handler = signal_handler;
  action.sa_flags = 0;
  sigaction(SIGINT, &action, &old_action);

  Bitwuzla &theSolver = initNativeBitwuzla(query, exprs);
  theSolver.configure_terminator(&terminator);

  for (size_t i = 0; i < exprs.framesSize(); i++) {
    push(theSolver);
    for (auto it = exprs.begin(i), ie = exprs.end(i); it != ie; ++it) {
      theSolver.assert_formula(*it);
    }
  }

  Result satisfiable = theSolver.check_sat();
  theSolver.configure_terminator(nullptr);
  runStatusCode = handleSolverResponse(theSolver, satisfiable, env, needObjects,
                                       values, hasSolution);
  sigaction(SIGINT, &old_action, nullptr);

  if (runStatusCode == SolverImpl::SOLVER_RUN_STATUS_FAILURE) {
    if (terminator.isTimeout()) {
      runStatusCode = SolverImpl::SOLVER_RUN_STATUS_TIMEOUT;
    }
    if (interrupted) {
      runStatusCode = SolverImpl::SOLVER_RUN_STATUS_INTERRUPTED;
    }
  }

  if (ProduceUnsatCore && validityCore && satisfiable == Result::UNSAT) {
    const std::vector<Term> bitwuzla_unsat_core = theSolver.get_unsat_core();
    const std::unordered_set<Term> bitwuzla_term_unsat_core(
        bitwuzla_unsat_core.begin(), bitwuzla_unsat_core.end());

    constraints_ty unsatCore;
    for (const auto &bitwuzla_constraint : env.expr_to_track) {
      if (bitwuzla_term_unsat_core.count(bitwuzla_constraint)) {
        ref<Expr> constraint =
            env.bitwuzla_ast_expr_to_klee_expr[bitwuzla_constraint];
        if (Expr::createIsZero(constraint) != query.getOriginalQueryExpr()) {
          unsatCore.insert(constraint);
        }
      }
    }
    assert(validityCore && "validityCore cannot be nullptr");
    *validityCore = ValidityCore(unsatCore, query.getOriginalQueryExpr());

    stats::validQueriesSize += theSolver.get_assertions().size();
    stats::validityCoresSize += bitwuzla_unsat_core.size();
    ++stats::queryValidityCores;
  }

  deinitNativeBitwuzla(theSolver);

  // Clear the builder's cache to prevent memory usage exploding.
  // By using ``autoClearConstructCache=false`` and clearning now
  // we allow Term expressions to be shared from an entire
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

SolverImpl::SolverRunStatus BitwuzlaSolverImpl::handleSolverResponse(
    Bitwuzla &theSolver, Result satisfiable, const BitwuzlaSolverEnv &env,
    ObjectAssignment needObjects,
    std::vector<SparseStorage<unsigned char>> *values, bool &hasSolution) {
  switch (satisfiable) {
  case Result::SAT: {
    hasSolution = true;
    auto objects = env.getObjectsForGetModel(needObjects);
    if (!objects) {
      // No assignment is needed
      assert(!values);
      return SolverImpl::SOLVER_RUN_STATUS_SUCCESS_SOLVABLE;
    }
    assert(values && "values cannot be nullptr");

    values->reserve(objects->size());
    for (auto array : *objects) {
      SparseStorage<unsigned char> data;

      if (env.usedArrayBytes.count(array)) {
        std::unordered_set<uint64_t> offsetValues;
        for (const ref<Expr> &offsetExpr : env.usedArrayBytes.at(array)) {
          Term arrayElementOffsetExpr =
              theSolver.get_value(builder->construct(offsetExpr));

          uint64_t concretizedOffsetValue =
              std::stoull(arrayElementOffsetExpr.value<std::string>(10));
          offsetValues.insert(concretizedOffsetValue);
        }

        for (unsigned offset : offsetValues) {
          // We can't use Term here so have to do ref counting manually
          Term initial_read = builder->getInitialRead(array, offset);
          Term initial_read_expr = theSolver.get_value(initial_read);

          uint64_t arrayElementValue =
              std::stoull(initial_read_expr.value<std::string>(10));
          data.store(offset, arrayElementValue);
        }
      }

      values->emplace_back(std::move(data));
    }

    assert(values->size() == objects->size());

    return SolverImpl::SOLVER_RUN_STATUS_SUCCESS_SOLVABLE;
  }
  case Result::UNSAT:
    hasSolution = false;
    return SolverImpl::SOLVER_RUN_STATUS_SUCCESS_UNSOLVABLE;
  case Result::UNKNOWN: {
    return SolverImpl::SOLVER_RUN_STATUS_FAILURE;
  }
  default:
    llvm_unreachable("unhandled Bitwuzla result");
  }
}

SolverImpl::SolverRunStatus BitwuzlaSolverImpl::getOperationStatusCode() {
  return runStatusCode;
}

class BitwuzlaNonIncSolverImpl final : public BitwuzlaSolverImpl {
private:
  std::optional<Bitwuzla> theSolver;

public:
  BitwuzlaNonIncSolverImpl() = default;

  /// implementation of BitwuzlaSolverImpl interface
  Bitwuzla &initNativeBitwuzla(const ConstraintQuery &query,
                               BitwuzlaASTIncSet &assertions) override {
    theSolver.emplace(solverParameters);
    return theSolver.value();
  }

  void deinitNativeBitwuzla(Bitwuzla &) override {
    deleteNativeBitwuzla(theSolver);
  }

  void push(Bitwuzla &s) override {}

  /// implementation of the SolverImpl interface
  bool computeTruth(const Query &query, bool &isValid) override {
    BitwuzlaSolverEnv env;
    return BitwuzlaSolverImpl::computeTruth(ConstraintQuery(query, false), env,
                                            isValid);
  }
  bool computeValue(const Query &query, ref<Expr> &result) override {
    BitwuzlaSolverEnv env;
    return BitwuzlaSolverImpl::computeValue(ConstraintQuery(query, false), env,
                                            result);
  }
  bool computeInitialValues(const Query &query,
                            const std::vector<const Array *> &objects,
                            std::vector<SparseStorage<unsigned char>> &values,
                            bool &hasSolution) override {
    BitwuzlaSolverEnv env(objects);
    return BitwuzlaSolverImpl::computeInitialValues(
        ConstraintQuery(query, false), env, values, hasSolution);
  }
  bool check(const Query &query, ref<SolverResponse> &result) override {
    BitwuzlaSolverEnv env;
    return BitwuzlaSolverImpl::check(ConstraintQuery(query, false), env,
                                     result);
  }
  bool computeValidityCore(const Query &query, ValidityCore &validityCore,
                           bool &isValid) override {
    BitwuzlaSolverEnv env;
    return BitwuzlaSolverImpl::computeValidityCore(
        ConstraintQuery(query, false), env, validityCore, isValid);
  }
  void notifyStateTermination(std::uint32_t id) override {}
};

BitwuzlaSolver::BitwuzlaSolver()
    : Solver(std::make_unique<BitwuzlaNonIncSolverImpl>()) {}

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

class BitwuzlaIncNativeSolver {
private:
  std::optional<Bitwuzla> nativeSolver;
  Options solverParameters;
  /// underlying solver frames
  /// saved only for calculating distances from next queries
  ConstraintFrames frames;

  void pop(size_t popSize);

public:
  BitwuzlaSolverEnv env;
  std::uint32_t stateID = 0;
  bool isRecycled = false;

  BitwuzlaIncNativeSolver(Options solverParameters)
      : solverParameters(solverParameters) {}
  ~BitwuzlaIncNativeSolver();

  void clear();

  void distance(const ConstraintQuery &query, ConstraintDistance &delta) const;

  void popPush(ConstraintDistance &delta);

  Bitwuzla &getOrInit();

  bool isConsistent() const {
    return frames.framesSize() == env.objects.framesSize();
  }

  void dump() const { ::klee::dump(frames); }
};

void BitwuzlaIncNativeSolver::pop(size_t popSize) {
  if (!nativeSolver.has_value() || !popSize)
    return;
  nativeSolver.value().pop(popSize);
}

void BitwuzlaIncNativeSolver::popPush(ConstraintDistance &delta) {
  env.pop(delta.toPopSize);
  pop(delta.toPopSize);
  frames.pop(delta.toPopSize);
  frames.extend(delta.toPush.constraints);
}

Bitwuzla &BitwuzlaIncNativeSolver::getOrInit() {
  if (!nativeSolver.has_value()) {
    nativeSolver.emplace(solverParameters);
  }
  return nativeSolver.value();
}

BitwuzlaIncNativeSolver::~BitwuzlaIncNativeSolver() {
  if (nativeSolver.has_value()) {
    deleteNativeBitwuzla(nativeSolver);
  }
}

void BitwuzlaIncNativeSolver::clear() {
  if (!nativeSolver.has_value())
    return;
  env.clear();
  frames.clear();
  nativeSolver.emplace(solverParameters);
  isRecycled = false;
}

void BitwuzlaIncNativeSolver::distance(const ConstraintQuery &query,
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

class BitwuzlaTreeSolverImpl final : public BitwuzlaSolverImpl {
private:
  using solvers_ty = std::vector<std::unique_ptr<BitwuzlaIncNativeSolver>>;
  using solvers_it = solvers_ty::iterator;

  const size_t maxSolvers;
  std::unique_ptr<BitwuzlaIncNativeSolver> currentSolver = nullptr;
  solvers_ty solvers;

  void findSuitableSolver(const ConstraintQuery &query,
                          ConstraintDistance &delta);
  void setSolver(solvers_it &it, bool recycle = false);
  ConstraintQuery prepare(const Query &q);

public:
  BitwuzlaTreeSolverImpl(size_t maxSolvers) : maxSolvers(maxSolvers){};

  /// implementation of BitwuzlaSolverImpl interface
  Bitwuzla &initNativeBitwuzla(const ConstraintQuery &query,
                               BitwuzlaASTIncSet &assertions) override {
    return currentSolver->getOrInit();
  }
  void deinitNativeBitwuzla(Bitwuzla &theSolver) override {
    assert(currentSolver->isConsistent());
    solvers.push_back(std::move(currentSolver));
  }
  void push(Bitwuzla &s) override { s.push(1); }

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

void BitwuzlaTreeSolverImpl::setSolver(solvers_it &it, bool recycle) {
  assert(it != solvers.end());
  currentSolver = std::move(*it);
  solvers.erase(it);
  currentSolver->isRecycled = false;
  if (recycle)
    currentSolver->clear();
}

void BitwuzlaTreeSolverImpl::findSuitableSolver(const ConstraintQuery &query,
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
            std::make_unique<BitwuzlaIncNativeSolver>(solverParameters);
      else
        setSolver(free_it, /*recycle=*/true);
      return;
    }
  }
  assert(min_it != solvers.end());
  delta = min_delta;
  setSolver(min_it);
}

ConstraintQuery BitwuzlaTreeSolverImpl::prepare(const Query &q) {
  ConstraintDistance delta;
  ConstraintQuery query(q, true);
  findSuitableSolver(query, delta);
  assert(currentSolver->isConsistent());
  currentSolver->stateID = q.id;
  currentSolver->popPush(delta);
  return delta.toPush;
}

bool BitwuzlaTreeSolverImpl::computeTruth(const Query &query, bool &isValid) {
  auto q = prepare(query);
  return BitwuzlaSolverImpl::computeTruth(q, currentSolver->env, isValid);
}

bool BitwuzlaTreeSolverImpl::computeValue(const Query &query,
                                          ref<Expr> &result) {
  auto q = prepare(query);
  return BitwuzlaSolverImpl::computeValue(q, currentSolver->env, result);
}

bool BitwuzlaTreeSolverImpl::computeInitialValues(
    const Query &query, const std::vector<const Array *> &objects,
    std::vector<SparseStorage<unsigned char>> &values, bool &hasSolution) {
  auto q = prepare(query);
  currentSolver->env.objectsForGetModel = objects;
  return BitwuzlaSolverImpl::computeInitialValues(q, currentSolver->env, values,
                                                  hasSolution);
}

bool BitwuzlaTreeSolverImpl::check(const Query &query,
                                   ref<SolverResponse> &result) {
  auto q = prepare(query);
  return BitwuzlaSolverImpl::check(q, currentSolver->env, result);
}

bool BitwuzlaTreeSolverImpl::computeValidityCore(const Query &query,
                                                 ValidityCore &validityCore,
                                                 bool &isValid) {
  auto q = prepare(query);
  return BitwuzlaSolverImpl::computeValidityCore(q, currentSolver->env,
                                                 validityCore, isValid);
}

void BitwuzlaTreeSolverImpl::notifyStateTermination(std::uint32_t id) {
  for (auto &s : solvers)
    if (s->stateID == id)
      s->isRecycled = true;
}

BitwuzlaTreeSolver::BitwuzlaTreeSolver(unsigned maxSolvers)
    : Solver(std::make_unique<BitwuzlaTreeSolverImpl>(maxSolvers)) {}

} // namespace klee
#endif // ENABLE_BITWUZLA
