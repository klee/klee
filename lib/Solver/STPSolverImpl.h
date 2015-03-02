#ifndef STPSOLVERIMPL_H_
#define STPSOLVERIMPL_H_

#include "klee/Solver.h"
#include "klee/SolverImpl.h"

#include "STPBuilder.h"

using namespace klee;

class STPSolverImpl : public SolverImpl {
private:
  VC vc;
  STPBuilder *builder;
  double timeout;
  bool useForkedSTP;
  SolverRunStatus runStatusCode;

public:
  STPSolverImpl(bool _useForkedSTP, bool _optimizeDivides = true);
  ~STPSolverImpl();
  
  char *getConstraintLog(const Query&);
  void setCoreSolverTimeout(double _timeout) { timeout = _timeout; }

  bool computeTruth(const Query&, bool &isValid);
  bool computeValue(const Query&, ref<Expr> &result);
  bool computeInitialValues(const Query&,
                            const std::vector<const Array*> &objects,
                            std::vector< std::vector<unsigned char> > &values,
                            bool &hasSolution);
  SolverRunStatus getOperationStatusCode();
};



#endif /* STPSOLVERIMPL_H_ */
