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
#include <sstream>
#include <iostream>
using namespace klee;
using namespace llvm;
using namespace klee::util;

///

class PCLoggingSolver : public SolverImpl {
  Solver *solver;
  std::ofstream os;
  std::ostringstream logBuffer; // buffer to store logs before flushing to 
                                // file
  ExprPPrinter *printer;
  unsigned queryCount;
  int minQueryTimeToLog; // we log to file only those queries
                         // which take longer than specified time (ms);
                         // if this param is negative, log only those queries
                         // on which solver's timed out
  
  double startTime;
  double lastQueryTime;

  void startQuery(const Query& query, const char *typeName,
                  const ref<Expr> *evalExprsBegin = 0,
                  const ref<Expr> *evalExprsEnd = 0,
                  const Array * const* evalArraysBegin = 0,
                  const Array * const* evalArraysEnd = 0) {
    Statistic *S = theStatisticManager->getStatisticByName("Instructions");
    uint64_t instructions = S ? S->getValue() : 0;

    logBuffer << "# Query " << queryCount++ << " -- "
              << "Type: " << typeName << ", "
              << "Instructions: " << instructions << "\n";
    printer->printQuery(logBuffer, query.constraints, query.expr,
                        evalExprsBegin, evalExprsEnd,
                        evalArraysBegin, evalArraysEnd);

    startTime = getWallTime();
  }

  void finishQuery(bool success) {
    lastQueryTime = getWallTime() - startTime;
    logBuffer << "#   " << (success ? "OK" : "FAIL") << " -- "
              << "Elapsed: " << lastQueryTime << "\n";
    
    if (true == solver->impl->hasTimeoutOccurred()) {
        logBuffer << "\nSolver TIMEOUT detected\n";
    }
  }
  
  /// flushBuffer - flushes the temporary logs buffer. Depending on threshold
  /// settings, contents of the buffer are either discarded or written to a file.  
  void flushBuffer(void) {           
      logBuffer.flush();            

      if ((0 == minQueryTimeToLog) ||
          (static_cast<int>(lastQueryTime * 1000) > minQueryTimeToLog)) {
          // we either do not limit logging queries or the query time
          // is larger than threshold (in ms)
          
          if ((minQueryTimeToLog >= 0) || 
              (true == (solver->impl->hasTimeoutOccurred()))) {
              // we do additional check here to log only timeouts in case
              // user specified negative value for minQueryTimeToLog param
              os << logBuffer.str();
          }                    
      }
      
      // prepare the buffer for reuse
      logBuffer.clear();
      logBuffer.str("");
  }
  
public:
  PCLoggingSolver(Solver *_solver, std::string path, int queryTimeToLog) 
  : solver(_solver),
    os(path.c_str(), std::ios::trunc),
    logBuffer(""),
    printer(ExprPPrinter::create(logBuffer)),
    queryCount(0),    
    minQueryTimeToLog(queryTimeToLog),
    startTime(0.0f),
    lastQueryTime(0.0f) {
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
      logBuffer << "#   Is Valid: " << (isValid ? "true" : "false") << "\n";
    logBuffer << "\n";
    
    flushBuffer();
    
    return success;
  }

  bool computeValidity(const Query& query, Solver::Validity &result) {
    startQuery(query, "Validity");
    bool success = solver->impl->computeValidity(query, result);
    finishQuery(success);
    if (success)
      logBuffer << "#   Validity: " << result << "\n";
    logBuffer << "\n";
    
    flushBuffer();
    
    return success;
  }

  bool computeValue(const Query& query, ref<Expr> &result) {
    startQuery(query.withFalse(), "Value", 
               &query.expr, &query.expr + 1);
    bool success = solver->impl->computeValue(query, result);
    finishQuery(success);
    if (success)
      logBuffer << "#   Result: " << result << "\n";
    logBuffer << "\n";
    
    flushBuffer();
    
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
      logBuffer << "#   Solvable: " << (hasSolution ? "true" : "false") << "\n";
      if (hasSolution) {
        std::vector< std::vector<unsigned char> >::iterator
          values_it = values.begin();
        for (std::vector<const Array*>::const_iterator i = objects.begin(),
               e = objects.end(); i != e; ++i, ++values_it) {
          const Array *array = *i;
          std::vector<unsigned char> &data = *values_it;
          logBuffer << "#     " << array->name << " = [";
          for (unsigned j = 0; j < array->size; j++) {
            logBuffer << (int) data[j];
            if (j+1 < array->size)
              logBuffer << ",";
          }
          logBuffer << "]\n";
        }
      }
    }
    logBuffer << "\n";
    
    flushBuffer();
    
    return success;
  }
  
  bool hasTimeoutOccurred() {
    return solver->impl->hasTimeoutOccurred();
  }
};

///

Solver *klee::createPCLoggingSolver(Solver *_solver, std::string path,
                                    int minQueryTimeToLog) {
  return new Solver(new PCLoggingSolver(_solver, path, minQueryTimeToLog));
}
