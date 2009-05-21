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
  struct BOut;
  struct BOutObject;
}

namespace klee {
  class ExecutionState;
  class TimingSolver;

  class SeedInfo {
  public:
    Assignment assignment;
    BOut *input;
    unsigned inputPosition;
    std::set<struct BOutObject*> used;
    
  public:
    explicit
    SeedInfo(BOut *_input) : assignment(true),
                             input(_input),
                             inputPosition(0) {}
    
    BOutObject *getNextInput(const MemoryObject *mo,
                             bool byName);
    
    /// Patch the seed so that condition is satisfied while retaining as
    /// many of the seed values as possible.
    void patchSeed(const ExecutionState &state, 
                   ref<Expr> condition,
                   TimingSolver *solver);
  };
}

#endif
