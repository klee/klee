//===-- STPSolver.h
//---------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_STPSOLVER_H
#define KLEE_STPSOLVER_H

#include "klee/Solver/Solver.h"

namespace klee {
/// STPSolver - A complete solver based on STP.
class STPSolver : public Solver {
public:
  /// STPSolver - Construct a new STPSolver.
  ///
  /// \param useForkedSTP - Whether STP should be run in a separate process
  /// (required for using timeouts).
  /// \param optimizeDivides - Whether constant division operations should
  /// be optimized into add/shift/multiply operations.
  STPSolver(bool useForkedSTP, bool optimizeDivides = true);

  /// getConstraintLog - Return the constraint log for the given state in CVC
  /// format.
  virtual char *getConstraintLog(const Query &);

  /// setCoreSolverTimeout - Set constraint solver timeout delay to the given
  /// value; 0
  /// is off.
  virtual void setCoreSolverTimeout(time::Span timeout);
};
}

#endif /* KLEE_STPSOLVER_H */
