//===-- Executor.h ----------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Class to wrap information for a timer.
//
//===----------------------------------------------------------------------===//

#ifndef EXECUTORTIMERINFO_H_
#define EXECUTORTIMERINFO_H_

#include "klee/Internal/System/Time.h"

namespace klee {

class Executor::TimerInfo {
public:
  Timer *timer;

  /// Approximate delay per timer firing.
  time::Span rate;
  /// Wall time for next firing.
  time::Point nextFireTime;

public:
  TimerInfo(Timer *_timer, time::Span _rate)
    : timer(_timer),
      rate(_rate),
      nextFireTime(time::getWallTime() + rate) {}
  ~TimerInfo() { delete timer; }
};


}


#endif /* EXECUTORTIMERINFO_H_ */
