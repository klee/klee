
#include "klee/Solver.h"
#include "klee/SolverImpl.h"

#include "klee/Constraints.h"
#include "klee/CommandLine.h"
#include "klee/Expr.h"

#include "klee/util/Assignment.h"
#include "klee/util/ExprUtil.h"


#include "STPSolverImpl.h"
#include "SharedMem.h"

#include <errno.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "llvm/Support/ErrorHandling.h"

using namespace klee;

static void stp_error_handler(const char* err_msg) {
  fprintf(stderr, "error: STP Error: %s\n", err_msg);
  abort();
}

STPSolverImpl::STPSolverImpl(bool _useForkedSTP, bool _optimizeDivides)
  : vc(vc_createValidityChecker()),
    builder(new STPBuilder(vc, _optimizeDivides)),
    timeout(0.0),
    useForkedSTP(_useForkedSTP),
    runStatusCode(SOLVER_RUN_STATUS_FAILURE)
{
  assert(vc && "unable to create validity checker");
  assert(builder && "unable to create STPBuilder");

  // In newer versions of STP, a memory management mechanism has been
  // introduced that automatically invalidates certain C interface
  // pointers at vc_Destroy time.  This caused double-free errors
  // due to the ExprHandle destructor also attempting to invalidate
  // the pointers using vc_DeleteExpr.  By setting EXPRDELETE to 0
  // we restore the old behaviour.
  vc_setInterfaceFlags(vc, EXPRDELETE, 0);

  vc_registerErrorHandler(::stp_error_handler);

  if (useForkedSTP) {
    assert(shared_memory_id == 0 && "shared memory id already allocated");
    shared_memory_id = shmget(IPC_PRIVATE, shared_memory_size, IPC_CREAT | 0700);
    if (shared_memory_id < 0)
      llvm::report_fatal_error("unable to allocate shared memory region");
    shared_memory_ptr = (unsigned char*) shmat(shared_memory_id, NULL, 0);
    if (shared_memory_ptr == (void*)-1)
      llvm::report_fatal_error("unable to attach shared memory region");
    shmctl(shared_memory_id, IPC_RMID, NULL);
  }
}

STPSolverImpl::~STPSolverImpl() {
  // Detach the memory region.
  shmdt(shared_memory_ptr);
  shared_memory_ptr = 0;
  shared_memory_id = 0;

  delete builder;

  vc_Destroy(vc);
}

STPSolver::STPSolver(bool useForkedSTP, bool optimizeDivides)
  : Solver(new STPSolverImpl(useForkedSTP, optimizeDivides))
{
}

char *STPSolver::getConstraintLog(const Query &query) {
  return impl->getConstraintLog(query);  
}

void STPSolver::setCoreSolverTimeout(double timeout) {
    impl->setCoreSolverTimeout(timeout);
}

char *STPSolverImpl::getConstraintLog(const Query &query) {
  vc_push(vc);
  for (std::vector< ref<Expr> >::const_iterator it = query.constraints.begin(), 
         ie = query.constraints.end(); it != ie; ++it)
    vc_assertFormula(vc, builder->construct(*it));
  assert(query.expr == ConstantExpr::alloc(0, Expr::Bool) &&
         "Unexpected expression in query!");

  char *buffer;
  unsigned long length;
  vc_printQueryStateToBuffer(vc, builder->getFalse(), 
                             &buffer, &length, false);
  vc_pop(vc);

  return buffer;
}

bool STPSolverImpl::computeTruth(const Query& query,
                                 bool &isValid) {
  std::vector<const Array*> objects;
  std::vector< std::vector<unsigned char> > values;
  bool hasSolution;
  
  if (!computeInitialValues(query, objects, values, hasSolution))
    return false;

  isValid = !hasSolution;
  return true;
}

bool STPSolverImpl::computeValue(const Query& query,
                                 ref<Expr> &result) {
  std::vector<const Array*> objects;
  std::vector< std::vector<unsigned char> > values;
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



static SolverImpl::SolverRunStatus runSTPAndGetCex(::VC vc, STPBuilder *builder, ::VCExpr q,
                                                const std::vector<const Array*> &objects,
                                                std::vector< std::vector<unsigned char> > &values,
                                                bool &hasSolution) {
  // XXX I want to be able to timeout here, safely
  hasSolution = !vc_query(vc, q);

  if (hasSolution) {
    values.reserve(objects.size());
    for (std::vector<const Array*>::const_iterator
           it = objects.begin(), ie = objects.end(); it != ie; ++it) {
      const Array *array = *it;
      std::vector<unsigned char> data;
      
      data.reserve(array->size);
      for (unsigned offset = 0; offset < array->size; offset++) {
        ExprHandle counter = 
          vc_getCounterExample(vc, builder->getInitialRead(array, offset));
        unsigned char val = getBVUnsigned(counter);
        data.push_back(val);
      }
      
      values.push_back(data);
    }
  }
  
  if (true == hasSolution) {
    return SolverImpl::SOLVER_RUN_STATUS_SUCCESS_SOLVABLE;
  }
  else {
    return SolverImpl::SOLVER_RUN_STATUS_SUCCESS_UNSOLVABLE;  
  }
}

static void stpTimeoutHandler(int x) {
  _exit(52);
}

static SolverImpl::SolverRunStatus runSTPAndGetCexForked(::VC vc, 
                                                      STPBuilder *builder,
                                                      ::VCExpr q,
                                                      const std::vector<const Array*> &objects,
                                                      std::vector< std::vector<unsigned char> >
                                                      &values,
                                                      bool &hasSolution,
                                                      double timeout) {
  unsigned char *pos = shared_memory_ptr;
  unsigned sum = 0;
  for (std::vector<const Array*>::const_iterator
         it = objects.begin(), ie = objects.end(); it != ie; ++it)
    sum += (*it)->size;
  if (sum >= shared_memory_size)
    llvm::report_fatal_error("not enough shared memory for counterexample");

  fflush(stdout);
  fflush(stderr);
  int pid = fork();
  if (pid==-1) {
    fprintf(stderr, "ERROR: fork failed (for STP)");
    if (!IgnoreSolverFailures) 
      exit(1);
    return SolverImpl::SOLVER_RUN_STATUS_FORK_FAILED;
  }

  if (pid == 0) {
    if (timeout) {      
      ::alarm(0); /* Turn off alarm so we can safely set signal handler */
      ::signal(SIGALRM, stpTimeoutHandler);
      ::alarm(std::max(1, (int)timeout));
    } 
   
    unsigned res = vc_query(vc, q);
    if (!res) {
      for (std::vector<const Array*>::const_iterator
             it = objects.begin(), ie = objects.end(); it != ie; ++it) {
        const Array *array = *it;
        for (unsigned offset = 0; offset < array->size; offset++) {
          ExprHandle counter = 
            vc_getCounterExample(vc, builder->getInitialRead(array, offset));
          *pos++ = getBVUnsigned(counter);
        }
      }
    }
    _exit(res);

  } else {
    int status;
    pid_t res;

    do {
      res = waitpid(pid, &status, 0);
    } while (res < 0 && errno == EINTR);
    
    if (res < 0) {
      fprintf(stderr, "ERROR: waitpid() for STP failed");
      if (!IgnoreSolverFailures) 
	exit(1);
      return SolverImpl::SOLVER_RUN_STATUS_WAITPID_FAILED;
    }
    
    // From timed_run.py: It appears that linux at least will on
    // "occasion" return a status when the process was terminated by a
    // signal, so test signal first.
    if (WIFSIGNALED(status) || !WIFEXITED(status)) {
      fprintf(stderr, "ERROR: STP did not return successfully.  Most likely you forgot to run 'ulimit -s unlimited'\n");
      if (!IgnoreSolverFailures)  {
	exit(1);
      }
      return SolverImpl::SOLVER_RUN_STATUS_INTERRUPTED;
    }

    int exitcode = WEXITSTATUS(status);
    if (exitcode==0) {
      hasSolution = true;
    } else if (exitcode==1) {
      hasSolution = false;
    } else if (exitcode==52) {
      fprintf(stderr, "error: STP timed out");
      // mark that a timeout occurred
      return SolverImpl::SOLVER_RUN_STATUS_TIMEOUT;
    } else {
      fprintf(stderr, "error: STP did not return a recognized code");
      if (!IgnoreSolverFailures) 
	exit(1);
      return SolverImpl::SOLVER_RUN_STATUS_UNEXPECTED_EXIT_CODE;
    }
    
    if (hasSolution) {
      values = std::vector< std::vector<unsigned char> >(objects.size());
      unsigned i=0;
      for (std::vector<const Array*>::const_iterator
             it = objects.begin(), ie = objects.end(); it != ie; ++it) {
        const Array *array = *it;
        std::vector<unsigned char> &data = values[i++];
        data.insert(data.begin(), pos, pos + array->size);
        pos += array->size;
      }
    }
    
    if (true == hasSolution) {
      return SolverImpl::SOLVER_RUN_STATUS_SUCCESS_SOLVABLE;
    }
    else {        
      return SolverImpl::SOLVER_RUN_STATUS_SUCCESS_UNSOLVABLE;
    }
  }
}

bool
STPSolverImpl::computeInitialValues(const Query &query,
                                    const std::vector<const Array*> 
                                      &objects,
                                    std::vector< std::vector<unsigned char> > 
                                      &values,
                                    bool &hasSolution) {
  runStatusCode =  SOLVER_RUN_STATUS_FAILURE; 

  TimerStatIncrementer t(stats::queryTime);

  vc_push(vc);

  for (ConstraintManager::const_iterator it = query.constraints.begin(), 
         ie = query.constraints.end(); it != ie; ++it)
    vc_assertFormula(vc, builder->construct(*it));
  
  ++stats::queries;
  ++stats::queryCounterexamples;

  ExprHandle stp_e = builder->construct(query.expr);
     
  if (0) {
    char *buf;
    unsigned long len;
    vc_printQueryStateToBuffer(vc, stp_e, &buf, &len, false);
    fprintf(stderr, "note: STP query: %.*s\n", (unsigned) len, buf);
  }

  bool success;
  if (useForkedSTP) {
    runStatusCode = runSTPAndGetCexForked(vc, builder, stp_e, objects, values, 
                                       hasSolution, timeout);
    success = ((SOLVER_RUN_STATUS_SUCCESS_SOLVABLE == runStatusCode) ||
               (SOLVER_RUN_STATUS_SUCCESS_UNSOLVABLE == runStatusCode));    
  } else {
    runStatusCode = runSTPAndGetCex(vc, builder, stp_e, objects, values, hasSolution);    
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

