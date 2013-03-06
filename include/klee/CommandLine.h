/*
 * This header groups command line options declarations and associated data
 * that is common for klee and kleaver.
 */

#ifndef KLEE_COMMANDLINE_H
#define KLEE_COMMANDLINE_H

#include "llvm/Support/CommandLine.h"

namespace klee {

extern llvm::cl::opt<bool> UseFastCexSolver;

extern llvm::cl::opt<bool> UseCexCache;

extern llvm::cl::opt<bool> UseCache;

extern llvm::cl::opt<bool> UseIndependentSolver; 
  
extern llvm::cl::opt<int> MinQueryTimeToLog;

extern llvm::cl::opt<double> MaxSTPTime;

///The different query logging solvers that can switched on/off
enum QueryLoggingSolverType
{
    ALL_PC,       ///< Log all queries (un-optimised) in .pc (KQuery) format
    ALL_SMTLIB,   ///< Log all queries (un-optimised)  .smt2 (SMT-LIBv2) format
    SOLVER_PC,    ///< Log queries passed to solver (optimised) in .pc (KQuery) format
    SOLVER_SMTLIB ///< Log queries passed to solver (optimised) in .smt2 (SMT-LIBv2) format
};

/* Using cl::list<> instead of cl::bits<> results in quite a bit of ugliness when it comes to checking
 * if an option is set. Unfortunately with gcc4.7 cl::bits<> is broken with LLVM2.9 and I doubt everyone
 * wants to patch their copy of LLVM just for these options.
 */
extern llvm::cl::list<QueryLoggingSolverType> queryLoggingOptions;


//A bit of ugliness so we can use cl::list<> like cl::bits<>, see queryLoggingOptions
template <typename T>
static bool optionIsSet(llvm::cl::list<T> list, T option)
{
    return std::find(list.begin(), list.end(), option) != list.end();
}

}

#endif	/* KLEE_COMMANDLINE_H */

