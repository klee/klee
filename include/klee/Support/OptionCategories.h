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

#ifndef KLEE_OPTIONCATEGORIES_H
#define KLEE_OPTIONCATEGORIES_H

#include "klee/Support/CompilerWarning.h"
DISABLE_WARNING_PUSH
DISABLE_WARNING_DEPRECATED_DECLARATIONS
#include "llvm/Support/CommandLine.h"
DISABLE_WARNING_POP

namespace klee {
  extern llvm::cl::OptionCategory DebugCat;
  extern llvm::cl::OptionCategory ExprCat;
  extern llvm::cl::OptionCategory ExtCallsCat;
  extern llvm::cl::OptionCategory MemoryCat;
  extern llvm::cl::OptionCategory MergeCat;
  extern llvm::cl::OptionCategory MiscCat;
  extern llvm::cl::OptionCategory ModuleCat;
  extern llvm::cl::OptionCategory SearchCat;
  extern llvm::cl::OptionCategory ExecTreeCat;
  extern llvm::cl::OptionCategory SeedingCat;
  extern llvm::cl::OptionCategory SolvingCat;
  extern llvm::cl::OptionCategory StatsCat;
  extern llvm::cl::OptionCategory TerminationCat;
  extern llvm::cl::OptionCategory TestGenCat;
}

#endif /* KLEE_OPTIONCATEGORIES_H */
