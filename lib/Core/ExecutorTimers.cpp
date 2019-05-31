//===-- ExecutorTimers.cpp ------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "CoreStats.h"
#include "Executor.h"
#include "ExecutorTimerInfo.h"
#include "PTree.h"
#include "StatsTracker.h"

#include "klee/ExecutionState.h"
#include "klee/Internal/Module/InstructionInfoTable.h"
#include "klee/Internal/Module/KInstruction.h"
#include "klee/Internal/Module/KModule.h"
#include "klee/Internal/Support/ErrorHandling.h"
#include "klee/Internal/System/Time.h"
#include "klee/OptionCategories.h"

#include "llvm/IR/Function.h"
#include "llvm/Support/CommandLine.h"

#include <math.h>
#include <signal.h>
#include <string>
#include <sys/time.h>
#include <unistd.h>

using namespace llvm;
using namespace klee;

namespace klee {
cl::opt<std::string>
    MaxTime("max-time",
            cl::desc("Halt execution after the specified number of seconds.  "
                     "Set to 0s to disable (default=0s)"),
            cl::init("0s"),
            cl::cat(TerminationCat));

cl::opt<std::string>
    TimerInterval("timer-interval",
                  cl::desc("Minimum interval to check timers (default=1s)"),
                  cl::init("1s"),
                  cl::cat(TerminationCat)); // StatsCat?
}

///

class HaltTimer : public Executor::Timer {
  Executor *executor;

public:
  HaltTimer(Executor *_executor) : executor(_executor) {}
  ~HaltTimer() {}

  void run() {
    klee_message("HaltTimer invoked");
    executor->setHaltExecution(true);
  }
};

///

void Executor::initTimers() {
  const time::Span maxTime(MaxTime);
  if (maxTime) {
    addTimer(new HaltTimer(this), maxTime);
  }
}

///

void Executor::addTimer(Timer *timer, time::Span rate) {
  timers.push_back(new TimerInfo(timer, rate));
}

void Executor::processTimers(ExecutionState *current,
                             time::Span maxInstTime) {
  const static time::Span minInterval(TimerInterval);
  static time::Point lastCheckedTime;
  const auto currentTime = time::getWallTime();
  const auto duration = currentTime - lastCheckedTime;

  if (duration < minInterval) return;

  // check max instruction time
  if (maxInstTime && current &&
      std::find(removedStates.begin(), removedStates.end(), current) == removedStates.end() &&
      duration > maxInstTime) {
    klee_warning("max-instruction-time exceeded: %.2fs", duration.toSeconds());
    terminateStateEarly(*current, "max-instruction-time exceeded");
  }


  // check timers
  for (auto &timerInfo : timers) {
    if (currentTime >= timerInfo->nextFireTime) {
      timerInfo->timer->run();
      timerInfo->nextFireTime = currentTime + timerInfo->rate;
    }
  }

  lastCheckedTime = currentTime;
}
