#ifndef VALIDATINGSOLVER_H_
#define VALIDATINGSOLVER_H_

#include "klee/Solver.h"
#include "klee/SolverImpl.h"

using namespace klee;

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
  char *getConstraintLog(const Query&);
  void setCoreSolverTimeout(double timeout);
};


#endif /* VALIDATINGSOLVER_H_ */
