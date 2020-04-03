//===-- Cell.h --------------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_CELL_H
#define KLEE_CELL_H

#include "klee/Expr/Expr.h"

namespace klee {
  class MemoryObject;

  struct Cell {
    ref<Expr> value;
  };
}

#endif /* KLEE_CELL_H */
