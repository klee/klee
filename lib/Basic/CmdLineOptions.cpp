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

llvm::cl::opt<bool>
CoreSolverOptimizeDivides("solver-optimize-divides", 
                 llvm::cl::desc("Optimize constant divides into add/shift/multiplies before passing to core SMT solver (default=on)"),
                 llvm::cl::init(true));


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
        clEnumValN(SOLVER_SMTLIB,"solver:smt2","All queries reaching the solver in .smt2 (SMT-LIBv2) format"),
        clEnumValEnd
	),
    llvm::cl::CommaSeparated
);

#if defined(SUPPORT_STP) && defined(SUPPORT_Z3)
llvm::cl::opt<SolverType>
SelectSolver("select-solver",
             llvm::cl::desc("Select solver to use with z3 as the default."),
             llvm::cl::values(clEnumValN(SOLVER_STP, "stp", "Use STP solver"),
                              clEnumValN(SOLVER_Z3, "z3", "Use Z3 solver"),
                              clEnumValEnd));
#endif

// We should compile in this option even when SUPPORT_Z3
// was undefined to avoid regression test failure.
llvm::cl::opt<bool> NoInterpolation(
    "no-interpolation",
    llvm::cl::desc("Disable interpolation for search space reduction. "
                   "Interpolation is enabled by default when Z3 was the solver "
                   "used. This option has no effect when Z3 was not used."));

#ifdef SUPPORT_Z3
llvm::cl::opt<bool> OutputTree(
    "output-tree",
    llvm::cl::desc("Outputs tree.dot: the execution tree in .dot file "
                   "format. At present, this feature is only available when "
                   "Z3 is compiled in and interpolation is enabled."));

llvm::cl::opt<bool> InterpolationStat(
    "interpolation-stat",
    llvm::cl::desc(
        "Displays an execution profile of the interpolation routines."));

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
    "max-fail-subsume",
    llvm::cl::desc(
        "To set the maximum "
        "number of fail subsumption check. When this options is "
        "selected, the number of subsumption table entries is equal to "
        "the number of max-fail-subsume. When the number of entries "
        "are more than specified max-fail-subsume, the oldest entry "
        "will be deleted (default=0 (off))"),
    llvm::cl::init(0));

#endif

#ifdef SUPPORT_METASMT

llvm::cl::opt<klee::MetaSMTBackendType>
UseMetaSMT("use-metasmt",
           llvm::cl::desc("Use MetaSMT as an underlying SMT solver and specify the solver backend type."),
           llvm::cl::values(clEnumValN(METASMT_BACKEND_NONE, "none", "Don't use metaSMT"),
                      clEnumValN(METASMT_BACKEND_STP, "stp", "Use metaSMT with STP"),
                      clEnumValN(METASMT_BACKEND_Z3, "z3", "Use metaSMT with Z3"),
                      clEnumValN(METASMT_BACKEND_BOOLECTOR, "btor", "Use metaSMT with Boolector"),
                      clEnumValEnd),  
           llvm::cl::init(METASMT_BACKEND_NONE));

#endif /* SUPPORT_METASMT */

}




