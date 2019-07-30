//===-- Timer.cpp ---------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Internal/Support/ErrorHandling.h"
#include "klee/Internal/Support/Timer.h"
#include "klee/Internal/System/Time.h"


using namespace klee;


// WallTimer

WallTimer::WallTimer() : start{time::getWallTime()} {}

time::Span WallTimer::delta() const {
  return {time::getWallTime() - start};
}


// Timer

Timer::Timer(const time::Span &interval, std::function<void()> &&callback) :
  interval{interval}, nextInvocationTime{time::getWallTime() + interval}, run{std::move(callback)} {};

time::Span Timer::getInterval() const {
  return interval;
};

void Timer::invoke(const time::Point &currentTime) {
  if (currentTime < nextInvocationTime) return;

  run();
  nextInvocationTime = currentTime + interval;
};

void Timer::reset(const time::Point &currentTime) {
  nextInvocationTime = currentTime + interval;
};


// TimerGroup

TimerGroup::TimerGroup(const time::Span &minInterval) :
  invocationTimer{
    minInterval,
    [&]{
      // invoke timers
      for (auto &timer : timers)
        timer->invoke(currentTime);
    }
  } {};

void TimerGroup::add(std::unique_ptr<klee::Timer> timer) {
  const auto &interval = timer->getInterval();
  const auto &minInterval = invocationTimer.getInterval();
  if (interval < minInterval)
    klee_warning("Timer interval below minimum timer interval (-timer-interval)");
  if (interval.toMicroseconds() % minInterval.toMicroseconds())
    klee_warning("Timer interval not a multiple of timer interval (-timer-interval)");

  timers.emplace_back(std::move(timer));
}

void TimerGroup::invoke() {
  currentTime = time::getWallTime();
  invocationTimer.invoke(currentTime);
}

void TimerGroup::reset() {
  currentTime = time::getWallTime();
  invocationTimer.reset(currentTime);
  for (auto &timer : timers)
    timer->reset(currentTime);
}
