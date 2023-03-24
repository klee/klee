//===-- ConstructSolverChain.cpp ------------------------------------++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

/*
 * This file groups declarations that are common to both KLEE and Kleaver.
 */

#include "klee/Solver/Common.h"
#include "klee/Solver/SolverCmdLine.h"
#include "klee/Support/ErrorHandling.h"
#include "klee/System/Time.h"

#include "llvm/Support/raw_ostream.h"

#include <memory>
#include <utility>

namespace klee {
std::unique_ptr<Solver> constructSolverChain(
    std::unique_ptr<Solver> coreSolver, std::string querySMT2LogPath,
    std::string baseSolverQuerySMT2LogPath, std::string queryKQueryLogPath,
    std::string baseSolverQueryKQueryLogPath) {
  Solver *rawCoreSolver = coreSolver.get();
  std::unique_ptr<Solver> solver = std::move(coreSolver);
  const time::Span minQueryTimeToLog(MinQueryTimeToLog);

  if (QueryLoggingOptions.isSet(SOLVER_KQUERY)) {
    solver = createKQueryLoggingSolver(std::move(solver),
                                       baseSolverQueryKQueryLogPath,
                                       minQueryTimeToLog, LogTimedOutQueries);
    klee_message("Logging queries that reach solver in .kquery format to %s\n",
                 baseSolverQueryKQueryLogPath.c_str());
  }

  if (QueryLoggingOptions.isSet(SOLVER_SMTLIB)) {
    solver =
        createSMTLIBLoggingSolver(std::move(solver), baseSolverQuerySMT2LogPath,
                                  minQueryTimeToLog, LogTimedOutQueries);
    klee_message("Logging queries that reach solver in .smt2 format to %s\n",
                 baseSolverQuerySMT2LogPath.c_str());
  }

  if (UseAssignmentValidatingSolver)
    solver = createAssignmentValidatingSolver(std::move(solver));

  if (UseFastCexSolver)
    solver = createFastCexSolver(std::move(solver));

  if (UseCexCache)
    solver = createCexCachingSolver(std::move(solver));

  if (UseBranchCache)
    solver = createCachingSolver(std::move(solver));

  if (UseIndependentSolver)
    solver = createIndependentSolver(std::move(solver));

  if (DebugValidateSolver)
    solver = createValidatingSolver(std::move(solver), rawCoreSolver, false);

  if (QueryLoggingOptions.isSet(ALL_KQUERY)) {
    solver = createKQueryLoggingSolver(std::move(solver), queryKQueryLogPath,
                                       minQueryTimeToLog, LogTimedOutQueries);
    klee_message("Logging all queries in .kquery format to %s\n",
                 queryKQueryLogPath.c_str());
  }

  if (QueryLoggingOptions.isSet(ALL_SMTLIB)) {
    solver = createSMTLIBLoggingSolver(std::move(solver), querySMT2LogPath,
                                       minQueryTimeToLog, LogTimedOutQueries);
    klee_message("Logging all queries in .smt2 format to %s\n",
                 querySMT2LogPath.c_str());
  }
  if (DebugCrossCheckCoreSolverWith != NO_SOLVER) {
    std::unique_ptr<Solver> oracleSolver =
        createCoreSolver(DebugCrossCheckCoreSolverWith);
    solver =
        createValidatingSolver(std::move(solver), oracleSolver.release(), true);
  }

  return solver;
}
} // namespace klee
