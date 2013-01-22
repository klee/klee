//===-- Solver.cpp --------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Solver.h"
#include "klee/SolverImpl.h"

#include "SolverStats.h"
#include "STPBuilder.h"

#include "klee/Constraints.h"
#include "klee/Expr.h"
#include "klee/TimerStatIncrementer.h"
#include "klee/util/Assignment.h"
#include "klee/util/ExprPPrinter.h"
#include "klee/util/ExprUtil.h"
#include "klee/Internal/Support/Timer.h"

#define vc_bvBoolExtract IAMTHESPAWNOFSATAN

#include <cassert>
#include <cstdio>
#include <map>
#include <vector>

#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "llvm/Support/CommandLine.h"

llvm::cl::opt<bool>
IgnoreSolverFailures("ignore-solver-failures",
                     llvm::cl::init(false),
                     llvm::cl::desc("Ignore any solver failures (default=off)"));


using namespace klee;

/***/

SolverImpl::~SolverImpl() {
}

bool SolverImpl::computeValidity(const Query& query, Solver::Validity &result) {
  bool isTrue, isFalse;
  if (!computeTruth(query, isTrue))
    return false;
  if (isTrue) {
    result = Solver::True;
  } else {
    if (!computeTruth(query.negateExpr(), isFalse))
      return false;
    result = isFalse ? Solver::False : Solver::Unknown;
  }
  return true;
}

const char* SolverImpl::getOperationStatusString(SolverRunStatus statusCode)
{
    switch (statusCode)
    {
        case SOLVER_RUN_STATUS_SUCCESS_SOLVABLE:
            return "OPERATION SUCCESSFUL, QUERY IS SOLVABLE";
        case SOLVER_RUN_STATUS_SUCCESS_UNSOLVABLE:
            return "OPERATION SUCCESSFUL, QUERY IS UNSOLVABLE";
        case SOLVER_RUN_STATUS_FAILURE:
            return "OPERATION FAILED";
        case SOLVER_RUN_STATUS_TIMEOUT:
            return "SOLVER TIMEOUT";
        case SOLVER_RUN_STATUS_FORK_FAILED:
            return "FORK FAILED";
        case SOLVER_RUN_STATUS_INTERRUPTED:
            return "SOLVER PROCESS INTERRUPTED";
        case SOLVER_RUN_STATUS_UNEXPECTED_EXIT_CODE:
            return "UNEXPECTED SOLVER PROCESS EXIT CODE";
        case SOLVER_RUN_STATUS_WAITPID_FAILED:
            return "WAITPID FAILED FOR SOLVER PROCESS";
        default:
            return "UNRECOGNIZED OPERATION STATUS";        
    }    
}

const char *Solver::validity_to_str(Validity v) {
  switch (v) {
  default:    return "Unknown";
  case True:  return "True";
  case False: return "False";
  }
}

Solver::~Solver() { 
  delete impl; 
}

bool Solver::evaluate(const Query& query, Validity &result) {
  assert(query.expr->getWidth() == Expr::Bool && "Invalid expression type!");

  // Maintain invariants implementations expect.
  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(query.expr)) {
    result = CE->isTrue() ? True : False;
    return true;
  }

  return impl->computeValidity(query, result);
}

bool Solver::mustBeTrue(const Query& query, bool &result) {
  assert(query.expr->getWidth() == Expr::Bool && "Invalid expression type!");

  // Maintain invariants implementations expect.
  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(query.expr)) {
    result = CE->isTrue() ? true : false;
    return true;
  }

  return impl->computeTruth(query, result);
}

bool Solver::mustBeFalse(const Query& query, bool &result) {
  return mustBeTrue(query.negateExpr(), result);
}

bool Solver::mayBeTrue(const Query& query, bool &result) {
  bool res;
  if (!mustBeFalse(query, res))
    return false;
  result = !res;
  return true;
}

bool Solver::mayBeFalse(const Query& query, bool &result) {
  bool res;
  if (!mustBeTrue(query, res))
    return false;
  result = !res;
  return true;
}

bool Solver::getValue(const Query& query, ref<ConstantExpr> &result) {
  // Maintain invariants implementation expect.
  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(query.expr)) {
    result = CE;
    return true;
  }

  // FIXME: Push ConstantExpr requirement down.
  ref<Expr> tmp;
  if (!impl->computeValue(query, tmp))
    return false;
  
  result = cast<ConstantExpr>(tmp);
  return true;
}

bool 
Solver::getInitialValues(const Query& query,
                         const std::vector<const Array*> &objects,
                         std::vector< std::vector<unsigned char> > &values) {
  bool hasSolution;
  bool success =
    impl->computeInitialValues(query, objects, values, hasSolution);
  // FIXME: Propogate this out.
  if (!hasSolution)
    return false;
    
  return success;
}

std::pair< ref<Expr>, ref<Expr> > Solver::getRange(const Query& query) {
  ref<Expr> e = query.expr;
  Expr::Width width = e->getWidth();
  uint64_t min, max;

  if (width==1) {
    Solver::Validity result;
    if (!evaluate(query, result))
      assert(0 && "computeValidity failed");
    switch (result) {
    case Solver::True: 
      min = max = 1; break;
    case Solver::False: 
      min = max = 0; break;
    default:
      min = 0, max = 1; break;
    }
  } else if (ConstantExpr *CE = dyn_cast<ConstantExpr>(e)) {
    min = max = CE->getZExtValue();
  } else {
    // binary search for # of useful bits
    uint64_t lo=0, hi=width, mid, bits=0;
    while (lo<hi) {
      mid = lo + (hi - lo)/2;
      bool res;
      bool success = 
        mustBeTrue(query.withExpr(
                     EqExpr::create(LShrExpr::create(e,
                                                     ConstantExpr::create(mid, 
                                                                          width)),
                                    ConstantExpr::create(0, width))),
                   res);

      assert(success && "FIXME: Unhandled solver failure");
      (void) success;

      if (res) {
        hi = mid;
      } else {
        lo = mid+1;
      }

      bits = lo;
    }
    
    // could binary search for training zeros and offset
    // min max but unlikely to be very useful

    // check common case
    bool res = false;
    bool success = 
      mayBeTrue(query.withExpr(EqExpr::create(e, ConstantExpr::create(0, 
                                                                      width))), 
                res);

    assert(success && "FIXME: Unhandled solver failure");      
    (void) success;

    if (res) {
      min = 0;
    } else {
      // binary search for min
      lo=0, hi=bits64::maxValueOfNBits(bits);
      while (lo<hi) {
        mid = lo + (hi - lo)/2;
        bool res = false;
        bool success = 
          mayBeTrue(query.withExpr(UleExpr::create(e, 
                                                   ConstantExpr::create(mid, 
                                                                        width))),
                    res);

        assert(success && "FIXME: Unhandled solver failure");      
        (void) success;

        if (res) {
          hi = mid;
        } else {
          lo = mid+1;
        }
      }

      min = lo;
    }

    // binary search for max
    lo=min, hi=bits64::maxValueOfNBits(bits);
    while (lo<hi) {
      mid = lo + (hi - lo)/2;
      bool res;
      bool success = 
        mustBeTrue(query.withExpr(UleExpr::create(e, 
                                                  ConstantExpr::create(mid, 
                                                                       width))),
                   res);

      assert(success && "FIXME: Unhandled solver failure");      
      (void) success;

      if (res) {
        hi = mid;
      } else {
        lo = mid+1;
      }
    }

    max = lo;
  }

  return std::make_pair(ConstantExpr::create(min, width),
                        ConstantExpr::create(max, width));
}

/***/

class ValidatingSolver : public SolverImpl {
private:
  Solver *solver, *oracle;

public: 
  ValidatingSolver(Solver *_solver, Solver *_oracle) 
    : solver(_solver), oracle(_oracle) {}
  ~ValidatingSolver() { delete solver; }
  
  bool computeValidity(const Query&, Solver::Validity &result);
  bool computeTruth(const Query&, bool &isValid);
  bool computeValue(const Query&, ref<Expr> &result);
  bool computeInitialValues(const Query&,
                            const std::vector<const Array*> &objects,
                            std::vector< std::vector<unsigned char> > &values,
                            bool &hasSolution);
  SolverRunStatus getOperationStatusCode();
};

bool ValidatingSolver::computeTruth(const Query& query,
                                    bool &isValid) {
  bool answer;
  
  if (!solver->impl->computeTruth(query, isValid))
    return false;
  if (!oracle->impl->computeTruth(query, answer))
    return false;
  
  if (isValid != answer)
    assert(0 && "invalid solver result (computeTruth)");
  
  return true;
}

bool ValidatingSolver::computeValidity(const Query& query,
                                       Solver::Validity &result) {
  Solver::Validity answer;
  
  if (!solver->impl->computeValidity(query, result))
    return false;
  if (!oracle->impl->computeValidity(query, answer))
    return false;
  
  if (result != answer)
    assert(0 && "invalid solver result (computeValidity)");
  
  return true;
}

bool ValidatingSolver::computeValue(const Query& query,
                                    ref<Expr> &result) {  
  bool answer;

  if (!solver->impl->computeValue(query, result))
    return false;
  // We don't want to compare, but just make sure this is a legal
  // solution.
  if (!oracle->impl->computeTruth(query.withExpr(NeExpr::create(query.expr, 
                                                                result)),
                                  answer))
    return false;

  if (answer)
    assert(0 && "invalid solver result (computeValue)");

  return true;
}

bool 
ValidatingSolver::computeInitialValues(const Query& query,
                                       const std::vector<const Array*>
                                         &objects,
                                       std::vector< std::vector<unsigned char> >
                                         &values,
                                       bool &hasSolution) {
  bool answer;

  if (!solver->impl->computeInitialValues(query, objects, values, 
                                          hasSolution))
    return false;

  if (hasSolution) {
    // Assert the bindings as constraints, and verify that the
    // conjunction of the actual constraints is satisfiable.
    std::vector< ref<Expr> > bindings;
    for (unsigned i = 0; i != values.size(); ++i) {
      const Array *array = objects[i];
      for (unsigned j=0; j<array->size; j++) {
        unsigned char value = values[i][j];
        bindings.push_back(EqExpr::create(ReadExpr::create(UpdateList(array, 0),
                                                           ConstantExpr::alloc(j, Expr::Int32)),
                                          ConstantExpr::alloc(value, Expr::Int8)));
      }
    }
    ConstraintManager tmp(bindings);
    ref<Expr> constraints = Expr::createIsZero(query.expr);
    for (ConstraintManager::const_iterator it = query.constraints.begin(), 
           ie = query.constraints.end(); it != ie; ++it)
      constraints = AndExpr::create(constraints, *it);
    
    if (!oracle->impl->computeTruth(Query(tmp, constraints), answer))
      return false;
    if (!answer)
      assert(0 && "invalid solver result (computeInitialValues)");
  } else {
    if (!oracle->impl->computeTruth(query, answer))
      return false;
    if (!answer)
      assert(0 && "invalid solver result (computeInitialValues)");    
  }

  return true;
}

SolverImpl::SolverRunStatus ValidatingSolver::getOperationStatusCode() {
    return solver->impl->getOperationStatusCode();
}

Solver *klee::createValidatingSolver(Solver *s, Solver *oracle) {
  return new Solver(new ValidatingSolver(s, oracle));
}

/***/

class DummySolverImpl : public SolverImpl {
public: 
  DummySolverImpl() {}
  
  bool computeValidity(const Query&, Solver::Validity &result) { 
    ++stats::queries;
    // FIXME: We should have stats::queriesFail;
    return false; 
  }
  bool computeTruth(const Query&, bool &isValid) { 
    ++stats::queries;
    // FIXME: We should have stats::queriesFail;
    return false; 
  }
  bool computeValue(const Query&, ref<Expr> &result) { 
    ++stats::queries;
    ++stats::queryCounterexamples;
    return false; 
  }
  bool computeInitialValues(const Query&,
                            const std::vector<const Array*> &objects,
                            std::vector< std::vector<unsigned char> > &values,
                            bool &hasSolution) { 
    ++stats::queries;
    ++stats::queryCounterexamples;
    return false; 
  }
  SolverRunStatus getOperationStatusCode() {
      return SOLVER_RUN_STATUS_FAILURE;
  }
  
};

Solver *klee::createDummySolver() {
  return new Solver(new DummySolverImpl());
}

/***/

class STPSolverImpl : public SolverImpl {
private:
  /// The solver we are part of, for access to public information.
  STPSolver *solver;
  VC vc;
  STPBuilder *builder;
  double timeout;
  bool useForkedSTP;
  SolverRunStatus runStatusCode;

public:
  STPSolverImpl(STPSolver *_solver, bool _useForkedSTP, bool _optimizeDivides = true);
  ~STPSolverImpl();

  char *getConstraintLog(const Query&);
  void setTimeout(double _timeout) { timeout = _timeout; }

  bool computeTruth(const Query&, bool &isValid);
  bool computeValue(const Query&, ref<Expr> &result);
  bool computeInitialValues(const Query&,
                            const std::vector<const Array*> &objects,
                            std::vector< std::vector<unsigned char> > &values,
                            bool &hasSolution);
  SolverRunStatus getOperationStatusCode();
};

static unsigned char *shared_memory_ptr;
static const unsigned shared_memory_size = 1<<20;
static int shared_memory_id;

static void stp_error_handler(const char* err_msg) {
  fprintf(stderr, "error: STP Error: %s\n", err_msg);
  abort();
}

STPSolverImpl::STPSolverImpl(STPSolver *_solver, bool _useForkedSTP, bool _optimizeDivides)
  : solver(_solver),
    vc(vc_createValidityChecker()),
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
    shared_memory_id = shmget(IPC_PRIVATE, shared_memory_size, IPC_CREAT | 0700);
    assert(shared_memory_id>=0 && "shmget failed");
    shared_memory_ptr = (unsigned char*) shmat(shared_memory_id, NULL, 0);
    assert(shared_memory_ptr!=(void*)-1 && "shmat failed");
    shmctl(shared_memory_id, IPC_RMID, NULL);
  }
}

STPSolverImpl::~STPSolverImpl() {
  delete builder;

  vc_Destroy(vc);
}

/***/

STPSolver::STPSolver(bool useForkedSTP, bool optimizeDivides)
  : Solver(new STPSolverImpl(this, useForkedSTP, optimizeDivides))
{
}

char *STPSolver::getConstraintLog(const Query &query) {
  return static_cast<STPSolverImpl*>(impl)->getConstraintLog(query);  
}

void STPSolver::setTimeout(double timeout) {
  static_cast<STPSolverImpl*>(impl)->setTimeout(timeout);
}

/***/

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

static SolverImpl::SolverRunStatus runAndGetCex(::VC vc, STPBuilder *builder, ::VCExpr q,
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

static SolverImpl::SolverRunStatus runAndGetCexForked(::VC vc, 
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
  assert(sum<shared_memory_size && "not enough shared memory for counterexample");

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
#include <iostream>
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
    runStatusCode = runAndGetCexForked(vc, builder, stp_e, objects, values, 
                                       hasSolution, timeout);
    success = ((SOLVER_RUN_STATUS_SUCCESS_SOLVABLE == runStatusCode) ||
               (SOLVER_RUN_STATUS_SUCCESS_UNSOLVABLE == runStatusCode));    
  } else {
    runStatusCode = runAndGetCex(vc, builder, stp_e, objects, values, hasSolution);    
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
