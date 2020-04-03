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

#include "klee/Solver/SolverCmdLine.h"

#include "klee/Config/Version.h"
#include "klee/Support/OptionCategories.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/Support/CommandLine.h"

using namespace llvm;

namespace klee {

cl::extrahelp TimeFormatInfo(
    "\nTime format used by KLEE's options\n"
    "\n"
    "  Time spans can be specified in two ways:\n"
    "    1. As positive real numbers representing seconds, e.g. '10', '3.5' "
    "but not 'INF', 'NaN', '1e3', '-4.5s'\n"
    "    2. As a sequence of natural numbers with specified units, e.g. "
    "'1h10min' (= '70min'), '5min10s' but not '3.5min', '8S'\n"
    "       The following units are supported: h, min, s, ms, us, ns.\n");

cl::OptionCategory SolvingCat("Constraint solving options",
                              "These options impact constraint solving.");

cl::opt<bool> UseFastCexSolver(
    "use-fast-cex-solver", cl::init(false),
    cl::desc("Enable an experimental range-based solver (default=false)"),
    cl::cat(SolvingCat));

cl::opt<bool> UseCexCache("use-cex-cache", cl::init(true),
                          cl::desc("Use the counterexample cache (default=true)"),
                          cl::cat(SolvingCat));

cl::opt<bool> UseBranchCache("use-branch-cache", cl::init(true),
                             cl::desc("Use the branch cache (default=true)"),
                             cl::cat(SolvingCat));

cl::opt<bool>
    UseIndependentSolver("use-independent-solver", cl::init(true),
                         cl::desc("Use constraint independence (default=true)"),
                         cl::cat(SolvingCat));

cl::opt<bool> DebugValidateSolver(
    "debug-validate-solver", cl::init(false),
    cl::desc("Crosscheck the results of the solver chain above the core solver "
             "with the results of the core solver (default=false)"),
    cl::cat(SolvingCat));

cl::opt<std::string> MinQueryTimeToLog(
    "min-query-time-to-log",
    cl::desc("Set time threshold for queries logged in files. "
             "Only queries longer than threshold will be logged. (default=0s)"),
    cl::cat(SolvingCat));

cl::opt<bool>
    LogTimedOutQueries("log-timed-out-queries", cl::init(true),
                       cl::desc("Log queries that timed out. (default=true)."),
                       cl::cat(SolvingCat));

cl::opt<std::string> MaxCoreSolverTime(
    "max-solver-time",
    cl::desc("Maximum amount of time for a single SMT query (default=0s (off)). "
             "Enables --use-forked-solver"),
    cl::cat(SolvingCat));

cl::opt<bool> UseForkedCoreSolver(
    "use-forked-solver",
    cl::desc("Run the core SMT solver in a forked process (default=true)"),
    cl::init(true), cl::cat(SolvingCat));

cl::opt<bool> CoreSolverOptimizeDivides(
    "solver-optimize-divides",
    cl::desc("Optimize constant divides into add/shift/multiplies before "
             "passing them to the core SMT solver (default=false)"),
    cl::init(false), cl::cat(SolvingCat));

cl::bits<QueryLoggingSolverType> QueryLoggingOptions(
    "use-query-log",
    cl::desc("Log queries to a file. Multiple options can be specified "
             "separated by a comma. By default nothing is logged."),
    cl::values(
        clEnumValN(ALL_KQUERY, "all:kquery",
                   "All queries in .kquery (KQuery) format"),
        clEnumValN(ALL_SMTLIB, "all:smt2",
                   "All queries in .smt2 (SMT-LIBv2) format"),
        clEnumValN(
            SOLVER_KQUERY, "solver:kquery",
            "All queries reaching the solver in .kquery (KQuery) format"),
        clEnumValN(
            SOLVER_SMTLIB, "solver:smt2",
            "All queries reaching the solver in .smt2 (SMT-LIBv2) format")
            KLEE_LLVM_CL_VAL_END),
    cl::CommaSeparated, cl::cat(SolvingCat));

cl::opt<bool> UseAssignmentValidatingSolver(
    "debug-assignment-validating-solver", cl::init(false),
    cl::desc("Debug the correctness of generated assignments (default=false)"),
    cl::cat(SolvingCat));


void KCommandLine::HideOptions(llvm::cl::OptionCategory &Category) {
  StringMap<cl::Option *> &map = cl::getRegisteredOptions();

  for (auto &elem : map) {
#if LLVM_VERSION_CODE >= LLVM_VERSION(9, 0)
    for (auto &cat : elem.second->Categories) {
#else
    {
      auto &cat = elem.second->Category;
#endif
      if (cat == &Category) {
        elem.second->setHiddenFlag(cl::Hidden);
      }
    }
  }
}

#ifdef ENABLE_METASMT

#ifdef METASMT_DEFAULT_BACKEND_IS_BTOR
#define METASMT_DEFAULT_BACKEND_STR "(default = btor)."
#define METASMT_DEFAULT_BACKEND METASMT_BACKEND_BOOLECTOR
#elif METASMT_DEFAULT_BACKEND_IS_Z3
#define METASMT_DEFAULT_BACKEND_STR "(default = z3)."
#define METASMT_DEFAULT_BACKEND METASMT_BACKEND_Z3
#elif METASMT_DEFAULT_BACKEND_IS_CVC4
#define METASMT_DEFAULT_BACKEND_STR "(default = cvc4)."
#define METASMT_DEFAULT_BACKEND METASMT_BACKEND_CVC4
#elif METASMT_DEFAULT_BACKEND_IS_YICES2
#define METASMT_DEFAULT_BACKEND_STR "(default = yices2)."
#define METASMT_DEFAULT_BACKEND METASMT_BACKEND_YICES2
#else
#define METASMT_DEFAULT_BACKEND_STR "(default = stp)."
#define METASMT_DEFAULT_BACKEND METASMT_BACKEND_STP
#endif

cl::opt<klee::MetaSMTBackendType>
MetaSMTBackend("metasmt-backend",
               cl::desc("Specify the MetaSMT solver backend type " METASMT_DEFAULT_BACKEND_STR),
               cl::values(clEnumValN(METASMT_BACKEND_STP, "stp", "Use metaSMT with STP"),
                          clEnumValN(METASMT_BACKEND_Z3, "z3", "Use metaSMT with Z3"),
                          clEnumValN(METASMT_BACKEND_BOOLECTOR, "btor",
                                     "Use metaSMT with Boolector"),
                          clEnumValN(METASMT_BACKEND_CVC4, "cvc4", "Use metaSMT with CVC4"),
                          clEnumValN(METASMT_BACKEND_YICES2, "yices2", "Use metaSMT with Yices2")
                          KLEE_LLVM_CL_VAL_END),
               cl::init(METASMT_DEFAULT_BACKEND),
               cl::cat(SolvingCat));

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

cl::opt<CoreSolverType> CoreSolverToUse(
    "solver-backend", cl::desc("Specifiy the core solver backend to use"),
    cl::values(clEnumValN(STP_SOLVER, "stp", "STP" STP_IS_DEFAULT_STR),
               clEnumValN(METASMT_SOLVER, "metasmt",
                          "metaSMT" METASMT_IS_DEFAULT_STR),
               clEnumValN(DUMMY_SOLVER, "dummy", "Dummy solver"),
               clEnumValN(Z3_SOLVER, "z3", "Z3" Z3_IS_DEFAULT_STR)
                   KLEE_LLVM_CL_VAL_END),
    cl::init(DEFAULT_CORE_SOLVER), cl::cat(SolvingCat));

cl::opt<CoreSolverType> DebugCrossCheckCoreSolverWith(
    "debug-crosscheck-core-solver",
    cl::desc(
        "Specifiy a solver to use for crosschecking the results of the core solver"),
    cl::values(clEnumValN(STP_SOLVER, "stp", "STP"),
               clEnumValN(METASMT_SOLVER, "metasmt", "metaSMT"),
               clEnumValN(DUMMY_SOLVER, "dummy", "Dummy solver"),
               clEnumValN(Z3_SOLVER, "z3", "Z3"),
               clEnumValN(NO_SOLVER, "none", "Do not crosscheck (default)")
                   KLEE_LLVM_CL_VAL_END),
    cl::init(NO_SOLVER), cl::cat(SolvingCat));
} // namespace klee

#undef STP_IS_DEFAULT_STR
#undef METASMT_IS_DEFAULT_STR
#undef Z3_IS_DEFAULT_STR
#undef DEFAULT_CORE_SOLVER
