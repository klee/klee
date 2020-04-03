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

#include "klee/System/Time.h"

#include "llvm/ADT/SmallVector.h"

#include <functional>
#include <memory>

namespace klee {

  /**
   * A WallTimer stores its creation time.
   */
  class WallTimer {
    const time::Point start;
  public:
    WallTimer();

    /// Return the delta since the timer was created
    time::Span delta() const;
  };


  /**
   * A Timer repeatedly executes a `callback` after a specified `interval`.
   * An object of this class is _passive_ and only keeps track of the next invocation time.
   * _Passive_ means, that it has to be `invoke`d by an external caller with the current time.
   * Only when the time span between the current time and the last invocation exceeds the
   * specified `interval`, the `callback` will be executed.
   * Multiple timers are typically managed by a TimerGroup.
   */
  class Timer {
    /// Approximate interval between callback invocations.
    time::Span interval;
    /// Wall time for next invocation.
    time::Point nextInvocationTime;
    /// The event callback.
    std::function<void()> run;
  public:
    /// \param interval The time span between callback invocations.
    /// \param callback The event callback.
    Timer(const time::Span &interval, std::function<void()> &&callback);

    /// Return specified `interval` between invocations.
    time::Span getInterval() const;
    /// Execute `callback` if invocation time exceeded.
    void invoke(const time::Point &currentTime);
    /// Set new invocation time to `currentTime + interval`.
    void reset(const time::Point &currentTime);
  };


  /**
   * A TimerGroup manages multiple timers.
   *
   * TimerGroup simplifies the handling of multiple Timer objects by offering a unifying
   * Timer-like interface. Additionally, it serves as a barrier and prevents timers from
   * being invoked too often by defining a minimum invocation interval (MI).
   * All registered timer intervals should be larger than MI and also be multiples of MI.
   * Similar to Timer, a TimerGroup is _passive_ and needs to be `invoke`d by an external
   * caller.
   */
  class TimerGroup {
    /// Registered timers.
    llvm::SmallVector<std::unique_ptr<Timer>, 4> timers;
    /// Timer that invokes all registered timers after minimum interval.
    Timer invocationTimer;
    /// Time of last `invoke` call.
    time::Point currentTime;
  public:
    /// \param minInterval The minimum interval between invocations of registered timers.
    explicit TimerGroup(const time::Span &minInterval);

    /// Add a timer to be executed periodically.
    ///
    /// \param timer The timer object to register.
    void add(std::unique_ptr<Timer> timer);
    /// Invoke registered timers with current time only if minimum interval exceeded.
    void invoke();
    /// Reset all timers.
    void reset();
  };

} // klee
#endif /* KLEE_TIMER_H */
