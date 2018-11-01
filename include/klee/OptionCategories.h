//===-- OptionCategories.h --------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

/*
 * This header defines the option categories used in KLEE.
 */

#ifndef KLEE_SOLVERCOMMANDLINE_H
#define KLEE_SOLVERCOMMANDLINE_H

#include "llvm/Support/CommandLine.h"

namespace klee {
  extern cl::OptionCategory SolvingCat;
}

#endif
