//===-- YicesSolverImpl.cpp--------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Incorporating Yices into Klee was supported by DARPA under 
// contract N66001-13-C-4057.
//  
// Approved for Public Release, Distribution Unlimited.
//
// Bruno Dutertre & Ian A. Mason @  SRI International.
//===----------------------------------------------------------------------===//



#include "klee/Solver.h"
#include "klee/SolverImpl.h"

#include "klee/Constraints.h"
#include "klee/CommandLine.h"
#include "klee/Expr.h"

#include "klee/util/Assignment.h"
#include "klee/util/ExprUtil.h"

#ifdef SUPPORT_YICES

#include "YicesSolverImpl.h"
#include "SharedMem.h"

#include <cstring>

#include <errno.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "llvm/Support/ErrorHandling.h"

using namespace klee;

/*
static void yices_error_handler(const char* err_msg) {
  fprintf(stderr, "error: Yices Error: %s\n", err_msg);
  abort();
}
*/

YicesSolverImpl::YicesSolverImpl(bool _useForkedYices, bool _optimizeDivides)
  : builder(new YicesBuilder()),
    config(NULL),
    timeout(0.0),
    useForkedYices(_useForkedYices),
    runStatusCode(SOLVER_RUN_STATUS_FAILURE)
{

assert(builder && "unable to create YicesBuilder");

config = yices_new_config();
int32_t ycode = yices_default_config_for_logic(config, "QF_ABV");

assert(ycode == 0 && "unable to create YicesConfig");




  if (useForkedYices) {
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

YicesSolverImpl::~YicesSolverImpl() {
// Detach the memory region.
shmdt(shared_memory_ptr);
shared_memory_ptr = 0;
shared_memory_id = 0;

yices_free_config(config);


delete builder;


}



YicesSolver::YicesSolver(bool useForkedYices, bool optimizeDivides)
  : Solver(new YicesSolverImpl(useForkedYices, optimizeDivides))
{
}

char *YicesSolver::getConstraintLog(const Query &query) {
  return impl->getConstraintLog(query);  
}

void YicesSolver::setCoreSolverTimeout(double timeout) {
    impl->setCoreSolverTimeout(timeout);
}


char *YicesSolverImpl::getConstraintLog(const Query &query) {
  std::string output;
  char *retval = NULL;
  for (std::vector< ref<Expr> >::const_iterator it = query.constraints.begin(), 
         ie = query.constraints.end(); it != ie; ++it){
    ::YicesExpr term = builder->construct(*it);
    char* term_str = yices_term_to_string(term, 120, 64, 0);
    output.append(term_str);
    yices_free_string(term_str);
  }
  retval = strdup(output.c_str());
  return retval;
}

bool YicesSolverImpl::computeTruth(const Query& query,
                                 bool &isValid) {
  std::vector<const Array*> objects;
  std::vector< std::vector<unsigned char> > values;
  bool hasSolution;

  if (!computeInitialValues(query, objects, values, hasSolution))
    return false;

  isValid = !hasSolution;
  return true;
}

bool YicesSolverImpl::computeValue(const Query& query,
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


static SolverImpl::SolverRunStatus runYicesAndGetCex(YicesBuilder *builder, ::YicesContext *ctx,
						      const std::vector<const Array*> &objects,
					              std::vector< std::vector<unsigned char> > &values,
						      bool &hasSolution) {

  smt_status_t ycode = yices_check_context(ctx, NULL);
  SolverImpl::SolverRunStatus retval;
  
  switch(ycode){
  case STATUS_SAT: {
    hasSolution = true;
    retval = SolverImpl::SOLVER_RUN_STATUS_SUCCESS_SOLVABLE;
    values.reserve(objects.size());
    
    ::YicesModel* cex = yices_get_model(ctx, true);
    
    assert(cex != NULL);
    
    
    for (std::vector<const Array*>::const_iterator
	   it = objects.begin(), ie = objects.end(); it != ie; ++it) {
      const Array *array = *it;
      std::vector<unsigned char> data;
      
      data.reserve(array->size);
      for (unsigned offset = 0; offset < array->size; offset++) {
	int32_t buffer[8];
	//a little dangerous
	yices_get_bv_value(cex, builder->getInitialRead(array, offset), buffer);
	unsigned val =  0;
	unsigned j = 8;
	while(j > 0){
	  j--;
	  val <<= 1;
	  if(buffer[j] != 0){
	    val  |= 1;
	  }
	}
	data.push_back((unsigned char)val);
      }
      values.push_back(data);
    }
    
    yices_free_model(cex);
    
    break;
  }
    
  case STATUS_UNSAT:
    hasSolution = false;
    retval = SolverImpl::SOLVER_RUN_STATUS_SUCCESS_UNSOLVABLE; 
    break;
    
  case STATUS_INTERRUPTED: 
    retval = SolverImpl::SOLVER_RUN_STATUS_TIMEOUT;
    break;
    
    
  case STATUS_ERROR:
    retval = SolverImpl::SOLVER_RUN_STATUS_FAILURE;
    break;
    
  case STATUS_UNKNOWN: 
  default:
    retval = SolverImpl::SOLVER_RUN_STATUS_UNEXPECTED_EXIT_CODE;
    break;
    
  }

  return retval;  
}

static void yicesTimeoutHandler(int x) {
  _exit(SolverImpl::SOLVER_RUN_STATUS_TIMEOUT);
}

static SolverImpl::SolverRunStatus runYicesAndGetCexForked(YicesBuilder *builder,
                                                      ::YicesContext *ctx,
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
    fprintf(stderr, "ERROR: fork failed (for Yices)");
    if (!IgnoreSolverFailures) 
      exit(1);
    return SolverImpl::SOLVER_RUN_STATUS_FORK_FAILED;
  }

  if (pid == 0) {
    if (timeout) {      
      ::alarm(0); /* Turn off alarm so we can safely set signal handler */
      ::signal(SIGALRM, yicesTimeoutHandler);
      ::alarm(std::max(1, (int)timeout));
    }
    /* i'm the child; in the salt mines */
    smt_status_t ycode = yices_check_context(ctx, NULL);
    SolverImpl::SolverRunStatus exit_code;

    switch(ycode){
    case STATUS_SAT: {
      hasSolution = true;
      exit_code = SolverImpl::SOLVER_RUN_STATUS_SUCCESS_SOLVABLE;
      values.reserve(objects.size());
      
      ::YicesModel* cex = yices_get_model(ctx, true);
      
      assert(cex != NULL);
      
      
      for (std::vector<const Array*>::const_iterator
	     it = objects.begin(), ie = objects.end(); it != ie; ++it) {
	const Array *array = *it;

	for (unsigned offset = 0; offset < array->size; offset++) {
	  int32_t buffer[8];
	  //a little dangerous (what guarantees that the result is 8-bit wide)
	  yices_get_bv_value(cex, builder->getInitialRead(array, offset), buffer);
	  unsigned val =  0;
	  unsigned j = 8;
	  while(j > 0){
	    j--;
	    val <<= 1;
	    if(buffer[j] != 0){
	      val  |= 1;
	    }
	  }
	  *pos++ = val;
	}
      }
      
      yices_free_model(cex);
      
      break;
    }
    
  case STATUS_UNSAT:
    hasSolution = false;
    exit_code = SolverImpl::SOLVER_RUN_STATUS_SUCCESS_UNSOLVABLE; 
    break;
    
  case STATUS_INTERRUPTED: 
    exit_code = SolverImpl::SOLVER_RUN_STATUS_TIMEOUT;
    break;
    
    
  case STATUS_ERROR:
    exit_code = SolverImpl::SOLVER_RUN_STATUS_FAILURE;
    break;
    
  case STATUS_UNKNOWN: 
  default:
    exit_code = SolverImpl::SOLVER_RUN_STATUS_UNEXPECTED_EXIT_CODE;
    break;
    
  }
    
    _exit(exit_code);

    
  } else {
    /* i'm the parent, waiting for my child */
    int status;
    pid_t res;

    do {
      res = waitpid(pid, &status, 0);
    } while (res < 0 && errno == EINTR);
    
    if (res < 0) {
      fprintf(stderr, "ERROR: waitpid() for Yices failed");
      if (!IgnoreSolverFailures) 
	exit(1);
      return SolverImpl::SOLVER_RUN_STATUS_WAITPID_FAILED;
    }
    
    // From timed_run.py: It appears that linux at least will on
    // "occasion" return a status when the process was terminated by a
    // signal, so test signal first.
    if (WIFSIGNALED(status) || !WIFEXITED(status)) {
      fprintf(stderr, "ERROR: Yices did not return successfully.  Most likely you forgot to run 'ulimit -s unlimited'\n");
      if (!IgnoreSolverFailures)  {
	exit(1);
      }
      return SolverImpl::SOLVER_RUN_STATUS_INTERRUPTED;
    }

    int exitcode = WEXITSTATUS(status);
    if (exitcode==SolverImpl::SOLVER_RUN_STATUS_SUCCESS_SOLVABLE) {
      hasSolution = true;
    } else if (exitcode==SolverImpl::SOLVER_RUN_STATUS_SUCCESS_UNSOLVABLE) {
      hasSolution = false;
    } else if (exitcode==SolverImpl::SOLVER_RUN_STATUS_TIMEOUT) {
      fprintf(stderr, "error: Yices timed out");
      return SolverImpl::SOLVER_RUN_STATUS_TIMEOUT;
    } else {
      fprintf(stderr, "error: Yices did not return a recognized code");
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
YicesSolverImpl::computeInitialValues(const Query &query,
                                      const std::vector<const Array*> 
                                      &objects,
                                      std::vector< std::vector<unsigned char> > 
                                      &values,
                                      bool &hasSolution) {
  runStatusCode =  SOLVER_RUN_STATUS_FAILURE; 
    
  TimerStatIncrementer t(stats::queryTime);

  ::YicesContext* ctx = yices_new_context(config);

  for (ConstraintManager::const_iterator it = query.constraints.begin(), 
         ie = query.constraints.end(); it != ie; ++it){
    int32_t ycode = yices_assert_formula(ctx, builder->construct(*it));
    assert(ycode == 0);
  }
  
  ++stats::queries;
  ++stats::queryCounterexamples;
  
  ::YicesExpr yices_e = yices_not(builder->construct(query.expr));
  int32_t ycode = yices_assert_formula(ctx, yices_e);

  if(ycode != 0){
    yices_print_error(stderr);
    assert(ycode == 0);
  }
  
  if (0) {
    fprintf(stderr, "note: Yices query: ");
    yices_pp_term(stderr, yices_e, 120, 64, strlen("note: Yices query: "));
  }

  bool success;
  if (useForkedYices) {
    runStatusCode = runYicesAndGetCexForked(builder, ctx, objects, values, 
                                       hasSolution, timeout);
    success = ((SOLVER_RUN_STATUS_SUCCESS_SOLVABLE == runStatusCode) ||
               (SOLVER_RUN_STATUS_SUCCESS_UNSOLVABLE == runStatusCode));    
  } else {
    runStatusCode = runYicesAndGetCex(builder, ctx, objects, values, hasSolution);    
    success = true;
  }
  
  if (success) {
    if (hasSolution)
      ++stats::queriesInvalid;
    else
      ++stats::queriesValid;
  }

  yices_free_context(ctx);
  
  return success;
}

SolverImpl::SolverRunStatus YicesSolverImpl::getOperationStatusCode() {
   return runStatusCode;
}


#endif /* SUPPORT_YICES */

