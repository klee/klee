#ifndef BITWUZLASOLVER_H_
#define BITWUZLASOLVER_H_

#include "klee/Solver/Solver.h"

namespace klee {

/// BitwuzlaSolver - A complete solver based on Bitwuzla
class BitwuzlaSolver : public Solver {
public:
  /// BitwuzlaSolver - Construct a new BitwuzlaSolver.
  BitwuzlaSolver();
};

class BitwuzlaTreeSolver : public Solver {
public:
  BitwuzlaTreeSolver(unsigned maxSolvers);
};

} // namespace klee

#endif
