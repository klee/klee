//===-- UserSearcher.h ------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_USERSEARCHER_H
#define KLEE_USERSEARCHER_H

#include "klee/Support/CompilerWarning.h"
DISABLE_WARNING_PUSH
DISABLE_WARNING_DEPRECATED_DECLARATIONS
#include "llvm/Support/CommandLine.h"
DISABLE_WARNING_POP

namespace klee {
class Executor;
class Searcher;

// XXX gross, should be on demand?
bool userSearcherRequiresMD2U();

void initializeSearchOptions();

Searcher *constructUserSearcher(Executor &executor,
                                bool stopAfterReachingTarget = true);
} // namespace klee

#endif /* KLEE_USERSEARCHER_H */
