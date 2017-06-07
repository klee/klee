//===-- Timer.h -------------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_TIMER_H
#define KLEE_TIMER_H

#include <stdint.h>

#if LLVM_VERSION_CODE >= LLVM_VERSION(4, 0)
#include <llvm/Support/Chrono.h>
#endif

namespace klee {
  class WallTimer {
#if LLVM_VERSION_CODE >= LLVM_VERSION(4, 0)
    llvm::sys::TimePoint<> start;
#else
    uint64_t startMicroseconds;
#endif
    
  public:
    WallTimer();

    /// check - Return the delta since the timer was created, in microseconds.
    uint64_t check();
  };
}

#endif

