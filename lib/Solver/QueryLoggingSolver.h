//===-- QueryLoggingSolver.h
//---------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_QUERYLOGGINGSOLVER_H
#define KLEE_QUERYLOGGINGSOLVER_H

#include "klee/Solver.h"
#include "klee/SolverImpl.h"
#include "llvm/Support/raw_ostream.h"

using namespace klee;

/// This abstract class represents a solver that is capable of logging
/// queries to a file.
/// Derived classes might specialize this one by providing different formats
/// for the query output.
class QueryLoggingSolver : public SolverImpl {

protected:
  Solver *solver;
  std::string ErrorInfo;
#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 6)
  std::error_code ec;
#endif
  llvm::raw_fd_ostream os;
  // @brief Buffer used by logBuffer
  std::string BufferString;
  // @brief buffer to store logs before flushing to file
  llvm::raw_string_ostream logBuffer;
  unsigned queryCount;
  int minQueryTimeToLog; // we log to file only those queries
                         // which take longer than the specified time (ms);
                         // if this param is negative, log only those queries
                         // on which the solver has timed out

  double startTime;
  double lastQueryTime;
  const std::string queryCommentSign; // sign representing commented lines
                                      // in given a query format

  virtual void startQuery(const Query &query, const char *typeName,
                          const Query *falseQuery = 0,
                          const std::vector<const Array *> *objects = 0);

  virtual void finishQuery(bool success);

  /// flushBuffer - flushes the temporary logs buffer. Depending on threshold
  /// settings, contents of the buffer are either discarded or written to a
  /// file.
  void flushBuffer(void);

  virtual void printQuery(const Query &query, const Query *falseQuery = 0,
                          const std::vector<const Array *> *objects = 0) = 0;
  void flushBufferConditionally(bool writeToFile);

public:
  QueryLoggingSolver(Solver *_solver, std::string path,
                     const std::string &commentSign, int queryTimeToLog);

  virtual ~QueryLoggingSolver();

  /// implementation of the SolverImpl interface
  bool computeTruth(const Query &query, bool &isValid);
  bool computeValidity(const Query &query, Solver::Validity &result);
  bool computeValue(const Query &query, ref<Expr> &result);
  bool computeInitialValues(const Query &query,
                            const std::vector<const Array *> &objects,
                            std::vector<std::vector<unsigned char> > &values,
                            bool &hasSolution);
  SolverRunStatus getOperationStatusCode();
  char *getConstraintLog(const Query &);
  void setCoreSolverTimeout(double timeout);
};

#endif /* KLEE_QUERYLOGGINGSOLVER_H */
