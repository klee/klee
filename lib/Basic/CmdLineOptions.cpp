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
#include "klee/Config/Version.h"

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
                 llvm::cl::desc("Optimize constant divides into add/shift/multiplies before passing to core SMT solver (default=off)"),
                 llvm::cl::init(false));


/* Using cl::list<> instead of cl::bits<> results in quite a bit of ugliness when it comes to checking
 * if an option is set. Unfortunately with gcc4.7 cl::bits<> is broken with LLVM2.9 and I doubt everyone
 * wants to patch their copy of LLVM just for these options.
 */
llvm::cl::list<QueryLoggingSolverType> queryLoggingOptions(
    "use-query-log",
    llvm::cl::desc("Log queries to a file. Multiple options can be specified separated by a comma. By default nothing is logged."),
    llvm::cl::values(
        clEnumValN(ALL_KQUERY,"all:kquery","All queries in .kquery (KQuery) format"),
        clEnumValN(ALL_SMTLIB,"all:smt2","All queries in .smt2 (SMT-LIBv2) format"),
        clEnumValN(SOLVER_KQUERY,"solver:kquery","All queries reaching the solver in .kquery (KQuery) format"),
        clEnumValN(SOLVER_SMTLIB,"solver:smt2","All queries reaching the solver in .smt2 (SMT-LIBv2) format")
        KLEE_LLVM_CL_VAL_END),
    llvm::cl::CommaSeparated
);

llvm::cl::opt<bool>
    UseAssignmentValidatingSolver("debug-assignment-validating-solver",
                                  llvm::cl::init(false));

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
    llvm::cl::desc("Specify the MetaSMT solver backend type " METASMT_DEFAULT_BACKEND_STR),
    llvm::cl::values(
        clEnumValN(METASMT_BACKEND_STP, "stp", "Use metaSMT with STP"),
        clEnumValN(METASMT_BACKEND_Z3, "z3", "Use metaSMT with Z3"),
        clEnumValN(METASMT_BACKEND_BOOLECTOR, "btor",
                   "Use metaSMT with Boolector")
        KLEE_LLVM_CL_VAL_END),
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
                     clEnumValN(METASMT_SOLVER, "metasmt", "metaSMT" METASMT_IS_DEFAULT_STR),
                     clEnumValN(DUMMY_SOLVER, "dummy", "Dummy solver"),
                     clEnumValN(Z3_SOLVER, "z3", "Z3" Z3_IS_DEFAULT_STR)
                     KLEE_LLVM_CL_VAL_END),
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
                                "Do not cross check (default)")
                     KLEE_LLVM_CL_VAL_END),
    llvm::cl::init(NO_SOLVER));
}
#undef STP_IS_DEFAULT_STR
#undef METASMT_IS_DEFAULT_STR
#undef Z3_IS_DEFAULT_STR
#undef DEFAULT_CORE_SOLVER




