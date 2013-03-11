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
#include <iostream>
#include "klee/Common.h"
#include "klee/CommandLine.h"

namespace klee
{
	Solver *constructSolverChain(STPSolver *stpSolver,
								 std::string querySMT2LogPath,
								 std::string baseSolverQuerySMT2LogPath,
								 std::string queryPCLogPath,
								 std::string baseSolverQueryPCLogPath)
	{
	  Solver *solver = stpSolver;

	  if (optionIsSet(queryLoggingOptions,SOLVER_PC))
	  {
		solver = createPCLoggingSolver(solver,
									   baseSolverQueryPCLogPath,
							   MinQueryTimeToLog);
		std::cerr << "Logging queries that reach solver in .pc format to " << baseSolverQueryPCLogPath.c_str() << std::endl;
	  }

	  if (optionIsSet(queryLoggingOptions,SOLVER_SMTLIB))
	  {
		solver = createSMTLIBLoggingSolver(solver,baseSolverQuerySMT2LogPath,
										   MinQueryTimeToLog);
		std::cerr << "Logging queries that reach solver in .smt2 format to " << baseSolverQuerySMT2LogPath.c_str() << std::endl;
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
		solver = createValidatingSolver(solver, stpSolver);

	  if (optionIsSet(queryLoggingOptions,ALL_PC))
	  {
		solver = createPCLoggingSolver(solver,
									   queryPCLogPath,
									   MinQueryTimeToLog);
		std::cerr << "Logging all queries in .pc format to " << queryPCLogPath.c_str() << std::endl;
	  }

	  if (optionIsSet(queryLoggingOptions,ALL_SMTLIB))
	  {
		solver = createSMTLIBLoggingSolver(solver,querySMT2LogPath,
										   MinQueryTimeToLog);
		std::cerr << "Logging all queries in .smt2 format to " << querySMT2LogPath.c_str() << std::endl;
	  }

	  return solver;
	}

}


