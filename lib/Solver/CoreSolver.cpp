//===-- CoreSolver.cpp ------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "MetaSMTSolver.h"
#include "STPSolver.h"
#include "Z3Solver.h"

#include "klee/Solver/Solver.h"
#include "klee/Solver/SolverCmdLine.h"
#include "klee/Support/ErrorHandling.h"

#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

#include <string>

namespace klee {

Solver *createCoreSolver(CoreSolverType cst) {
  switch (cst) {
  case STP_SOLVER:
#ifdef ENABLE_STP
    klee_message("Using STP solver backend");
    if (ProduceUnsatCore) {
      ProduceUnsatCore = false;
      klee_message(
          "Unsat cores are only supported by Z3, disabling unsat cores.");
    }
    return new STPSolver(UseForkedCoreSolver, CoreSolverOptimizeDivides);
#else
    klee_message("Not compiled with STP support");
    return NULL;
#endif
  case METASMT_SOLVER:
#ifdef ENABLE_METASMT
    ProduceUnsatCore = false;
    klee_message("Using MetaSMT solver backend");
    if (ProduceUnsatCore) {
      ProduceUnsatCore = false;
      klee_message(
          "Unsat cores are only supported by Z3, disabling unsat cores.");
    }
    return createMetaSMTSolver();
#else
    klee_message("Not compiled with MetaSMT support");
    return NULL;
#endif
  case DUMMY_SOLVER:
    return createDummySolver();
  case Z3_SOLVER:
#ifdef ENABLE_Z3
    klee_message("Using Z3 solver backend");
#ifdef ENABLE_FP
    klee_message("Using Z3 bitvector builder");
    return new Z3Solver(KLEE_BITVECTOR);
#else
    klee_message("Using Z3 core builder");
    return new Z3Solver(KLEE_CORE);
#endif
#else
    klee_message("Not compiled with Z3 support");
    return NULL;
#endif
  case NO_SOLVER:
    klee_message("Invalid solver");
    return NULL;
  default:
    llvm_unreachable("Unsupported CoreSolverType");
  }
}
} // namespace klee
