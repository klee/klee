//===-- ConstructSolverChain.cpp ------------------------------------------------*- C++ -*-===//
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
#include "llvm/Support/raw_ostream.h"

namespace klee
{
        Solver *constructSolverChain(Solver *coreSolver,
                                     std::string querySMT2LogPath,
                                     std::string baseSolverQuerySMT2LogPath,
                                     std::string queryPCLogPath,
                                     std::string baseSolverQueryPCLogPath)
	{
	  Solver *solver = coreSolver;

	  if (optionIsSet(queryLoggingOptions, SOLVER_PC))
	  {
		solver = createPCLoggingSolver(solver,
					       baseSolverQueryPCLogPath,
					       MinQueryTimeToLog);
		llvm::errs() << "Logging queries that reach solver in .pc format to "
			  << baseSolverQueryPCLogPath.c_str() << "\n";
	  }

	  if (optionIsSet(queryLoggingOptions, SOLVER_SMTLIB))
	  {
		solver = createSMTLIBLoggingSolver(solver,
						   baseSolverQuerySMT2LogPath,
						   MinQueryTimeToLog);
		llvm::errs() << "Logging queries that reach solver in .smt2 format to "
			  << baseSolverQuerySMT2LogPath.c_str() << "\n";
	  }

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

	  if (optionIsSet(queryLoggingOptions, ALL_PC))
	  {
		solver = createPCLoggingSolver(solver,
					       queryPCLogPath,
					       MinQueryTimeToLog);
		llvm::errs() << "Logging all queries in .pc format to "
			  << queryPCLogPath.c_str() << "\n";
	  }

	  if (optionIsSet(queryLoggingOptions, ALL_SMTLIB))
	  {
		solver = createSMTLIBLoggingSolver(solver,querySMT2LogPath,
						   MinQueryTimeToLog);
		llvm::errs() << "Logging all queries in .smt2 format to "
			  << querySMT2LogPath.c_str() << "\n";
	  }

	  return solver;
	}

}


