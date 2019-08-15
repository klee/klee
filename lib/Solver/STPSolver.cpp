//===-- STPSolver.cpp -----------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#include "klee/Config/config.h"

#ifdef ENABLE_STP

#include "STPBuilder.h"
#include "STPSolver.h"

#include "klee/Expr/Assignment.h"
#include "klee/Expr/Constraints.h"
#include "klee/Expr/ExprUtil.h"
#include "klee/Internal/Support/ErrorHandling.h"
#include "klee/OptionCategories.h"
#include "klee/Solver/SolverImpl.h"

#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Errno.h"

#include <csignal>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <unistd.h>

namespace {

llvm::cl::opt<bool> DebugDumpSTPQueries(
    "debug-dump-stp-queries", llvm::cl::init(false),
    llvm::cl::desc("Dump every STP query to stderr (default=false)"),
    llvm::cl::cat(klee::SolvingCat));

llvm::cl::opt<bool> IgnoreSolverFailures(
    "ignore-solver-failures", llvm::cl::init(false),
    llvm::cl::desc("Ignore any STP solver failures (default=false)"),
    llvm::cl::cat(klee::SolvingCat));
}

#define vc_bvBoolExtract IAMTHESPAWNOFSATAN

static unsigned char *shared_memory_ptr = nullptr;
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

static void stp_error_handler(const char *err_msg) {
  fprintf(stderr, "error: STP Error: %s\n", err_msg);
  abort();
}

namespace klee {

class STPSolverImpl : public SolverImpl {
private:
  VC vc;
  STPBuilder *builder;
  time::Span timeout;
  bool useForkedSTP;
  SolverRunStatus runStatusCode;

public:
  explicit STPSolverImpl(bool useForkedSTP, bool optimizeDivides = true);
  ~STPSolverImpl() override;

  char *getConstraintLog(const Query &) override;
  void setCoreSolverTimeout(time::Span timeout) override { this->timeout = timeout; }

  bool computeTruth(const Query &, bool &isValid) override;
  bool computeValue(const Query &, ref<Expr> &result) override;
  bool computeInitialValues(const Query &,
                            const std::vector<const Array *> &objects,
                            std::vector<std::vector<unsigned char>> &values,
                            bool &hasSolution) override;
  SolverRunStatus getOperationStatusCode() override;
};

STPSolverImpl::STPSolverImpl(bool useForkedSTP, bool optimizeDivides)
    : vc(vc_createValidityChecker()),
      builder(new STPBuilder(vc, optimizeDivides)),
      useForkedSTP(useForkedSTP), runStatusCode(SOLVER_RUN_STATUS_FAILURE) {
  assert(vc && "unable to create validity checker");
  assert(builder && "unable to create STPBuilder");

  // In newer versions of STP, a memory management mechanism has been
  // introduced that automatically invalidates certain C interface
  // pointers at vc_Destroy time.  This caused double-free errors
  // due to the ExprHandle destructor also attempting to invalidate
  // the pointers using vc_DeleteExpr.  By setting EXPRDELETE to 0
  // we restore the old behaviour.
  vc_setInterfaceFlags(vc, EXPRDELETE, 0);

  make_division_total(vc);

  vc_registerErrorHandler(::stp_error_handler);

  if (useForkedSTP) {
    assert(shared_memory_id == 0 && "shared memory id already allocated");
    shared_memory_id =
        shmget(IPC_PRIVATE, shared_memory_size, IPC_CREAT | 0700);
    if (shared_memory_id < 0)
      llvm::report_fatal_error("unable to allocate shared memory region");
    shared_memory_ptr = (unsigned char *)shmat(shared_memory_id, nullptr, 0);
    if (shared_memory_ptr == (void *)-1)
      llvm::report_fatal_error("unable to attach shared memory region");
    shmctl(shared_memory_id, IPC_RMID, nullptr);
  }
}

STPSolverImpl::~STPSolverImpl() {
  // Detach the memory region.
  shmdt(shared_memory_ptr);
  shared_memory_ptr = nullptr;
  shared_memory_id = 0;

  delete builder;

  vc_Destroy(vc);
}

/***/

char *STPSolverImpl::getConstraintLog(const Query &query) {
  vc_push(vc);

  for (const auto &constraint : query.constraints)
    vc_assertFormula(vc, builder->construct(constraint));
  assert(query.expr == ConstantExpr::alloc(0, Expr::Bool) &&
         "Unexpected expression in query!");

  char *buffer;
  unsigned long length;
  vc_printQueryStateToBuffer(vc, builder->getFalse(), &buffer, &length, false);
  vc_pop(vc);

  return buffer;
}

bool STPSolverImpl::computeTruth(const Query &query, bool &isValid) {
  std::vector<const Array *> objects;
  std::vector<std::vector<unsigned char>> values;
  bool hasSolution;

  if (!computeInitialValues(query, objects, values, hasSolution))
    return false;

  isValid = !hasSolution;
  return true;
}

bool STPSolverImpl::computeValue(const Query &query, ref<Expr> &result) {
  std::vector<const Array *> objects;
  std::vector<std::vector<unsigned char>> values;
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

static SolverImpl::SolverRunStatus
runAndGetCex(::VC vc, STPBuilder *builder, ::VCExpr q,
             const std::vector<const Array *> &objects,
             std::vector<std::vector<unsigned char>> &values,
             bool &hasSolution) {
  // XXX I want to be able to timeout here, safely
  hasSolution = !vc_query(vc, q);

  if (!hasSolution)
    return SolverImpl::SOLVER_RUN_STATUS_SUCCESS_UNSOLVABLE;

  values.reserve(objects.size());
  unsigned i = 0; // FIXME C++17: use reference from emplace_back()
  for (const auto object : objects) {
    values.emplace_back(object->size);

    for (unsigned offset = 0; offset < object->size; offset++) {
      ExprHandle counter =
          vc_getCounterExample(vc, builder->getInitialRead(object, offset));
      values[i][offset] = static_cast<unsigned char>(getBVUnsigned(counter));
    }
    ++i;
  }

  return SolverImpl::SOLVER_RUN_STATUS_SUCCESS_SOLVABLE;
}

static void stpTimeoutHandler(int x) { _exit(52); }

static SolverImpl::SolverRunStatus
runAndGetCexForked(::VC vc, STPBuilder *builder, ::VCExpr q,
                   const std::vector<const Array *> &objects,
                   std::vector<std::vector<unsigned char>> &values,
                   bool &hasSolution, time::Span timeout) {
  unsigned char *pos = shared_memory_ptr;
  unsigned sum = 0;
  for (const auto object : objects)
    sum += object->size;
  if (sum >= shared_memory_size)
    llvm::report_fatal_error("not enough shared memory for counterexample");

  fflush(stdout);
  fflush(stderr);

  // fork solver
  int pid = fork();
  // - error
  if (pid == -1) {
    klee_warning("fork failed (for STP) - %s", llvm::sys::StrError(errno).c_str());
    if (!IgnoreSolverFailures)
      exit(1);
    return SolverImpl::SOLVER_RUN_STATUS_FORK_FAILED;
  }
  // - child (solver)
  if (pid == 0) {
    if (timeout) {
      ::alarm(0); /* Turn off alarm so we can safely set signal handler */
      ::signal(SIGALRM, stpTimeoutHandler);
      ::alarm(std::max(1u, static_cast<unsigned>(timeout.toSeconds())));
    }
    int res = vc_query(vc, q);
    if (!res) {
      for (const auto object : objects) {
        for (unsigned offset = 0; offset < object->size; offset++) {
          ExprHandle counter =
              vc_getCounterExample(vc, builder->getInitialRead(object, offset));
          *pos++ = static_cast<unsigned char>(getBVUnsigned(counter));
        }
      }
    }
    _exit(res);
  // - parent
  } else {
    int status;
    pid_t res;

    do {
      res = waitpid(pid, &status, 0);
    } while (res < 0 && errno == EINTR);

    if (res < 0) {
      klee_warning("waitpid() for STP failed");
      if (!IgnoreSolverFailures)
        exit(1);
      return SolverImpl::SOLVER_RUN_STATUS_WAITPID_FAILED;
    }

    // From timed_run.py: It appears that linux at least will on
    // "occasion" return a status when the process was terminated by a
    // signal, so test signal first.
    if (WIFSIGNALED(status) || !WIFEXITED(status)) {
      klee_warning("STP did not return successfully.  Most likely you forgot "
                   "to run 'ulimit -s unlimited'");
      if (!IgnoreSolverFailures) {
        exit(1);
      }
      return SolverImpl::SOLVER_RUN_STATUS_INTERRUPTED;
    }

    int exitcode = WEXITSTATUS(status);

    // solvable
    if (exitcode == 0) {
      hasSolution = true;

      values.reserve(objects.size());
      for (const auto object : objects) {
        values.emplace_back(pos, pos + object->size);
        pos += object->size;
      }

      return SolverImpl::SOLVER_RUN_STATUS_SUCCESS_SOLVABLE;
    }

    // unsolvable
    if (exitcode == 1) {
      hasSolution = false;
      return SolverImpl::SOLVER_RUN_STATUS_SUCCESS_UNSOLVABLE;
    }

    // timeout
    if (exitcode == 52) {
      klee_warning("STP timed out");
      // mark that a timeout occurred
      return SolverImpl::SOLVER_RUN_STATUS_TIMEOUT;
    }

    // unknown return code
    klee_warning("STP did not return a recognized code");
    if (!IgnoreSolverFailures)
      exit(1);
    return SolverImpl::SOLVER_RUN_STATUS_UNEXPECTED_EXIT_CODE;
  }
}

bool STPSolverImpl::computeInitialValues(
    const Query &query, const std::vector<const Array *> &objects,
    std::vector<std::vector<unsigned char>> &values, bool &hasSolution) {
  runStatusCode = SOLVER_RUN_STATUS_FAILURE;
  TimerStatIncrementer t(stats::queryTime);

  vc_push(vc);

  for (const auto &constraint : query.constraints)
    vc_assertFormula(vc, builder->construct(constraint));

  ++stats::queries;
  ++stats::queryCounterexamples;

  ExprHandle stp_e = builder->construct(query.expr);

  if (DebugDumpSTPQueries) {
    char *buf;
    unsigned long len;
    vc_printQueryStateToBuffer(vc, stp_e, &buf, &len, false);
    klee_warning("STP query:\n%.*s\n", (unsigned)len, buf);
    free(buf);
  }

  bool success;
  if (useForkedSTP) {
    runStatusCode = runAndGetCexForked(vc, builder, stp_e, objects, values,
                                       hasSolution, timeout);
    success = ((SOLVER_RUN_STATUS_SUCCESS_SOLVABLE == runStatusCode) ||
               (SOLVER_RUN_STATUS_SUCCESS_UNSOLVABLE == runStatusCode));
  } else {
    runStatusCode =
        runAndGetCex(vc, builder, stp_e, objects, values, hasSolution);
    success = true;
  }

  if (success) {
    if (hasSolution)
      ++stats::queriesInvalid;
    else
      ++stats::queriesValid;
  }

  vc_pop(vc);

  return success;
}

SolverImpl::SolverRunStatus STPSolverImpl::getOperationStatusCode() {
  return runStatusCode;
}

STPSolver::STPSolver(bool useForkedSTP, bool optimizeDivides)
    : Solver(new STPSolverImpl(useForkedSTP, optimizeDivides)) {}

char *STPSolver::getConstraintLog(const Query &query) {
  return impl->getConstraintLog(query);
}

void STPSolver::setCoreSolverTimeout(time::Span timeout) {
  impl->setCoreSolverTimeout(timeout);
}

} // klee
#endif // ENABLE_STP
