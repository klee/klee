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

#include "klee/Internal/System/Time.h"

namespace klee {
  class WallTimer {
    time::Point start;
    
  public:
    WallTimer();

    /// check - Return the delta since the timer was created
    time::Span check();
  };
}

#endif

