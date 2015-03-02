//===-- YicesSolverImpl.h  --------------------------------------*- C++ -*-===//
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

#ifndef YICESSOLVERIMPL_H_
#define YICESSOLVERIMPL_H_


#include "klee/Solver.h"
#include "klee/SolverImpl.h"

#ifdef SUPPORT_YICES


#include "YicesBuilder.h"


using namespace klee;

class YicesSolverImpl : public SolverImpl {
private:
  YicesBuilder *builder;
  ::YicesConfig *config;
  double timeout;
  bool useForkedYices;
  SolverRunStatus runStatusCode;

public:
  YicesSolverImpl(bool _useForkedYices, bool _optimizeDivides = true);
  ~YicesSolverImpl();
  
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

#endif /* SUPPORT_YICES */

#endif /* YICESSOLVERIMPL_H_ */
