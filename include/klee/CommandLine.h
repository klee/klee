/*
 * FIXME: This is a temporary solution.
 * This header groups command line options and associated data that is common
 * for klee and kleaver.
 */

#ifndef COMMANDLINE_H
#define	COMMANDLINE_H

#include "llvm/Support/CommandLine.h"

namespace {

#define ALL_QUERIES_SMT2_FILE_NAME      "all-queries.smt2"
#define SOLVER_QUERIES_SMT2_FILE_NAME   "solver-queries.smt2"
#define ALL_QUERIES_PC_FILE_NAME        "all-queries.pc"
#define SOLVER_QUERIES_PC_FILE_NAME     "solver-queries.pc"
    
llvm::cl::opt<bool>
UseFastCexSolver("use-fast-cex-solver",
		 llvm::cl::init(false),
		 llvm::cl::desc("(default=off)"));
  
llvm::cl::opt<int>
MinQueryTimeToLog("min-query-time-to-log",
                  llvm::cl::init(0),
                  llvm::cl::value_desc("milliseconds"),
                  llvm::cl::desc("Set time threshold (in ms) for queries logged in files. "
                                 "Only queries longer than threshold will be logged. (default=0). "
                                 "Set this param to a negative value to log timeouts only."));

///The different query logging solvers that can switched on/off
enum QueryLoggingSolverType
{
    ALL_PC, ///< Log all queries (un-optimised) in .pc (KQuery) format
    ALL_SMTLIB, ///< Log all queries (un-optimised)  .smt2 (SMT-LIBv2) format
    SOLVER_PC, ///< Log queries passed to solver (optimised) in .pc (KQuery) format
    SOLVER_SMTLIB ///< Log queries passed to solver (optimised) in .smt2 (SMT-LIBv2) format
};

/* Using cl::list<> instead of cl::bits<> results in quite a bit of ugliness when it comes to checking
 * if an option is set. Unfortunately with gcc4.7 cl::bits<> is broken with LLVM2.9 and I doubt everyone
 * wants to patch their copy of LLVM just for these options.
 */
llvm::cl::list<QueryLoggingSolverType> queryLoggingOptions(
    "use-query-log",
    llvm::cl::desc("Log queries to a file. Multiple options can be specified seperate by a comma. By default nothing is logged."),
    llvm::cl::values(
        clEnumValN(ALL_PC,"all:pc","All queries in .pc (KQuery) format"),
        clEnumValN(ALL_SMTLIB,"all:smt2","All queries in .smt2 (SMT-LIBv2) format"),
        clEnumValN(SOLVER_PC,"solver:pc","All queries reaching the solver in .pc (KQuery) format"),
        clEnumValN(SOLVER_SMTLIB,"solver:smt2","All queries reaching the solver in .pc (SMT-LIBv2) format"),
        clEnumValEnd
	),
    llvm::cl::CommaSeparated
);

}

namespace klee { 
  //A bit of ugliness so we can use cl::list<> like cl::bits<>, see queryLoggingOptions
  template <typename T>
  static bool optionIsSet(llvm::cl::list<T> list, T option)
  {
	  return std::find(list.begin(), list.end(), option) != list.end();
  }
}

#endif	/* COMMANDLINE_H */

