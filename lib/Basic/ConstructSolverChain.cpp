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
#include "klee/Common.h"
#include "klee/CommandLine.h"
#include "klee/Internal/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

namespace klee {
Solver *constructSolverChain(Solver *coreSolver,
                             std::string querySMT2LogPath,
                             std::string baseSolverQuerySMT2LogPath,
                             std::string queryKQueryLogPath,
                             std::string baseSolverQueryKQueryLogPath) {
  Solver *solver = coreSolver;

  if (optionIsSet(queryLoggingOptions, SOLVER_KQUERY)) {
    solver = createKQueryLoggingSolver(solver, baseSolverQueryKQueryLogPath,
                                   MinQueryTimeToLog);
    klee_message("Logging queries that reach solver in .kquery format to %s\n",
                 baseSolverQueryKQueryLogPath.c_str());
  }

  if (optionIsSet(queryLoggingOptions, SOLVER_SMTLIB)) {
    solver = createSMTLIBLoggingSolver(solver, baseSolverQuerySMT2LogPath,
                                       MinQueryTimeToLog);
    klee_message("Logging queries that reach solver in .smt2 format to %s\n",
                 baseSolverQuerySMT2LogPath.c_str());
  }

  if (UseAssignmentValidatingSolver)
    solver = createAssignmentValidatingSolver(solver);

  if (UseFastCexSolver)
    solver = createFastCexSolver(solver);

  if (UseCexCache)
    solver = createCexCachingSolver(solver);

  if (UseCache)
    solver = createCachingSolver(solver);

  if (UseIndependentSolver)
    solver = createIndependentSolver(solver);

  if (DebugValidateSolver)
    solver = createValidatingSolver(solver, coreSolver);

  if (optionIsSet(queryLoggingOptions, ALL_KQUERY)) {
    solver = createKQueryLoggingSolver(solver, queryKQueryLogPath,
                                       MinQueryTimeToLog);
    klee_message("Logging all queries in .kquery format to %s\n",
                 queryKQueryLogPath.c_str());
  }

  if (optionIsSet(queryLoggingOptions, ALL_SMTLIB)) {
    solver =
        createSMTLIBLoggingSolver(solver, querySMT2LogPath, MinQueryTimeToLog);
    klee_message("Logging all queries in .smt2 format to %s\n",
                 querySMT2LogPath.c_str());
  }
  if (DebugCrossCheckCoreSolverWith != NO_SOLVER) {
    Solver *oracleSolver = createCoreSolver(DebugCrossCheckCoreSolverWith);
    solver = createValidatingSolver(/*s=*/solver, /*oracle=*/oracleSolver);
  }

  return solver;
}
}
