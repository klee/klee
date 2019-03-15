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

static const time::Span kMilliSecondsPerTick(time::milliseconds(100));
static volatile unsigned timerTicks = 0;

// XXX hack
extern "C" unsigned dumpStates, dumpPTree;
unsigned dumpStates = 0, dumpPTree = 0;

static void onAlarm(int) {
  ++timerTicks;
}

// oooogalay
static void setupHandler() {
  itimerval t{};
  timeval tv = static_cast<timeval>(kMilliSecondsPerTick);
  t.it_interval = t.it_value = tv;

  ::setitimer(ITIMER_REAL, &t, nullptr);
  ::signal(SIGALRM, onAlarm);
}

void Executor::initTimers() {
  static bool first = true;

  if (first) {
    first = false;
    setupHandler();
  }

  const time::Span maxTime(MaxTime);
  if (maxTime) {
    addTimer(new HaltTimer(this), maxTime);
  }
}

///

Executor::Timer::Timer() {}

Executor::Timer::~Timer() {}

void Executor::addTimer(Timer *timer, time::Span rate) {
  timers.push_back(new TimerInfo(timer, rate));
}

void Executor::processTimers(ExecutionState *current,
                             time::Span maxInstTime) {
  static unsigned callsWithoutCheck = 0;
  unsigned ticks = timerTicks;

  if (!ticks && ++callsWithoutCheck > 1000) {
    setupHandler();
    ticks = 1;
  }

  if (ticks || dumpPTree || dumpStates) {
    if (dumpPTree) {
      char name[32];
      sprintf(name, "ptree%08d.dot", (int) stats::instructions);
      auto os = interpreterHandler->openOutputFile(name);
      if (os) {
        processTree->dump(*os);
      }

      dumpPTree = 0;
    }

    if (dumpStates) {
      auto os = interpreterHandler->openOutputFile("states.txt");

      if (os) {
        for (ExecutionState *es : states) {
          *os << "(" << es << ",";
          *os << "[";
          auto next = es->stack.begin();
          ++next;
          for (auto sfIt = es->stack.begin(), sf_ie = es->stack.end();
               sfIt != sf_ie; ++sfIt) {
            *os << "('" << sfIt->kf->function->getName().str() << "',";
            if (next == es->stack.end()) {
              *os << es->prevPC->info->line << "), ";
            } else {
              *os << next->caller->info->line << "), ";
              ++next;
            }
          }
          *os << "], ";

          StackFrame &sf = es->stack.back();
          uint64_t md2u = computeMinDistToUncovered(es->pc,
                                                    sf.minDistToUncoveredOnReturn);
          uint64_t icnt = theStatisticManager->getIndexedValue(stats::instructions,
                                                               es->pc->info->id);
          uint64_t cpicnt = sf.callPathNode->statistics.getValue(stats::instructions);

          *os << "{";
          *os << "'depth' : " << es->depth << ", ";
          *os << "'weight' : " << es->weight << ", ";
          *os << "'queryCost' : " << es->queryCost << ", ";
          *os << "'coveredNew' : " << es->coveredNew << ", ";
          *os << "'instsSinceCovNew' : " << es->instsSinceCovNew << ", ";
          *os << "'md2u' : " << md2u << ", ";
          *os << "'icnt' : " << icnt << ", ";
          *os << "'CPicnt' : " << cpicnt << ", ";
          *os << "}";
          *os << ")\n";
        }
      }

      dumpStates = 0;
    }

    if (maxInstTime && current &&
        std::find(removedStates.begin(), removedStates.end(), current) ==
            removedStates.end()) {
      if (timerTicks * kMilliSecondsPerTick > maxInstTime) {
        klee_warning("max-instruction-time exceeded: %.2fs", (timerTicks * kMilliSecondsPerTick).toSeconds());
        terminateStateEarly(*current, "max-instruction-time exceeded");
      }
    }

    if (!timers.empty()) {
      auto time = time::getWallTime();

      for (std::vector<TimerInfo*>::iterator it = timers.begin(), 
             ie = timers.end(); it != ie; ++it) {
        TimerInfo *ti = *it;
        
        if (time >= ti->nextFireTime) {
          ti->timer->run();
          ti->nextFireTime = time + ti->rate;
        }
      }
    }

    timerTicks = 0;
    callsWithoutCheck = 0;
  }
}

