//===-- MetaSMTSolver.cpp -------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#include "klee/Config/config.h"

#ifdef ENABLE_METASMT
#include "MetaSMTSolver.h"
#include "MetaSMTBuilder.h"

#include "klee/Expr/Assignment.h"
#include "klee/Expr/Constraints.h"
#include "klee/Expr/ExprUtil.h"
#include "klee/Internal/Support/ErrorHandling.h"
#include "klee/Solver/Solver.h"
#include "klee/Solver/SolverImpl.h"

#include "llvm/Support/ErrorHandling.h"

#include <metaSMT/DirectSolver_Context.hpp>

#ifdef METASMT_HAVE_Z3
#include <metaSMT/backend/Z3_Backend.hpp>
#endif

#ifdef METASMT_HAVE_BTOR
#include <metaSMT/backend/Boolector.hpp>
#endif

#ifdef METASMT_HAVE_CVC4
#include <metaSMT/backend/CVC4.hpp>
#endif

#ifdef METASMT_HAVE_YICES2
#include <metaSMT/backend/Yices2.hpp>
#endif

#ifdef METASMT_HAVE_STP
#define Expr VCExpr
#define Type VCType
#define STP STP_Backend
#define type_t STP_type_t
#include <metaSMT/backend/STP.hpp>
#undef Expr
#undef Type
#undef STP
#undef type_t
#endif

#include <errno.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <unistd.h>

static unsigned char *shared_memory_ptr;
static int shared_memory_id = 0;
// Darwin by default has a very small limit on the maximum amount of shared
// memory, which will quickly be exhausted by KLEE running its tests in
// parallel. For now, we work around this by just requesting a smaller size --
// in practice users hitting this limit on counterexample sizes probably already
// are hitting more serious scalability issues.
#ifdef __APPLE__
static const unsigned shared_memory_size = 1 << 16;
#else
static const unsigned shared_memory_size = 1 << 20;
#endif

namespace klee {

template <typename SolverContext> class MetaSMTSolverImpl : public SolverImpl {
private:
  SolverContext _meta_solver;
  MetaSMTSolver<SolverContext> *_solver;
  MetaSMTBuilder<SolverContext> *_builder;
  time::Span _timeout;
  bool _useForked;
  SolverRunStatus _runStatusCode;

public:
  MetaSMTSolverImpl(MetaSMTSolver<SolverContext> *solver, bool useForked,
                    bool optimizeDivides);
  virtual ~MetaSMTSolverImpl();

  char *getConstraintLog(const Query &);
  void setCoreSolverTimeout(time::Span timeout) { _timeout = timeout; }

  bool computeTruth(const Query &, bool &isValid);
  bool computeValue(const Query &, ref<Expr> &result);

  bool computeInitialValues(const Query &query,
                            const std::vector<const Array *> &objects,
                            std::vector<std::vector<unsigned char> > &values,
                            bool &hasSolution);

  SolverImpl::SolverRunStatus
  runAndGetCex(const Query &query, const std::vector<const Array *> &objects,
               std::vector<std::vector<unsigned char> > &values,
               bool &hasSolution);

  SolverImpl::SolverRunStatus
  runAndGetCexForked(const Query &query,
                     const std::vector<const Array *> &objects,
                     std::vector<std::vector<unsigned char> > &values,
                     bool &hasSolution, time::Span timeout);

  SolverRunStatus getOperationStatusCode();

  SolverContext &get_meta_solver() { return _meta_solver; };
};

template <typename SolverContext>
MetaSMTSolverImpl<SolverContext>::MetaSMTSolverImpl(
    MetaSMTSolver<SolverContext> *solver, bool useForked, bool optimizeDivides)
    : _solver(solver), _builder(new MetaSMTBuilder<SolverContext>(
                           _meta_solver, optimizeDivides)),
      _useForked(useForked) {
  assert(_solver && "unable to create MetaSMTSolver");
  assert(_builder && "unable to create MetaSMTBuilder");

  if (_useForked) {
    shared_memory_id =
        shmget(IPC_PRIVATE, shared_memory_size, IPC_CREAT | 0700);
    assert(shared_memory_id >= 0 && "shmget failed");
    shared_memory_ptr = (unsigned char *)shmat(shared_memory_id, NULL, 0);
    assert(shared_memory_ptr != (void *)-1 && "shmat failed");
    shmctl(shared_memory_id, IPC_RMID, NULL);
  }
}

template <typename SolverContext>
MetaSMTSolverImpl<SolverContext>::~MetaSMTSolverImpl() {}

template <typename SolverContext>
char *MetaSMTSolverImpl<SolverContext>::getConstraintLog(const Query &) {
  const char *msg = "Not supported";
  char *buf = new char[strlen(msg) + 1];
  strcpy(buf, msg);
  return buf;
}

template <typename SolverContext>
bool MetaSMTSolverImpl<SolverContext>::computeTruth(const Query &query,
                                                    bool &isValid) {

  bool success = false;
  std::vector<const Array *> objects;
  std::vector<std::vector<unsigned char> > values;
  bool hasSolution;

  if (computeInitialValues(query, objects, values, hasSolution)) {
    // query.expr is valid iff !query.expr is not satisfiable
    isValid = !hasSolution;
    success = true;
  }

  return success;
}

template <typename SolverContext>
bool MetaSMTSolverImpl<SolverContext>::computeValue(const Query &query,
                                                    ref<Expr> &result) {

  bool success = false;
  std::vector<const Array *> objects;
  std::vector<std::vector<unsigned char> > values;
  bool hasSolution;

  // Find the object used in the expression, and compute an assignment for them.
  findSymbolicObjects(query.expr, objects);
  if (computeInitialValues(query.withFalse(), objects, values, hasSolution)) {
    assert(hasSolution && "state has invalid constraint set");
    // Evaluate the expression with the computed assignment.
    Assignment a(objects, values);
    result = a.evaluate(query.expr);
    success = true;
  }

  return success;
}

template <typename SolverContext>
bool MetaSMTSolverImpl<SolverContext>::computeInitialValues(
    const Query &query, const std::vector<const Array *> &objects,
    std::vector<std::vector<unsigned char> > &values, bool &hasSolution) {

  _runStatusCode = SOLVER_RUN_STATUS_FAILURE;

  TimerStatIncrementer t(stats::queryTime);
  assert(_builder);

  ++stats::queries;
  ++stats::queryCounterexamples;

  bool success = true;
  if (_useForked) {
    _runStatusCode =
        runAndGetCexForked(query, objects, values, hasSolution, _timeout);
    success = ((SOLVER_RUN_STATUS_SUCCESS_SOLVABLE == _runStatusCode) ||
               (SOLVER_RUN_STATUS_SUCCESS_UNSOLVABLE == _runStatusCode));
  } else {
    _runStatusCode = runAndGetCex(query, objects, values, hasSolution);
    success = true;
  }

  if (success) {
    if (hasSolution) {
      ++stats::queriesInvalid;
    } else {
      ++stats::queriesValid;
    }
  }

  return success;
}

template <typename SolverContext>
SolverImpl::SolverRunStatus MetaSMTSolverImpl<SolverContext>::runAndGetCex(
    const Query &query, const std::vector<const Array *> &objects,
    std::vector<std::vector<unsigned char> > &values, bool &hasSolution) {

  // assume the constraints of the query
  for (ConstraintManager::const_iterator it = query.constraints.begin(),
                                         ie = query.constraints.end();
       it != ie; ++it) {
    assumption(_meta_solver, _builder->construct(*it));
  }
  // assume the negation of the query
  assumption(_meta_solver, _builder->construct(Expr::createIsZero(query.expr)));
  hasSolution = solve(_meta_solver);

  if (hasSolution) {
    values.reserve(objects.size());
    for (std::vector<const Array *>::const_iterator it = objects.begin(),
                                                    ie = objects.end();
         it != ie; ++it) {

      const Array *array = *it;
      assert(array);
      typename SolverContext::result_type array_exp =
          _builder->getInitialArray(array);

      std::vector<unsigned char> data;
      data.reserve(array->size);

      for (unsigned offset = 0; offset < array->size; offset++) {
        typename SolverContext::result_type elem_exp = evaluate(
            _meta_solver, metaSMT::logic::Array::select(
                              array_exp, bvuint(offset, array->getDomain())));
        unsigned char elem_value = metaSMT::read_value(_meta_solver, elem_exp);
        data.push_back(elem_value);
      }

      values.push_back(data);
    }
  }

  if (true == hasSolution) {
    return SolverImpl::SOLVER_RUN_STATUS_SUCCESS_SOLVABLE;
  } else {
    return SolverImpl::SOLVER_RUN_STATUS_SUCCESS_UNSOLVABLE;
  }
}

static void metaSMTTimeoutHandler(int x) { _exit(52); }

template <typename SolverContext>
SolverImpl::SolverRunStatus
MetaSMTSolverImpl<SolverContext>::runAndGetCexForked(
    const Query &query, const std::vector<const Array *> &objects,
    std::vector<std::vector<unsigned char> > &values, bool &hasSolution,
    time::Span timeout) {
  unsigned char *pos = shared_memory_ptr;
  unsigned sum = 0;
  for (std::vector<const Array *>::const_iterator it = objects.begin(),
                                                  ie = objects.end();
       it != ie; ++it) {
    sum += (*it)->size;
  }
  // sum += sizeof(uint64_t);
  sum += sizeof(stats::queryConstructs);
  assert(sum < shared_memory_size &&
         "not enough shared memory for counterexample");

  fflush(stdout);
  fflush(stderr);
  int pid = fork();
  if (pid == -1) {
    klee_warning("fork failed (for metaSMT)");
    return SolverImpl::SOLVER_RUN_STATUS_FORK_FAILED;
  }

  if (pid == 0) {
    if (timeout) {
      ::alarm(0); /* Turn off alarm so we can safely set signal handler */
      ::signal(SIGALRM, metaSMTTimeoutHandler);
      ::alarm(std::max(1u, static_cast<unsigned>(timeout.toSeconds())));
    }

    // assert constraints as we are in a child process
    for (ConstraintManager::const_iterator it = query.constraints.begin(),
                                           ie = query.constraints.end();
         it != ie; ++it) {
      assertion(_meta_solver, _builder->construct(*it));
      // assumption(_meta_solver, _builder->construct(*it));
    }

    // asssert the negation of the query as we are in a child process
    assertion(_meta_solver,
              _builder->construct(Expr::createIsZero(query.expr)));
    unsigned res = solve(_meta_solver);

    if (res) {
      for (std::vector<const Array *>::const_iterator it = objects.begin(),
                                                      ie = objects.end();
           it != ie; ++it) {

        const Array *array = *it;
        assert(array);
        typename SolverContext::result_type array_exp =
            _builder->getInitialArray(array);

        for (unsigned offset = 0; offset < array->size; offset++) {

          typename SolverContext::result_type elem_exp = evaluate(
              _meta_solver, metaSMT::logic::Array::select(
                                array_exp, bvuint(offset, array->getDomain())));
          unsigned char elem_value =
              metaSMT::read_value(_meta_solver, elem_exp);
          *pos++ = elem_value;
        }
      }
    }
    assert((uint64_t *)pos);
    *((uint64_t *)pos) = stats::queryConstructs;

    _exit(!res);
  } else {
    int status;
    pid_t res;

    do {
      res = waitpid(pid, &status, 0);
    } while (res < 0 && errno == EINTR);

    if (res < 0) {
      klee_warning("waitpid() for metaSMT failed");
      return SolverImpl::SOLVER_RUN_STATUS_WAITPID_FAILED;
    }

    // From timed_run.py: It appears that linux at least will on
    // "occasion" return a status when the process was terminated by a
    // signal, so test signal first.
    if (WIFSIGNALED(status) || !WIFEXITED(status)) {
      klee_warning(
          "error: metaSMT did not return successfully (status = %d) \n",
          WTERMSIG(status));
      return SolverImpl::SOLVER_RUN_STATUS_INTERRUPTED;
    }

    int exitcode = WEXITSTATUS(status);
    if (exitcode == 0) {
      hasSolution = true;
    } else if (exitcode == 1) {
      hasSolution = false;
    } else if (exitcode == 52) {
      klee_warning("metaSMT timed out");
      return SolverImpl::SOLVER_RUN_STATUS_TIMEOUT;
    } else {
      klee_warning("metaSMT did not return a recognized code");
      return SolverImpl::SOLVER_RUN_STATUS_UNEXPECTED_EXIT_CODE;
    }

    if (hasSolution) {
      values = std::vector<std::vector<unsigned char> >(objects.size());
      unsigned i = 0;
      for (std::vector<const Array *>::const_iterator it = objects.begin(),
                                                      ie = objects.end();
           it != ie; ++it) {
        const Array *array = *it;
        assert(array);
        std::vector<unsigned char> &data = values[i++];
        data.insert(data.begin(), pos, pos + array->size);
        pos += array->size;
      }
    }
    stats::queryConstructs += (*((uint64_t *)pos) - stats::queryConstructs);

    if (true == hasSolution) {
      return SolverImpl::SOLVER_RUN_STATUS_SUCCESS_SOLVABLE;
    } else {
      return SolverImpl::SOLVER_RUN_STATUS_SUCCESS_UNSOLVABLE;
    }
  }
}

template <typename SolverContext>
SolverImpl::SolverRunStatus
MetaSMTSolverImpl<SolverContext>::getOperationStatusCode() {
  return _runStatusCode;
}

template <typename SolverContext>
MetaSMTSolver<SolverContext>::MetaSMTSolver(bool useForked,
                                            bool optimizeDivides)
    : Solver(new MetaSMTSolverImpl<SolverContext>(this, useForked,
                                                  optimizeDivides)) {}

template <typename SolverContext>
MetaSMTSolver<SolverContext>::~MetaSMTSolver() {}

template <typename SolverContext>
char *MetaSMTSolver<SolverContext>::getConstraintLog(const Query &query) {
  return impl->getConstraintLog(query);
}

template <typename SolverContext>
void MetaSMTSolver<SolverContext>::setCoreSolverTimeout(time::Span timeout) {
  impl->setCoreSolverTimeout(timeout);
}

Solver *createMetaSMTSolver() {
  using namespace metaSMT;

  Solver *coreSolver = NULL;
  std::string backend;
  switch (MetaSMTBackend) {
#ifdef METASMT_HAVE_STP
  case METASMT_BACKEND_STP:
    backend = "STP";
    coreSolver = new MetaSMTSolver<DirectSolver_Context<solver::STP_Backend> >(
        UseForkedCoreSolver, CoreSolverOptimizeDivides);
    break;
#endif
#ifdef METASMT_HAVE_Z3
  case METASMT_BACKEND_Z3:
    backend = "Z3";
    coreSolver = new MetaSMTSolver<DirectSolver_Context<solver::Z3_Backend> >(
        UseForkedCoreSolver, CoreSolverOptimizeDivides);
    break;
#endif
#ifdef METASMT_HAVE_BTOR
  case METASMT_BACKEND_BOOLECTOR:
    backend = "Boolector";
    coreSolver = new MetaSMTSolver<DirectSolver_Context<solver::Boolector> >(
        UseForkedCoreSolver, CoreSolverOptimizeDivides);
    break;
#endif
#ifdef METASMT_HAVE_CVC4
  case METASMT_BACKEND_CVC4:
    backend = "CVC4";
    coreSolver = new MetaSMTSolver<DirectSolver_Context<solver::CVC4> >(
        UseForkedCoreSolver, CoreSolverOptimizeDivides);
    break;
#endif
#ifdef METASMT_HAVE_YICES2
  case METASMT_BACKEND_YICES2:
    backend = "Yices2";
    coreSolver = new MetaSMTSolver<DirectSolver_Context<solver::Yices2> >(
        UseForkedCoreSolver, CoreSolverOptimizeDivides);
    break;
#endif
  default:
    llvm_unreachable("Unrecognised MetaSMT backend");
    break;
  };
  klee_message("Starting MetaSMTSolver(%s)", backend.c_str());
  return coreSolver;
}

}
#endif // ENABLE_METASMT
