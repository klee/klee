//===-- QueryLoggingSolver.cpp --------------------------------------------===//

#include "QueryLoggingSolver.h"
#include "klee/Internal/System/Time.h"
#include "klee/Statistics.h"

//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

using namespace klee::util;

QueryLoggingSolver::QueryLoggingSolver(Solver *_solver,
                                       std::string path,
                                       const std::string& commentSign,
                                       int queryTimeToLog)                                  
    : solver(_solver), 
      os(path.c_str(), std::ios::trunc),
      logBuffer(""),
      queryCount(0),    
      minQueryTimeToLog(queryTimeToLog),
      startTime(0.0f),
      lastQueryTime(0.0f),
      queryCommentSign(commentSign) {
    assert(0 != solver);
}

QueryLoggingSolver::~QueryLoggingSolver() {
    delete solver;
}

void QueryLoggingSolver::startQuery(const Query& query, const char* typeName,
                                    const Query* falseQuery,
                                    const std::vector<const Array*> *objects) {
    Statistic *S = theStatisticManager->getStatisticByName("Instructions");
    uint64_t instructions = S ? S->getValue() : 0;

    logBuffer << queryCommentSign << " Query " << queryCount++ << " -- "
              << "Type: " << typeName << ", "
              << "Instructions: " << instructions << "\n";
    
    printQuery(query, falseQuery, objects);
    
    startTime = getWallTime();
    
}

void QueryLoggingSolver::finishQuery(bool success) {
    lastQueryTime = getWallTime() - startTime;
    logBuffer << queryCommentSign << "   " << (success ? "OK" : "FAIL") << " -- "
              << "Elapsed: " << lastQueryTime << "\n";
    
    if (false == success) {
        logBuffer << queryCommentSign << "   Failure reason: "
                  << SolverImpl::getOperationStatusString(solver->impl->getOperationStatusCode())
                  << "\n";
    }
}

void QueryLoggingSolver::flushBuffer() {
    logBuffer.flush();            

      if ((0 == minQueryTimeToLog) ||
          (static_cast<int>(lastQueryTime * 1000) > minQueryTimeToLog)) {
          // we either do not limit logging queries or the query time
          // is larger than threshold (in ms)
          
          if ((minQueryTimeToLog >= 0) || 
              (SOLVER_RUN_STATUS_TIMEOUT == (solver->impl->getOperationStatusCode()))) {
              // we do additional check here to log only timeouts in case
              // user specified negative value for minQueryTimeToLog param
              os << logBuffer.str();              
          }                    
      }
      
      // prepare the buffer for reuse
      logBuffer.clear();
      logBuffer.str("");
}

bool QueryLoggingSolver::computeTruth(const Query& query, bool& isValid) {
    startQuery(query, "Truth");
    
    bool success = solver->impl->computeTruth(query, isValid);
    
    finishQuery(success);
    
    if (success) {
        logBuffer << queryCommentSign << "   Is Valid: " 
                  << (isValid ? "true" : "false") << "\n";
    }
    logBuffer << "\n";
    
    flushBuffer();
    
    return success;    
}

bool QueryLoggingSolver::computeValidity(const Query& query,
                                         Solver::Validity& result) {
    startQuery(query, "Validity");
    
    bool success = solver->impl->computeValidity(query, result);
    
    finishQuery(success);
    
    if (success) {
        logBuffer << queryCommentSign << "   Validity: " 
                  << result << "\n";
    }
    logBuffer << "\n";
    
    flushBuffer();
    
    return success;
}

bool QueryLoggingSolver::computeValue(const Query& query, ref<Expr>& result) {
    Query withFalse = query.withFalse();
    startQuery(query, "Value", &withFalse);

    bool success = solver->impl->computeValue(query, result);
    
    finishQuery(success);
    
    if (success) {
        logBuffer << queryCommentSign << "   Result: " << result << "\n";
    }
    logBuffer << "\n";
    
    flushBuffer();
    
    return success;
}

bool QueryLoggingSolver::computeInitialValues(const Query& query,
                                              const std::vector<const Array*>& objects,
                                              std::vector<std::vector<unsigned char> >& values,
                                              bool& hasSolution) {
    startQuery(query, "InitialValues", 0, &objects);

    bool success = solver->impl->computeInitialValues(query, objects, 
                                                      values, hasSolution);
    
    finishQuery(success);
    
    if (success) {
        logBuffer << queryCommentSign << "   Solvable: " 
                  << (hasSolution ? "true" : "false") << "\n";
        if (hasSolution) {
            std::vector< std::vector<unsigned char> >::iterator
            values_it = values.begin();
        
            for (std::vector<const Array*>::const_iterator i = objects.begin(),
                 e = objects.end(); i != e; ++i, ++values_it) {
                const Array *array = *i;
                std::vector<unsigned char> &data = *values_it;
                logBuffer << queryCommentSign << "     " << array->name << " = [";
                
                for (unsigned j = 0; j < array->size; j++) {
                    logBuffer << (int) data[j];
            
                    if (j+1 < array->size) {
                        logBuffer << ",";
                    }                    
              }
              logBuffer << "]\n";
            }
        }
    }
    logBuffer << "\n";
    
    flushBuffer();
    
    return success;
}

SolverImpl::SolverRunStatus QueryLoggingSolver::getOperationStatusCode() {
    return solver->impl->getOperationStatusCode();
}


