//===-- QueryLoggingSolver.cpp --------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#include "QueryLoggingSolver.h"

#include "klee/Config/config.h"
#include "klee/Internal/Support/ErrorHandling.h"
#include "klee/Internal/Support/FileHandling.h"
#include "klee/Internal/System/Time.h"
#include "klee/OptionCategories.h"
#include "klee/Statistics.h"

namespace {
llvm::cl::opt<bool> DumpPartialQueryiesEarly(
    "log-partial-queries-early", llvm::cl::init(false),
    llvm::cl::desc("Log queries before calling the solver (default=false)"),
    llvm::cl::cat(klee::SolvingCat));

#ifdef HAVE_ZLIB_H
llvm::cl::opt<bool> CreateCompressedQueryLog(
    "compress-query-log", llvm::cl::init(false),
    llvm::cl::desc("Compress query log files (default=false)"),
    llvm::cl::cat(klee::SolvingCat));
#endif
} // namespace

QueryLoggingSolver::QueryLoggingSolver(Solver *_solver, std::string path,
                                       const std::string &commentSign,
                                       time::Span queryTimeToLog,
                                       bool logTimedOut)
    : solver(_solver), BufferString(""), logBuffer(BufferString), queryCount(0),
      minQueryTimeToLog(queryTimeToLog), logTimedOutQueries(logTimedOut),
      queryCommentSign(commentSign) {
  std::string error;
#ifdef HAVE_ZLIB_H
  if (!CreateCompressedQueryLog) {
#endif
    os = klee_open_output_file(path, error);
#ifdef HAVE_ZLIB_H
  } else {
    path.append(".gz");
    os = klee_open_compressed_output_file(path, error);
  }
#endif
  if (!os) {
    klee_error("Could not open file %s : %s", path.c_str(), error.c_str());
  }
  assert(0 != solver);
}

QueryLoggingSolver::~QueryLoggingSolver() {
  delete solver;
}

void QueryLoggingSolver::flushBufferConditionally(bool writeToFile) {
  logBuffer.flush();
  if (writeToFile) {
    *os << logBuffer.str();
    os->flush();
  }
  // prepare the buffer for reuse
  BufferString = "";
}

void QueryLoggingSolver::startQuery(const Query &query, const char *typeName,
                                    const Query *falseQuery,
                                    const std::vector<const Array *> *objects) {
  Statistic *S = theStatisticManager->getStatisticByName("Instructions");
  uint64_t instructions = S ? S->getValue() : 0;

  logBuffer << queryCommentSign << " Query " << queryCount++ << " -- "
            << "Type: " << typeName << ", "
            << "Instructions: " << instructions << "\n";

  printQuery(query, falseQuery, objects);

  if (DumpPartialQueryiesEarly) {
    flushBufferConditionally(true);
  }
  startTime = time::getWallTime();
}

void QueryLoggingSolver::finishQuery(bool success) {
  lastQueryDuration = time::getWallTime() - startTime;
  logBuffer << queryCommentSign << "   " << (success ? "OK" : "FAIL") << " -- "
            << "Elapsed: " << lastQueryDuration << "\n";

  if (false == success) {
    logBuffer << queryCommentSign << "   Failure reason: "
              << SolverImpl::getOperationStatusString(
                     solver->impl->getOperationStatusCode()) << "\n";
  }
}

void QueryLoggingSolver::flushBuffer() {
  // we either do not limit logging queries
  // or the query time is larger than threshold
  // or we log a timed out query
  bool writeToFile = (!minQueryTimeToLog)
      || (lastQueryDuration > minQueryTimeToLog)
      || (logTimedOutQueries &&
         (SOLVER_RUN_STATUS_TIMEOUT == solver->impl->getOperationStatusCode()));

  flushBufferConditionally(writeToFile);
}

bool QueryLoggingSolver::computeTruth(const Query &query, bool &isValid) {
  startQuery(query, "Truth");

  bool success = solver->impl->computeTruth(query, isValid);

  finishQuery(success);

  if (success) {
    logBuffer << queryCommentSign
              << "   Is Valid: " << (isValid ? "true" : "false") << "\n";
  }
  logBuffer << "\n";

  flushBuffer();

  return success;
}

bool QueryLoggingSolver::computeValidity(const Query &query,
                                         Solver::Validity &result) {
  startQuery(query, "Validity");

  bool success = solver->impl->computeValidity(query, result);

  finishQuery(success);

  if (success) {
    logBuffer << queryCommentSign << "   Validity: " << result << "\n";
  }
  logBuffer << "\n";

  flushBuffer();

  return success;
}

bool QueryLoggingSolver::computeValue(const Query &query, ref<Expr> &result) {
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

bool QueryLoggingSolver::computeInitialValues(
    const Query &query, const std::vector<const Array *> &objects,
    std::vector<std::vector<unsigned char> > &values, bool &hasSolution) {
  startQuery(query, "InitialValues", 0, &objects);

  bool success =
      solver->impl->computeInitialValues(query, objects, values, hasSolution);

  finishQuery(success);

  if (success) {
    logBuffer << queryCommentSign
              << "   Solvable: " << (hasSolution ? "true" : "false") << "\n";
    if (hasSolution) {
      std::vector<std::vector<unsigned char> >::iterator values_it =
          values.begin();

      for (std::vector<const Array *>::const_iterator i = objects.begin(),
                                                      e = objects.end();
           i != e; ++i, ++values_it) {
        const Array *array = *i;
        std::vector<unsigned char> &data = *values_it;
        logBuffer << queryCommentSign << "     " << array->name << " = [";

        for (unsigned j = 0; j < array->size; j++) {
          logBuffer << (int)data[j];

          if (j + 1 < array->size) {
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

char *QueryLoggingSolver::getConstraintLog(const Query &query) {
  return solver->impl->getConstraintLog(query);
}

void QueryLoggingSolver::setCoreSolverTimeout(time::Span timeout) {
  solver->impl->setCoreSolverTimeout(timeout);
}
