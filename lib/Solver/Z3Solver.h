//===-- Z3Solver.h ---------------------------------------------*- C++ -*-====//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_Z3SOLVER_H
#define KLEE_Z3SOLVER_H

#include "klee/Solver/Solver.h"

#define Z3_TRUE true
#define Z3_FALSE false

namespace klee {
enum Z3BuilderType { KLEE_CORE, KLEE_BITVECTOR };

/// Z3Solver - A complete solver based on Z3
class Z3Solver : public Solver {
public:
  /// Z3Solver - Construct a new Z3Solver.
  Z3Solver(Z3BuilderType type);
};

class Z3TreeSolver : public Solver {
public:
  Z3TreeSolver(Z3BuilderType type, unsigned maxSolvers);
};

} // namespace klee

#endif /* KLEE_Z3SOLVER_H */
