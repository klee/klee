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

namespace klee {
class Executor;
class Searcher;

// XXX gross, should be on demand?
bool userSearcherRequiresMD2U();

void initializeSearchOptions();

Searcher *constructBaseSearcher(Executor &executor);
Searcher *constructUserSearcher(Executor &executor);
struct BaseSearcherConstructor {
  Executor &executor;
  BaseSearcherConstructor(Executor &executor) : executor(executor) {}
  Searcher *operator()() const { return constructBaseSearcher(executor); }
};
} // namespace klee

#endif /* KLEE_USERSEARCHER_H */
