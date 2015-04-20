#ifndef DUMMYSOLVER_H_
#define DUMMYSOLVER_H_

#include "klee/Solver.h"
#include "klee/SolverImpl.h"

using namespace klee;


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



#endif /* DUMMYSOLVER_H_ */
