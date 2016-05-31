//===-- SeedInfo.h ----------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_SEEDINFO_H
#define KLEE_SEEDINFO_H

#include "klee/util/Assignment.h"

extern "C" {
  struct KTest;
  struct KTestObject;
}

namespace klee {
  class ExecutionState;
  class TimingSolver;

  class SeedInfo {
  public:
    Assignment assignment;
    KTest *input;
    unsigned inputPosition;
    std::set<struct KTestObject*> used;
    /* occasionally, in seed/zest mode, we might want to interleave
 *     execution of states which are not on the concrete path with
 *     the concrete path state (rather than execute them at the end).
 *     Because the Executor logic considers in first instance only
 *     states with attached seeds, we create fake seeds */
    bool fakeSeed;

  public:
    explicit SeedInfo(KTest *_input, bool fake = false)
        : assignment(true), input(_input), inputPosition(0), fakeSeed(fake) {}

    KTestObject *getNextInput(const MemoryObject *mo,
                             bool byName);
    
    /// Patch the seed so that condition is satisfied while retaining as
    /// many of the seed values as possible.
    void patchSeed(const ExecutionState &state, 
                   ref<Expr> condition,
                   TimingSolver *solver);
  };
}

#endif
