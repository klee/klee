//===-- TimerStatIncrementer.h ----------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_TIMERSTATINCREMENTER_H
#define KLEE_TIMERSTATINCREMENTER_H

#include "klee/Statistics/Statistics.h"
#include "klee/Support/Timer.h"

namespace klee {

  /**
   * A TimerStatIncrementer adds its lifetime to a specified Statistic.
   */
  class TimerStatIncrementer {
  private:
    const WallTimer timer;
    Statistic &statistic;

  public:
    explicit TimerStatIncrementer(Statistic &statistic) : statistic(statistic) {}
    ~TimerStatIncrementer() {
      // record microseconds
      statistic += timer.delta().toMicroseconds();
    }

    time::Span delta() const { return timer.delta(); }
  };
}

#endif /* KLEE_TIMERSTATINCREMENTER_H */
