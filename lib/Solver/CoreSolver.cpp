//===-- CoreSolver.cpp ------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "STPSolver.h"
#include "Z3Solver.h"
#include "MetaSMTSolver.h"

#include "klee/Solver/SolverCmdLine.h"
#include "klee/Internal/Support/ErrorHandling.h"
#include "klee/Solver/Solver.h"

#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

#include <string>

namespace klee {

Solver *createCoreSolver(CoreSolverType cst) {
  switch (cst) {
  case STP_SOLVER:
#ifdef ENABLE_STP
    klee_message("Using STP solver backend");
    return new STPSolver(UseForkedCoreSolver, CoreSolverOptimizeDivides);
#else
    klee_message("Not compiled with STP support");
    return NULL;
#endif
  case METASMT_SOLVER:
#ifdef ENABLE_METASMT
    klee_message("Using MetaSMT solver backend");
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
    return new Z3Solver();
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
}
