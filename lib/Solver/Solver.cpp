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

#include "YicesBuilder.h"
#include "YicesSolverImpl.h"

#include "DummySolver.h"

#include "MetaSMTBuilder.h"

#include "klee/Constraints.h"
#include "klee/Expr.h"
#include "klee/TimerStatIncrementer.h"
#include "klee/util/Assignment.h"
#include "klee/util/ExprPPrinter.h"
#include "klee/util/ExprUtil.h"
#include "klee/Internal/Support/Timer.h"
#include "klee/CommandLine.h"

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
#include "llvm/Support/ErrorHandling.h"



using namespace klee;

#ifdef SUPPORT_METASMT

#include "SharedMem.h"

#include <metaSMT/DirectSolver_Context.hpp>
#include <metaSMT/backend/Z3_Backend.hpp>
#include <metaSMT/backend/Boolector.hpp>
#include <metaSMT/backend/MiniSAT.hpp>
#include <metaSMT/support/run_algorithm.hpp>
#include <metaSMT/API/Stack.hpp>
#include <metaSMT/API/Group.hpp>

using namespace metaSMT;
using namespace metaSMT::solver;

#endif /* SUPPORT_METASMT */

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

char *Solver::getConstraintLog(const Query& query) {
    return impl->getConstraintLog(query);
}

void Solver::setCoreSolverTimeout(double timeout) {
    impl->setCoreSolverTimeout(timeout);
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

#ifdef SUPPORT_METASMT

// ------------------------------------- MetaSMTSolverImpl class declaration ------------------------------

template<typename SolverContext>
class MetaSMTSolverImpl : public SolverImpl {
private:

  SolverContext _meta_solver;
  MetaSMTSolver<SolverContext>  *_solver;  
  MetaSMTBuilder<SolverContext> *_builder;
  double _timeout;
  bool   _useForked;
  SolverRunStatus _runStatusCode;

public:
  MetaSMTSolverImpl(MetaSMTSolver<SolverContext> *solver, bool useForked, bool optimizeDivides);  
  virtual ~MetaSMTSolverImpl();
  
  char *getConstraintLog(const Query&);
  void setCoreSolverTimeout(double timeout) { _timeout = timeout; }

  bool computeTruth(const Query&, bool &isValid);
  bool computeValue(const Query&, ref<Expr> &result);
    
  bool computeInitialValues(const Query &query,
                            const std::vector<const Array*> &objects,
                            std::vector< std::vector<unsigned char> > &values,
                            bool &hasSolution);
    
  SolverImpl::SolverRunStatus runAndGetCex(ref<Expr> query_expr,
                                           const std::vector<const Array*> &objects,
                                           std::vector< std::vector<unsigned char> > &values,
                                           bool &hasSolution);
  
  SolverImpl::SolverRunStatus runAndGetCexForked(const Query &query,
                                                 const std::vector<const Array*> &objects,
                                                 std::vector< std::vector<unsigned char> > &values,
                                                 bool &hasSolution,
                                                 double timeout);
  
  SolverRunStatus getOperationStatusCode();
  
  SolverContext& get_meta_solver() { return(_meta_solver); };
  
};


// ------------------------------------- MetaSMTSolver methods --------------------------------------------


template<typename SolverContext>
MetaSMTSolver<SolverContext>::MetaSMTSolver(bool useForked, bool optimizeDivides) 
  : Solver(new MetaSMTSolverImpl<SolverContext>(this, useForked, optimizeDivides))
{
   
}

template<typename SolverContext>
MetaSMTSolver<SolverContext>::~MetaSMTSolver()
{
  
}

template<typename SolverContext>
char *MetaSMTSolver<SolverContext>::getConstraintLog(const Query& query) {
  return(impl->getConstraintLog(query));
}

template<typename SolverContext>
void MetaSMTSolver<SolverContext>::setCoreSolverTimeout(double timeout) {
  impl->setCoreSolverTimeout(timeout);
}


// ------------------------------------- MetaSMTSolverImpl methods ----------------------------------------



template<typename SolverContext>
MetaSMTSolverImpl<SolverContext>::MetaSMTSolverImpl(MetaSMTSolver<SolverContext> *solver, bool useForked, bool optimizeDivides)
  : _solver(solver),    
    _builder(new MetaSMTBuilder<SolverContext>(_meta_solver, optimizeDivides)),
    _timeout(0.0),
    _useForked(useForked)
{  
  assert(_solver && "unable to create MetaSMTSolver");
  assert(_builder && "unable to create MetaSMTBuilder");
  
  if (_useForked) {
      shared_memory_id = shmget(IPC_PRIVATE, shared_memory_size, IPC_CREAT | 0700);
      assert(shared_memory_id >= 0 && "shmget failed");
      shared_memory_ptr = (unsigned char*) shmat(shared_memory_id, NULL, 0);
      assert(shared_memory_ptr != (void*) -1 && "shmat failed");
      shmctl(shared_memory_id, IPC_RMID, NULL);
  }
}

template<typename SolverContext>
MetaSMTSolverImpl<SolverContext>::~MetaSMTSolverImpl() {

}

template<typename SolverContext>
char *MetaSMTSolverImpl<SolverContext>::getConstraintLog(const Query&) {
  const char* msg = "Not supported";
  char *buf = new char[strlen(msg) + 1];
  strcpy(buf, msg);
  return(buf);
}

template<typename SolverContext>
bool MetaSMTSolverImpl<SolverContext>::computeTruth(const Query& query, bool &isValid) {  

  bool success = false;
  std::vector<const Array*> objects;
  std::vector< std::vector<unsigned char> > values;
  bool hasSolution;

  if (computeInitialValues(query, objects, values, hasSolution)) {
      // query.expr is valid iff !query.expr is not satisfiable
      isValid = !hasSolution;
      success = true;
  }

  return(success);
}

template<typename SolverContext>
bool MetaSMTSolverImpl<SolverContext>::computeValue(const Query& query, ref<Expr> &result) {
  
  bool success = false;
  std::vector<const Array*> objects;
  std::vector< std::vector<unsigned char> > values;
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

  return(success);
}


template<typename SolverContext>
bool MetaSMTSolverImpl<SolverContext>::computeInitialValues(const Query &query,
                                             const std::vector<const Array*> &objects,
                                             std::vector< std::vector<unsigned char> > &values,
                                             bool &hasSolution) {  

  _runStatusCode =  SOLVER_RUN_STATUS_FAILURE;

  TimerStatIncrementer t(stats::queryTime);
  assert(_builder);

  /*
   * FIXME push() and pop() work for Z3 but not for Boolector.
   * If using Z3, use push() and pop() and assert constraints.
   * If using Boolector, assume constrainsts instead of asserting them.
   */
  //push(_meta_solver);

  if (!_useForked) {
      for (ConstraintManager::const_iterator it = query.constraints.begin(), ie = query.constraints.end(); it != ie; ++it) {
          //assertion(_meta_solver, _builder->construct(*it));
          assumption(_meta_solver, _builder->construct(*it));  
      }  
  }  
    
  ++stats::queries;
  ++stats::queryCounterexamples;  
 
  bool success = true;
  if (_useForked) {
      _runStatusCode = runAndGetCexForked(query, objects, values, hasSolution, _timeout);
      success = ((SOLVER_RUN_STATUS_SUCCESS_SOLVABLE == _runStatusCode) || (SOLVER_RUN_STATUS_SUCCESS_UNSOLVABLE == _runStatusCode));
  }
  else {
      _runStatusCode = runAndGetCex(query.expr, objects, values, hasSolution);
      success = true;
  } 
    
  if (success) {
      if (hasSolution) {
          ++stats::queriesInvalid;
      }
      else {
          ++stats::queriesValid;
      }
  }  
   
  //pop(_meta_solver); 
  
  return(success);
}

template<typename SolverContext>
SolverImpl::SolverRunStatus MetaSMTSolverImpl<SolverContext>::runAndGetCex(ref<Expr> query_expr,
                                             const std::vector<const Array*> &objects,
                                             std::vector< std::vector<unsigned char> > &values,
                                             bool &hasSolution)
{

  // assume the negation of the query  
  assumption(_meta_solver, _builder->construct(Expr::createIsZero(query_expr)));  
  hasSolution = solve(_meta_solver);  
  
  if (hasSolution) {
      values.reserve(objects.size());
      for (std::vector<const Array*>::const_iterator it = objects.begin(), ie = objects.end(); it != ie; ++it) {
        
          const Array *array = *it;
          assert(array);
          typename SolverContext::result_type array_exp = _builder->getInitialArray(array);
           
          std::vector<unsigned char> data;      
          data.reserve(array->size);       
           
          for (unsigned offset = 0; offset < array->size; offset++) {
              typename SolverContext::result_type elem_exp = evaluate(
                       _meta_solver,
                       metaSMT::logic::Array::select(array_exp, bvuint(offset, array->getDomain())));
              unsigned char elem_value = metaSMT::read_value(_meta_solver, elem_exp);
              data.push_back(elem_value);
          }
                   
          values.push_back(data);
      }
  }
  
  if (true == hasSolution) {
      return(SolverImpl::SOLVER_RUN_STATUS_SUCCESS_SOLVABLE);
  }
  else {
      return(SolverImpl::SOLVER_RUN_STATUS_SUCCESS_UNSOLVABLE);  
  }
}

static void metaSMTTimeoutHandler(int x) {
  _exit(52);
}

template<typename SolverContext>
SolverImpl::SolverRunStatus MetaSMTSolverImpl<SolverContext>::runAndGetCexForked(const Query &query,
                                                          const std::vector<const Array*> &objects,
                                                          std::vector< std::vector<unsigned char> > &values,
                                                          bool &hasSolution,
                                                          double timeout)
{
  unsigned char *pos = shared_memory_ptr;
  unsigned sum = 0;
  for (std::vector<const Array*>::const_iterator it = objects.begin(), ie = objects.end(); it != ie; ++it) {
      sum += (*it)->size;    
  }
  // sum += sizeof(uint64_t);
  sum += sizeof(stats::queryConstructs);
  assert(sum < shared_memory_size && "not enough shared memory for counterexample");

  fflush(stdout);
  fflush(stderr);
  int pid = fork();
  if (pid == -1) {
      fprintf(stderr, "error: fork failed (for metaSMT)");
      return(SolverImpl::SOLVER_RUN_STATUS_FORK_FAILED);
  }
  
  if (pid == 0) {
      if (timeout) {
          ::alarm(0); /* Turn off alarm so we can safely set signal handler */
          ::signal(SIGALRM, metaSMTTimeoutHandler);
          ::alarm(std::max(1, (int) timeout));
      }     
      
      for (ConstraintManager::const_iterator it = query.constraints.begin(), ie = query.constraints.end(); it != ie; ++it) {      
          assertion(_meta_solver, _builder->construct(*it));
          //assumption(_meta_solver, _builder->construct(*it));  
      } 
      
      
      std::vector< std::vector<typename SolverContext::result_type> > aux_arr_exprs;
      if (UseMetaSMT == METASMT_BACKEND_BOOLECTOR) {
          for (std::vector<const Array*>::const_iterator it = objects.begin(), ie = objects.end(); it != ie; ++it) {
            
              std::vector<typename SolverContext::result_type> aux_arr;          
              const Array *array = *it;
              assert(array);
              typename SolverContext::result_type array_exp = _builder->getInitialArray(array);
              
              for (unsigned offset = 0; offset < array->size; offset++) {
                  typename SolverContext::result_type elem_exp = evaluate(
                      _meta_solver,
                      metaSMT::logic::Array::select(array_exp, bvuint(offset, array->getDomain())));
                  aux_arr.push_back(elem_exp);
              }
              aux_arr_exprs.push_back(aux_arr);
          }
          assert(aux_arr_exprs.size() == objects.size());
      }
      
           
      // assume the negation of the query
      // can be also asserted instead of assumed as we are in a child process
      assumption(_meta_solver, _builder->construct(Expr::createIsZero(query.expr)));        
      unsigned res = solve(_meta_solver);

      if (res) {

          if (UseMetaSMT != METASMT_BACKEND_BOOLECTOR) {

              for (std::vector<const Array*>::const_iterator it = objects.begin(), ie = objects.end(); it != ie; ++it) {

                  const Array *array = *it;
                  assert(array);
                  typename SolverContext::result_type array_exp = _builder->getInitialArray(array);      

                  for (unsigned offset = 0; offset < array->size; offset++) {

                      typename SolverContext::result_type elem_exp = evaluate(
                          _meta_solver,
                          metaSMT::logic::Array::select(array_exp, bvuint(offset, array->getDomain())));
                      unsigned char elem_value = metaSMT::read_value(_meta_solver, elem_exp);
                      *pos++ = elem_value;
                  }
              }
          }
          else {
              typename std::vector< std::vector<typename SolverContext::result_type> >::const_iterator eit = aux_arr_exprs.begin(), eie = aux_arr_exprs.end();
              for (std::vector<const Array*>::const_iterator it = objects.begin(), ie = objects.end(); it != ie, eit != eie; ++it, ++eit) {
                  const Array *array = *it;
                  const std::vector<typename SolverContext::result_type>& arr_exp = *eit;
                  assert(array);
                  assert(array->size == arr_exp.size());

                  for (unsigned offset = 0; offset < array->size; offset++) {
                      unsigned char elem_value = metaSMT::read_value(_meta_solver, arr_exp[offset]);
                       *pos++ = elem_value;
                  }
              }
          }
      }
      assert((uint64_t*) pos);
      *((uint64_t*) pos) = stats::queryConstructs;      
      
      _exit(!res);
  }
  else {
      int status;
      pid_t res;

      do {
          res = waitpid(pid, &status, 0);
      } 
      while (res < 0 && errno == EINTR);
    
      if (res < 0) {
          fprintf(stderr, "error: waitpid() for metaSMT failed");
          return(SolverImpl::SOLVER_RUN_STATUS_WAITPID_FAILED);
      }
      
      // From timed_run.py: It appears that linux at least will on
      // "occasion" return a status when the process was terminated by a
      // signal, so test signal first.
      if (WIFSIGNALED(status) || !WIFEXITED(status)) {
           fprintf(stderr, "error: metaSMT did not return successfully (status = %d) \n", WTERMSIG(status));
           return(SolverImpl::SOLVER_RUN_STATUS_INTERRUPTED);
      }

      int exitcode = WEXITSTATUS(status);
      if (exitcode == 0) {
          hasSolution = true;
      } 
      else if (exitcode == 1) {
          hasSolution = false;
      }
      else if (exitcode == 52) {
          fprintf(stderr, "error: metaSMT timed out");
          return(SolverImpl::SOLVER_RUN_STATUS_TIMEOUT);
      }
      else {
          fprintf(stderr, "error: metaSMT did not return a recognized code");
          return(SolverImpl::SOLVER_RUN_STATUS_UNEXPECTED_EXIT_CODE);
      }
      
      if (hasSolution) {
          values = std::vector< std::vector<unsigned char> >(objects.size());
          unsigned i = 0;
          for (std::vector<const Array*>::const_iterator it = objects.begin(), ie = objects.end(); it != ie; ++it) {
              const Array *array = *it;
              assert(array);
              std::vector<unsigned char> &data = values[i++];
              data.insert(data.begin(), pos, pos + array->size);
              pos += array->size;
          }
      }
      stats::queryConstructs += (*((uint64_t*) pos) - stats::queryConstructs);      

      if (true == hasSolution) {
          return SolverImpl::SOLVER_RUN_STATUS_SUCCESS_SOLVABLE;
      }
      else {
          return SolverImpl::SOLVER_RUN_STATUS_SUCCESS_UNSOLVABLE;
      }
  }
}

template<typename SolverContext>
SolverImpl::SolverRunStatus MetaSMTSolverImpl<SolverContext>::getOperationStatusCode() {
   return _runStatusCode;
}

template class MetaSMTSolver< DirectSolver_Context < Boolector> >;
template class MetaSMTSolver< DirectSolver_Context < Z3_Backend> >;
template class MetaSMTSolver< DirectSolver_Context < STP_Backend> >;

#endif /* SUPPORT_METASMT */

