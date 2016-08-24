/*
 * This header groups command line options declarations and associated data
 * that is common for klee and kleaver.
 */

#ifndef KLEE_COMMANDLINE_H
#define KLEE_COMMANDLINE_H

#include "llvm/Support/CommandLine.h"
#include "klee/Config/config.h"

#ifdef SUPPORT_Z3
#ifdef SUPPORT_STP
#define INTERPOLATION_ENABLED (SelectSolver == SOLVER_Z3 && !NoInterpolation)
#else
#define INTERPOLATION_ENABLED (!NoInterpolation)
#endif
#define OUTPUT_INTERPOLATION_TREE (INTERPOLATION_ENABLED &&OutputTree)
#else
#define INTERPOLATION_ENABLED false
#define OUTPUT_INTERPOLATION_TREE false
#endif

namespace klee {

extern llvm::cl::opt<bool> UseFastCexSolver;

extern llvm::cl::opt<bool> UseCexCache;

extern llvm::cl::opt<bool> UseCache;

extern llvm::cl::opt<bool> UseIndependentSolver; 

extern llvm::cl::opt<bool> DebugValidateSolver;
  
extern llvm::cl::opt<int> MinQueryTimeToLog;

extern llvm::cl::opt<double> MaxCoreSolverTime;

extern llvm::cl::opt<bool> UseForkedCoreSolver;

extern llvm::cl::opt<bool> CoreSolverOptimizeDivides;

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

#if defined(SUPPORT_STP) && defined(SUPPORT_Z3)
enum SolverType
{
	SOLVER_Z3,
	SOLVER_STP
};

extern llvm::cl::opt<SolverType> SelectSolver;
#endif

// We should compile in this option even when SUPPORT_Z3
// was undefined to avoid regression test failure.
extern llvm::cl::opt<bool> NoInterpolation;

#ifdef SUPPORT_Z3
extern llvm::cl::opt<bool> OutputTree;

extern llvm::cl::opt<bool> InterpolationStat;

extern llvm::cl::opt<bool> NoExistential;

extern llvm::cl::opt<int> MaxFailSubsumption;

#endif

#ifdef SUPPORT_METASMT

enum MetaSMTBackendType
{
    METASMT_BACKEND_NONE,
    METASMT_BACKEND_STP,
    METASMT_BACKEND_Z3,
    METASMT_BACKEND_BOOLECTOR
};

extern llvm::cl::opt<klee::MetaSMTBackendType> UseMetaSMT;

#endif /* SUPPORT_METASMT */

//A bit of ugliness so we can use cl::list<> like cl::bits<>, see queryLoggingOptions
template <typename T>
static bool optionIsSet(llvm::cl::list<T> list, T option)
{
    return std::find(list.begin(), list.end(), option) != list.end();
}

}

#endif	/* KLEE_COMMANDLINE_H */

