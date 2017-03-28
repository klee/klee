//===-- CmdLineOptions.cpp --------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

/*
 * This file groups command line options definitions and associated
 * data that are common to both KLEE and Kleaver.
 */

#include "klee/CommandLine.h"

namespace klee {

llvm::cl::opt<bool>
UseFastCexSolver("use-fast-cex-solver",
		 llvm::cl::init(false),
		 llvm::cl::desc("(default=off)"));

llvm::cl::opt<bool>
UseCexCache("use-cex-cache",
            llvm::cl::init(true),
            llvm::cl::desc("Use counterexample caching (default=on)"));

llvm::cl::opt<bool>
UseCache("use-cache",
         llvm::cl::init(true),
         llvm::cl::desc("Use validity caching (default=on)"));

llvm::cl::opt<bool>
UseIndependentSolver("use-independent-solver",
                     llvm::cl::init(true),
                     llvm::cl::desc("Use constraint independence (default=on)"));

llvm::cl::opt<bool>
DebugValidateSolver("debug-validate-solver",
		             llvm::cl::init(false));
  
llvm::cl::opt<int>
MinQueryTimeToLog("min-query-time-to-log",
                  llvm::cl::init(0),
                  llvm::cl::value_desc("milliseconds"),
                  llvm::cl::desc("Set time threshold (in ms) for queries logged in files. "
                                 "Only queries longer than threshold will be logged. (default=0). "
                                 "Set this param to a negative value to log timeouts only."));

llvm::cl::opt<double>
MaxCoreSolverTime("max-solver-time",
           llvm::cl::desc("Maximum amount of time for a single SMT query (default=0s (off)). Enables --use-forked-solver"),
           llvm::cl::init(0.0),
           llvm::cl::value_desc("seconds"));

llvm::cl::opt<bool>
UseForkedCoreSolver("use-forked-solver",
             llvm::cl::desc("Run the core SMT solver in a forked process (default=on)"),
             llvm::cl::init(true));

llvm::cl::opt<bool> CoreSolverOptimizeDivides(
    "solver-optimize-divides",
    llvm::cl::desc("Optimize constant divides into add/shift/multiplies before "
                   "passing to core SMT solver (default=off)"),
    llvm::cl::init(false));

/* Using cl::list<> instead of cl::bits<> results in quite a bit of ugliness when it comes to checking
 * if an option is set. Unfortunately with gcc4.7 cl::bits<> is broken with LLVM2.9 and I doubt everyone
 * wants to patch their copy of LLVM just for these options.
 */
llvm::cl::list<QueryLoggingSolverType> queryLoggingOptions(
    "use-query-log",
    llvm::cl::desc("Log queries to a file. Multiple options can be specified "
                   "separated by a comma. By default nothing is logged."),
    llvm::cl::values(
        clEnumValN(ALL_PC, "all:pc", "All queries in .pc (KQuery) format"),
        clEnumValN(ALL_SMTLIB, "all:smt2",
                   "All queries in .smt2 (SMT-LIBv2) format"),
        clEnumValN(SOLVER_PC, "solver:pc",
                   "All queries reaching the solver in .pc (KQuery) format"),
        clEnumValN(
            SOLVER_SMTLIB, "solver:smt2",
            "All queries reaching the solver in .smt2 (SMT-LIBv2) format"),
        clEnumValEnd),
    llvm::cl::CommaSeparated);

// We should compile in this option even when ENABLE_Z3
// was undefined to avoid regression test failure.
llvm::cl::opt<bool> NoInterpolation(
    "no-interpolation",
    llvm::cl::desc("Disable interpolation for search space reduction. "
                   "Interpolation is enabled by default when Z3 was the solver "
                   "used. This option has no effect when Z3 was not used."));

#ifdef ENABLE_Z3
llvm::cl::opt<bool> OutputTree(
    "output-tree",
    llvm::cl::desc("Outputs tree.dot: the execution tree in .dot file "
                   "format. At present, this feature is only available when "
                   "Z3 is compiled in and interpolation is enabled."));

llvm::cl::opt<bool> InterpolationStat(
    "interpolation-stat",
    llvm::cl::desc(
        "Displays an execution profile of the interpolation routines."));

llvm::cl::opt<bool> NoSubsumedTest(
    "no-subsumed-test",
    llvm::cl::desc("Disables generation of test cases for subsumed paths."),
    llvm::cl::init(false));

llvm::cl::opt<bool> NoExistential(
    "no-existential",
    llvm::cl::desc(
        "This option avoids existential quantification in subsumption "
        "check by equating each existential variable with its corresponding "
        "free variable. For example, when checking if x < 10 is subsumed by "
        "another state where there is x' s.t., x' <= 0 && x = x' + 20 (here "
        "the existential variable x' represents the value of x before adding "
        "20), we strengthen the query by adding the constraint x' = x. This "
        "has an effect of removing all existentially-quantified variables "
        "most solvers are not very powerful at solving, however, at likely "
        "less number of subsumptions due to the strengthening of the query."));

llvm::cl::opt<int> MaxFailSubsumption(
    "max-subsumption-failure",
    llvm::cl::desc("To set the maximum number of failed subsumption check. "
                   "When this options is specified and the number of "
                   "subsumption table entries is more than the specified "
                   "value, the oldest entry will be deleted (default=0 (off))"),
    llvm::cl::init(0));

llvm::cl::opt<int>
DebugState("debug-state",
           llvm::cl::desc("Dump information on symbolic execution state when "
                          "visited (default=0 (off))."),
           llvm::cl::init(0));

llvm::cl::opt<int> DebugSubsumption(
    "debug-subsumption",
    llvm::cl::desc("Set level of debug information on subsumption checks: the "
                   "higher the more (default=0 (off))."),
    llvm::cl::init(0));

llvm::cl::opt<bool> NoBoundInterpolation(
    "no-bound-interpolation",
    llvm::cl::desc("This option disables the generation of interpolant from "
                   "each successful out-of-bound check: It may result in loss "
                   "of error report(s)"));

llvm::cl::opt<bool> ExactAddressInterpolant(
    "exact-address-interpolant",
    llvm::cl::desc("This option uses exact address for interpolating "
                   "successful out-of-bound memory access instead of the "
                   "default memory offset bound. It has no effect when "
                   "-no-bound-interpolation is specified."));

llvm::cl::opt<bool> SpecialFunctionBoundInterpolation(
    "special-function-bound-interpolation",
    llvm::cl::desc("Perform memory bound interpolation only within function "
                   "named tracerx_check."),
    llvm::cl::init(false));
#endif // ENABLE_Z3

#ifdef ENABLE_METASMT

#ifdef METASMT_DEFAULT_BACKEND_IS_BTOR
#define METASMT_DEFAULT_BACKEND_STR "(default = btor)."
#define METASMT_DEFAULT_BACKEND METASMT_BACKEND_BOOLECTOR
#elif METASMT_DEFAULT_BACKEND_IS_Z3
#define METASMT_DEFAULT_BACKEND_STR "(default = z3)."
#define METASMT_DEFAULT_BACKEND METASMT_BACKEND_Z3
#else
#define METASMT_DEFAULT_BACKEND_STR "(default = stp)."
#define METASMT_DEFAULT_BACKEND METASMT_BACKEND_STP
#endif

llvm::cl::opt<klee::MetaSMTBackendType> MetaSMTBackend(
    "metasmt-backend",
    llvm::cl::desc(
        "Specify the MetaSMT solver backend type " METASMT_DEFAULT_BACKEND_STR),
    llvm::cl::values(
        clEnumValN(METASMT_BACKEND_STP, "stp", "Use metaSMT with STP"),
        clEnumValN(METASMT_BACKEND_Z3, "z3", "Use metaSMT with Z3"),
        clEnumValN(METASMT_BACKEND_BOOLECTOR, "btor",
                   "Use metaSMT with Boolector"),
        clEnumValEnd),
    llvm::cl::init(METASMT_DEFAULT_BACKEND));

#undef METASMT_DEFAULT_BACKEND
#undef METASMT_DEFAULT_BACKEND_STR

#endif /* ENABLE_METASMT */

// Pick the default core solver based on configuration
#ifdef ENABLE_STP
#define STP_IS_DEFAULT_STR " (default)"
#define METASMT_IS_DEFAULT_STR ""
#define Z3_IS_DEFAULT_STR ""
#define DEFAULT_CORE_SOLVER STP_SOLVER
#elif ENABLE_Z3
#define STP_IS_DEFAULT_STR ""
#define METASMT_IS_DEFAULT_STR ""
#define Z3_IS_DEFAULT_STR " (default)"
#define DEFAULT_CORE_SOLVER Z3_SOLVER
#elif ENABLE_METASMT
#define STP_IS_DEFAULT_STR ""
#define METASMT_IS_DEFAULT_STR " (default)"
#define Z3_IS_DEFAULT_STR ""
#define DEFAULT_CORE_SOLVER METASMT_SOLVER
#define Z3_IS_DEFAULT_STR ""
#else
#error "Unsupported solver configuration"
#endif
llvm::cl::opt<CoreSolverType> CoreSolverToUse(
    "solver-backend", llvm::cl::desc("Specifiy the core solver backend to use"),
    llvm::cl::values(clEnumValN(STP_SOLVER, "stp", "stp" STP_IS_DEFAULT_STR),
                     clEnumValN(METASMT_SOLVER, "metasmt",
                                "metaSMT" METASMT_IS_DEFAULT_STR),
                     clEnumValN(DUMMY_SOLVER, "dummy", "Dummy solver"),
                     clEnumValN(Z3_SOLVER, "z3", "Z3" Z3_IS_DEFAULT_STR),
                     clEnumValEnd),
    llvm::cl::init(DEFAULT_CORE_SOLVER));

llvm::cl::opt<CoreSolverType> DebugCrossCheckCoreSolverWith(
    "debug-crosscheck-core-solver",
    llvm::cl::desc(
        "Specifiy a solver to use for cross checking with the core solver"),
    llvm::cl::values(clEnumValN(STP_SOLVER, "stp", "stp"),
                     clEnumValN(METASMT_SOLVER, "metasmt", "metaSMT"),
                     clEnumValN(DUMMY_SOLVER, "dummy", "Dummy solver"),
                     clEnumValN(Z3_SOLVER, "z3", "Z3"),
                     clEnumValN(NO_SOLVER, "none",
                                "Do not cross check (default)"),
                     clEnumValEnd),
    llvm::cl::init(NO_SOLVER));
}
#undef STP_IS_DEFAULT_STR
#undef METASMT_IS_DEFAULT_STR
#undef Z3_IS_DEFAULT_STR
#undef DEFAULT_CORE_SOLVER
