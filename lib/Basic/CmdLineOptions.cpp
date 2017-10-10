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

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/Support/CommandLine.h"

using namespace llvm;

namespace klee {

cl::opt<bool>
UseFastCexSolver("use-fast-cex-solver",
		 cl::init(false),
		 cl::desc("(default=off)"));

cl::opt<bool>
UseCexCache("use-cex-cache",
            cl::init(true),
            cl::desc("Use counterexample caching (default=on)"));

cl::opt<bool>
UseCache("use-cache",
         cl::init(true),
         cl::desc("Use validity caching (default=on)"));

cl::opt<bool>
UseIndependentSolver("use-independent-solver",
                     cl::init(true),
                     cl::desc("Use constraint independence (default=on)"));

cl::opt<bool>
DebugValidateSolver("debug-validate-solver",
                    cl::init(false));
  
cl::opt<int>
MinQueryTimeToLog("min-query-time-to-log",
                  cl::init(0),
                  cl::value_desc("milliseconds"),
                  cl::desc("Set time threshold (in ms) for queries logged in files. "
                           "Only queries longer than threshold will be logged. (default=0). "
                           "Set this param to a negative value to log timeouts only."));

cl::opt<double>
MaxCoreSolverTime("max-solver-time",
                  cl::desc("Maximum amount of time for a single SMT query (default=0s (off)). Enables --use-forked-solver"),
                  cl::init(0.0),
                  cl::value_desc("seconds"));

cl::opt<bool>
UseForkedCoreSolver("use-forked-solver",
                    cl::desc("Run the core SMT solver in a forked process (default=on)"),
                    cl::init(true));

cl::opt<bool>
CoreSolverOptimizeDivides("solver-optimize-divides", 
                          cl::desc("Optimize constant divides into add/shift/multiplies before passing to core SMT solver (default=off)"),
                          cl::init(false));

cl::bits<QueryLoggingSolverType>
queryLoggingOptions("use-query-log",
                    cl::desc("Log queries to a file. Multiple options can be specified separated by a comma. By default nothing is logged."),
                    cl::values(clEnumValN(ALL_KQUERY,"all:kquery","All queries in .kquery (KQuery) format"),
                               clEnumValN(ALL_SMTLIB,"all:smt2","All queries in .smt2 (SMT-LIBv2) format"),
                               clEnumValN(SOLVER_KQUERY,"solver:kquery","All queries reaching the solver in .kquery (KQuery) format"),
                               clEnumValN(SOLVER_SMTLIB,"solver:smt2","All queries reaching the solver in .smt2 (SMT-LIBv2) format")
                               KLEE_LLVM_CL_VAL_END),
                    cl::CommaSeparated);

cl::opt<bool>
UseAssignmentValidatingSolver("debug-assignment-validating-solver",
                              cl::init(false));

void KCommandLine::HideUnrelatedOptions(cl::OptionCategory &Category) {
#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 7)
  StringMap<cl::Option *> &map = cl::getRegisteredOptions();
#else
  StringMap<cl::Option *> map;
  cl::getRegisteredOptions(map);
#endif
  for (StringMap<cl::Option *>::iterator i = map.begin(), e = map.end(); i != e;
       i++) {
    if (i->second->Category != &Category) {
      i->second->setHiddenFlag(cl::Hidden);
    }
  }
}

void KCommandLine::HideUnrelatedOptions(
    ArrayRef<const cl::OptionCategory *> Categories) {
  for (ArrayRef<const cl::OptionCategory *>::iterator i = Categories.begin(),
                                                      e = Categories.end();
       i != e; i++)
    HideUnrelatedOptions(*i);
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
               cl::init(METASMT_DEFAULT_BACKEND));

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

cl::opt<CoreSolverType>
CoreSolverToUse("solver-backend", cl::desc("Specifiy the core solver backend to use"),
                cl::values(clEnumValN(STP_SOLVER, "stp", "stp" STP_IS_DEFAULT_STR),
                           clEnumValN(METASMT_SOLVER, "metasmt", "metaSMT" METASMT_IS_DEFAULT_STR),
                           clEnumValN(DUMMY_SOLVER, "dummy", "Dummy solver"),
                           clEnumValN(Z3_SOLVER, "z3", "Z3" Z3_IS_DEFAULT_STR)
                           KLEE_LLVM_CL_VAL_END),
                cl::init(DEFAULT_CORE_SOLVER));

cl::opt<CoreSolverType>
DebugCrossCheckCoreSolverWith("debug-crosscheck-core-solver",
                              cl::desc("Specifiy a solver to use for cross checking with the core solver"),
                              cl::values(clEnumValN(STP_SOLVER, "stp", "stp"),
                                         clEnumValN(METASMT_SOLVER, "metasmt", "metaSMT"),
                                         clEnumValN(DUMMY_SOLVER, "dummy", "Dummy solver"),
                                         clEnumValN(Z3_SOLVER, "z3", "Z3"),
                                         clEnumValN(NO_SOLVER, "none",
                                                    "Do not cross check (default)")
                                         KLEE_LLVM_CL_VAL_END),
                              cl::init(NO_SOLVER));
}

#undef STP_IS_DEFAULT_STR
#undef METASMT_IS_DEFAULT_STR
#undef Z3_IS_DEFAULT_STR
#undef DEFAULT_CORE_SOLVER
