//===-- PCLoggingSolver.cpp -----------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Solver.h"

#include "klee/Expr.h"
#include "klee/SolverImpl.h"
#include "klee/Statistics.h"
#include "klee/util/ExprPPrinter.h"
#include "klee/Internal/Support/QueryLog.h"
#include "klee/Internal/System/Time.h"

#include "llvm/Support/CommandLine.h"

#include <fstream>

using namespace klee;
using namespace llvm;
using namespace klee::util;

///

class PCLoggingSolver : public SolverImpl {
  Solver *solver;
  std::ofstream os;
  ExprPPrinter *printer;
  unsigned queryCount;
  double startTime;

  void startQuery(const Query& query, const char *typeName,
                  const ref<Expr> *evalExprsBegin = 0,
                  const ref<Expr> *evalExprsEnd = 0,
                  const Array * const* evalArraysBegin = 0,
                  const Array * const* evalArraysEnd = 0) {
    Statistic *S = theStatisticManager->getStatisticByName("Instructions");
    uint64_t instructions = S ? S->getValue() : 0;
    os << "# Query " << queryCount++ << " -- "
       << "Type: " << typeName << ", "
       << "Instructions: " << instructions << "\n";
    printer->printQuery(os, query.constraints, query.expr,
                        evalExprsBegin, evalExprsEnd,
                        evalArraysBegin, evalArraysEnd);
    
    startTime = getWallTime();
  }

  void finishQuery(bool success) {
    double delta = getWallTime() - startTime;
    os << "#   " << (success ? "OK" : "FAIL") << " -- "
       << "Elapsed: " << delta << "\n";
  }
  
public:
  PCLoggingSolver(Solver *_solver, std::string path) 
  : solver(_solver),
    os(path.c_str(), std::ios::trunc),
    printer(ExprPPrinter::create(os)),
    queryCount(0) {
  }                                                      
  ~PCLoggingSolver() {
    delete printer;
    delete solver;
  }
  
  bool computeTruth(const Query& query, bool &isValid) {
    startQuery(query, "Truth");
    bool success = solver->impl->computeTruth(query, isValid);
    finishQuery(success);
    if (success)
      os << "#   Is Valid: " << (isValid ? "true" : "false") << "\n";
    os << "\n";
    return success;
  }

  bool computeValidity(const Query& query, Solver::Validity &result) {
    startQuery(query, "Validity");
    bool success = solver->impl->computeValidity(query, result);
    finishQuery(success);
    if (success)
      os << "#   Validity: " << result << "\n";
    os << "\n";
    return success;
  }

  bool computeValue(const Query& query, ref<Expr> &result) {
    startQuery(query.withFalse(), "Value", 
               &query.expr, &query.expr + 1);
    bool success = solver->impl->computeValue(query, result);
    finishQuery(success);
    if (success)
      os << "#   Result: " << result << "\n";
    os << "\n";
    return success;
  }

  bool computeInitialValues(const Query& query,
                            const std::vector<const Array*> &objects,
                            std::vector< std::vector<unsigned char> > &values,
                            bool &hasSolution) {
    if (objects.empty()) {
      startQuery(query, "InitialValues",
                 0, 0);
    } else {
      startQuery(query, "InitialValues",
                 0, 0,
                 &objects[0], &objects[0] + objects.size());
    }
    bool success = solver->impl->computeInitialValues(query, objects, 
                                                      values, hasSolution);
    finishQuery(success);
    if (success) {
      os << "#   Solvable: " << (hasSolution ? "true" : "false") << "\n";
      if (hasSolution) {
        std::vector< std::vector<unsigned char> >::iterator
          values_it = values.begin();
        for (std::vector<const Array*>::const_iterator i = objects.begin(),
               e = objects.end(); i != e; ++i, ++values_it) {
          const Array *array = *i;
          std::vector<unsigned char> &data = *values_it;
          os << "#     " << array->name << " = [";
          for (unsigned j = 0; j < array->size; j++) {
            os << (int) data[j];
            if (j+1 < array->size)
              os << ",";
          }
          os << "]\n";
        }
      }
    }
    os << "\n";
    return success;
  }
};

///

Solver *klee::createPCLoggingSolver(Solver *_solver, std::string path) {
  return new Solver(new PCLoggingSolver(_solver, path));
}
