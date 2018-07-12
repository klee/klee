//===-- StatsTracker.cpp --------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "StatsTracker.h"

#include "klee/ExecutionState.h"
#include "klee/Statistics.h"
#include "klee/Config/Version.h"
#include "klee/Internal/Module/InstructionInfoTable.h"
#include "klee/Internal/Module/KModule.h"
#include "klee/Internal/Module/KInstruction.h"
#include "klee/Internal/Support/ModuleUtil.h"
#include "klee/Internal/System/MemoryUsage.h"
#include "klee/Internal/System/Time.h"
#include "klee/Internal/Support/ErrorHandling.h"
#include "klee/SolverStats.h"

#include "CallPathManager.h"
#include "CoreStats.h"
#include "CoverageTracker.h"
#include "Executor.h"
#include "MemoryManager.h"
#include "UserSearcher.h"

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Process.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/FileSystem.h"

#if LLVM_VERSION_CODE < LLVM_VERSION(3, 5)
#include "llvm/Support/CallSite.h"
#include "llvm/Support/CFG.h"
#else
#include "llvm/IR/CallSite.h"
#include "llvm/IR/CFG.h"
#endif

#include <fstream>
#include <unistd.h>

using namespace klee;
using namespace llvm;

///

namespace {
cl::OptionCategory StatisticsTracking("Statistics Tracking",
                                      "Controls how to track statistics");

cl::opt<bool> TrackInstructionTime(
    "track-instruction-time", cl::init(false),
    cl::desc(
        "Enable tracking of time for individual instructions (default=off)"),
    cl::cat(StatisticsTracking));

cl::opt<double> StatsWriteInterval(
    "stats-write-interval", cl::init(1.),
    cl::desc("Write running stats trace file. Approximate number of seconds "
             "between stats writes (default=1.0s)"),
    cl::cat(StatisticsTracking));

cl::opt<unsigned> StatsWriteAfterInstructions(
    "stats-write-after-instructions", cl::init(0),
    cl::desc("Write running stats trace file. Write statistics after each n "
             "instructions, 0 to disable "
             "(default=0)"),
    cl::cat(StatisticsTracking));

cl::opt<double> IStatsWriteInterval(
    "istats-write-interval", cl::init(10.),
    cl::desc("Write instruction level statistics in callgrind format. "
             "Approximately write every n seconds (default: 10.0s)"),
    cl::cat(StatisticsTracking));

cl::opt<unsigned> IStatsWriteAfterInstructions(
    "istats-write-after-instructions", cl::init(0),
    cl::desc("Write instruction level statistics in callgrind format. Write "
             "after each n instructions, 0 to disable "
             "(default=0)"),
    cl::cat(StatisticsTracking));

cl::opt<bool> UseCallPaths("use-call-paths", cl::init(true),
                           cl::desc("Enable calltree tracking for instruction "
                                    "level statistics (default=on)"),
                           cl::cat(StatisticsTracking));
}

///

namespace klee {
  class WriteIStatsTimer : public Executor::Timer {
    StatsTracker *statsTracker;
    
  public:
    WriteIStatsTimer(StatsTracker *_statsTracker) : statsTracker(_statsTracker) {}
    ~WriteIStatsTimer() {}
    
    void run() { statsTracker->writeIStats(); }
  };
  
  class WriteStatsTimer : public Executor::Timer {
    StatsTracker *statsTracker;
    
  public:
    WriteStatsTimer(StatsTracker *_statsTracker) : statsTracker(_statsTracker) {}
    ~WriteStatsTimer() {}
    
    void run() { statsTracker->writeStatsLine(); }
  };

}

//

StatsTracker::StatsTracker(Executor &_executor, std::string _objectFilename)
    : executor(_executor), objectFilename(_objectFilename),
      startWallTime(util::getWallTime()), instructionGranularityTracking(false),
      codePathGranularityTracking(false) {

  // Handle and check all command line arguments before we setup the tracker

  if (StatsWriteAfterInstructions && StatsWriteInterval)
    klee_warning("Both options --stats-write-interval and "
                 "--stats-write-after-instructions are enabled. Consider "
                 "disabling one.");

  if (IStatsWriteAfterInstructions && IStatsWriteInterval)
    klee_warning("Both options --istats-write-interval and "
                 "--istats-write-after-instructions are enabled. Consider "
                 "disabling one.");

  // check if istats tracking is requested
  if (IStatsWriteInterval || IStatsWriteAfterInstructions)
    doTrackByInstruction();

  if (UseCallPaths)
    doTrackCodePath();

  // Enable coverage tracking if requested by user
  if (CoverageTracker::userRequestedCoverageTracking())
    doTrackCoverage();

  if (StatsWriteAfterInstructions || StatsWriteInterval) {
    statsFile = executor.interpreterHandler->openOutputFile("run.stats");
    if (!statsFile)
      klee_error("Unable to open statistics log file");

    writeStatsHeader();
    writeStatsLine();

    if (StatsWriteInterval > 0)
      executor.addTimer(new WriteStatsTimer(this), StatsWriteInterval);
  }

  if (IStatsWriteInterval || IStatsWriteAfterInstructions) {
    if (!sys::path::is_absolute(objectFilename)) {
      SmallString<128> current(objectFilename);
      if (sys::fs::make_absolute(current)) {
#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 6)
        Twine current_twine(current.str()); // requires a twine for this
        if (!sys::fs::exists(current_twine)) {
#elif LLVM_VERSION_CODE >= LLVM_VERSION(3, 5)
        bool exists = false;
        if (!sys::fs::exists(current.str(), exists)) {
#else
        bool exists = false;
        error_code ec = sys::fs::exists(current.str(), exists);
        if (ec == errc::success && exists) {
#endif
          objectFilename = current.c_str();
        }
      }
    }

    istatsFile = executor.interpreterHandler->openOutputFile("run.istats");
    if (!istatsFile)
      klee_error("Unable to open istats file");

    if (IStatsWriteInterval > 0)
      executor.addTimer(new WriteIStatsTimer(this), IStatsWriteInterval);
  }
}

void StatsTracker::done() {
  if (statsFile)
    writeStatsLine();

  if (istatsFile)
    writeIStats();
}

void StatsTracker::stepInstruction(ExecutionState &es) {
  const InstructionInfo &ii = *es.pc->info;

  // Update the statistics tracker to refer to the current instruction
  if (instructionGranularityTracking)
    theStatisticManager->setIndex(ii.id);

  if (codePathGranularityTracking) {
    StackFrame &sf = es.stack.back();
    theStatisticManager->setContext(&sf.callPathNode->statistics);
  }

  if (TrackInstructionTime) {
    sys::TimeValue sys(0, 0);
    sys::Process::GetTimeUsage(lastNowTime, lastUserTime, sys);
  }

  if (coverageTracker)
    coverageTracker->stepInstruction(es);

  if (statsFile && StatsWriteAfterInstructions &&
      (stats::instructions % StatsWriteAfterInstructions.getValue() == 0))
    writeStatsLine();

  if (istatsFile && IStatsWriteAfterInstructions &&
      (stats::instructions % IStatsWriteAfterInstructions.getValue() == 0))
    writeIStats();
}

void StatsTracker::postStepInstruction(ExecutionState &es) {
  if (TrackInstructionTime) {
    // Update statistic: the assumption is that statistics are still correctly
    // referencing the state
    sys::TimeValue now(0, 0), user(0, 0), sys(0, 0);
    sys::Process::GetTimeUsage(now, user, sys);
    sys::TimeValue delta = user - lastUserTime;
    sys::TimeValue deltaNow = now - lastNowTime;
    stats::instructionTime += delta.usec();
    stats::instructionRealTime += deltaNow.usec();
  }
}

///

/* Should be called _after_ the es->pushFrame() */
void StatsTracker::framePushed(ExecutionState &es, StackFrame *parentFrame) {
  if (codePathGranularityTracking) {
    StackFrame &sf = es.stack.back();
    CallPathNode *parent = parentFrame ? parentFrame->callPathNode : 0;
    CallPathNode *cp = callPathManager.getCallPath(
        parent, sf.caller ? sf.caller->inst : 0, sf.kf->function);
    sf.callPathNode = cp;
    cp->count++;
  }

  if (coverageTracker)
    coverageTracker->updateFrameCoverage(es);
}

/* Should be called _after_ the es->popFrame() */
void StatsTracker::framePopped(ExecutionState &es) {
  // XXX remove me?
}


void StatsTracker::markBranchVisited(ExecutionState *visitedTrue, 
                                     ExecutionState *visitedFalse) {
  if (!coverageTracker)
    return;

  coverageTracker->markBranchVisited(visitedTrue, visitedFalse);
}

void StatsTracker::writeStatsHeader() {
  *statsFile << "('Instructions',"
             << "'FullBranches',"
             << "'PartialBranches',"
             << "'NumBranches',"
             << "'UserTime',"
             << "'NumStates',"
             << "'MallocUsage',"
             << "'NumQueries',"
             << "'NumQueryConstructs',"
             << "'NumObjects',"
             << "'WallTime',"
             << "'CoveredInstructions',"
             << "'UncoveredInstructions',"
             << "'QueryTime',"
             << "'SolverTime',"
             << "'CexCacheTime',"
             << "'ForkTime',"
             << "'ResolveTime',"
             << "'QueryCexCacheMisses',"
             << "'QueryCexCacheHits',"
#ifdef KLEE_ARRAY_DEBUG
	     << "'ArrayHashTime',"
#endif
             << ")\n";
  statsFile->flush();
}

double StatsTracker::elapsed() {
  return util::getWallTime() - startWallTime;
}

void StatsTracker::writeStatsLine() {
  *statsFile << "(" << stats::instructions << ","
             << (coverageTracker ? coverageTracker->fullBranches : 0) << ","
             << (coverageTracker ? coverageTracker->partialBranches : 0) << ","
             << (coverageTracker ? coverageTracker->numBranches : 0) << ","
             << util::getUserTime() << "," << executor.states.size() << ","
             << util::GetTotalMallocUsage() +
                    executor.memory->getUsedDeterministicSize()
             << "," << stats::queries << "," << stats::queryConstructs << ","
             << 0 // was numObjects
             << "," << elapsed() << "," << stats::coveredInstructions << ","
             << stats::uncoveredInstructions << ","
             << stats::queryTime / 1000000. << ","
             << stats::solverTime / 1000000. << ","
             << stats::cexCacheTime / 1000000. << ","
             << stats::forkTime / 1000000. << ","
             << stats::resolveTime / 1000000. << ","
             << stats::queryCexCacheMisses << "," << stats::queryCexCacheHits
#ifdef KLEE_ARRAY_DEBUG
             << "," << stats::arrayHashTime / 1000000.
#endif
             << ")\n";
  statsFile->flush();
}

void StatsTracker::updateStateStatistics(uint64_t addend) {
  for (std::set<ExecutionState*>::iterator it = executor.states.begin(),
         ie = executor.states.end(); it != ie; ++it) {
    ExecutionState &state = **it;
    const InstructionInfo &ii = *state.pc->info;
    theStatisticManager->incrementIndexedValue(stats::states, ii.id, addend);
    if (codePathGranularityTracking)
      state.stack.back().callPathNode->statistics.incrementValue(stats::states, addend);
  }
}

void StatsTracker::writeIStats() {
  const auto m = executor.kmodule->module.get();
  uint64_t istatsMask = 0;
  llvm::raw_fd_ostream &of = *istatsFile;
  
  // We assume that we didn't move the file pointer
  unsigned istatsSize = of.tell();

  of.seek(0);

  of << "version: 1\n";
  of << "creator: klee\n";
  of << "pid: " << getpid() << "\n";
  of << "cmd: " << m->getModuleIdentifier() << "\n\n";
  of << "\n";
  
  StatisticManager &sm = *theStatisticManager;
  unsigned nStats = sm.getNumStatistics();

  // Max is 13, sadly
  istatsMask |= 1<<sm.getStatisticID("Queries");
  istatsMask |= 1<<sm.getStatisticID("QueriesValid");
  istatsMask |= 1<<sm.getStatisticID("QueriesInvalid");
  istatsMask |= 1<<sm.getStatisticID("QueryTime");
  istatsMask |= 1<<sm.getStatisticID("ResolveTime");
  istatsMask |= 1<<sm.getStatisticID("Instructions");
  istatsMask |= 1<<sm.getStatisticID("InstructionTimes");
  istatsMask |= 1<<sm.getStatisticID("InstructionRealTimes");
  istatsMask |= 1<<sm.getStatisticID("Forks");
  istatsMask |= 1<<sm.getStatisticID("CoveredInstructions");
  istatsMask |= 1<<sm.getStatisticID("UncoveredInstructions");
  istatsMask |= 1<<sm.getStatisticID("States");
  istatsMask |= 1<<sm.getStatisticID("MinDistToUncovered");

  of << "positions: instr line\n";

  for (unsigned i=0; i<nStats; i++) {
    if (istatsMask & (1<<i)) {
      Statistic &s = sm.getStatistic(i);
      of << "event: " << s.getShortName() << " : " 
         << s.getName() << "\n";
    }
  }

  of << "events: ";
  for (unsigned i=0; i<nStats; i++) {
    if (istatsMask & (1<<i))
      of << sm.getStatistic(i).getShortName() << " ";
  }
  of << "\n";

  // Track how many states are currently at which instruction, to do that
  // set state counts, write the stats and decrement it after that. So we
  // process such that we don't have to zero all records each time.
  if (istatsMask & (1<<stats::states.getID()))
    updateStateStatistics(1);

  std::string sourceFile = "";

  CallSiteSummaryTable callSiteStats;
  if (codePathGranularityTracking)
    callPathManager.getSummaryStatistics(callSiteStats);

  of << "ob=" << objectFilename << "\n";

  for (Module::iterator fnIt = m->begin(), fn_ie = m->end(); 
       fnIt != fn_ie; ++fnIt) {
    if (!fnIt->isDeclaration()) {
      // Always try to write the filename before the function name, as otherwise
      // KCachegrind can create two entries for the function, one with an
      // unnamed file and one without.
      Function *fn = &*fnIt;
      const InstructionInfo &ii = executor.kmodule->infos->getFunctionInfo(fn);
      if (ii.file != sourceFile) {
        of << "fl=" << ii.file << "\n";
        sourceFile = ii.file;
      }
      
      of << "fn=" << fnIt->getName().str() << "\n";
      for (Function::iterator bbIt = fnIt->begin(), bb_ie = fnIt->end(); 
           bbIt != bb_ie; ++bbIt) {
        for (BasicBlock::iterator it = bbIt->begin(), ie = bbIt->end(); 
             it != ie; ++it) {
          Instruction *instr = &*it;
          const InstructionInfo &ii = executor.kmodule->infos->getInfo(instr);
          unsigned index = ii.id;
          if (ii.file!=sourceFile) {
            of << "fl=" << ii.file << "\n";
            sourceFile = ii.file;
          }
          of << ii.assemblyLine << " ";
          of << ii.line << " ";
          for (unsigned i=0; i<nStats; i++)
            if (istatsMask&(1<<i))
              of << sm.getIndexedValue(sm.getStatistic(i), index) << " ";
          of << "\n";

          if (codePathGranularityTracking &&
              (isa<CallInst>(instr) || isa<InvokeInst>(instr))) {
            CallSiteSummaryTable::iterator it = callSiteStats.find(instr);
            if (it!=callSiteStats.end()) {
              for (std::map<llvm::Function*, CallSiteInfo>::iterator
                     fit = it->second.begin(), fie = it->second.end(); 
                   fit != fie; ++fit) {
                Function *f = fit->first;
                CallSiteInfo &csi = fit->second;
                const InstructionInfo &fii = 
                  executor.kmodule->infos->getFunctionInfo(f);
  
                if (fii.file!="" && fii.file!=sourceFile)
                  of << "cfl=" << fii.file << "\n";
                of << "cfn=" << f->getName().str() << "\n";
                of << "calls=" << csi.count << " ";
                of << fii.assemblyLine << " ";
                of << fii.line << "\n";

                of << ii.assemblyLine << " ";
                of << ii.line << " ";
                for (unsigned i=0; i<nStats; i++) {
                  if (istatsMask&(1<<i)) {
                    Statistic &s = sm.getStatistic(i);
                    uint64_t value;

                    // Hack, ignore things that don't make sense on
                    // call paths.
                    if (&s == &stats::uncoveredInstructions) {
                      value = 0;
                    } else {
                      value = csi.statistics.getValue(s);
                    }

                    of << value << " ";
                  }
                }
                of << "\n";
              }
            }
          }
        }
      }
    }
  }

  if (istatsMask & (1<<stats::states.getID()))
    updateStateStatistics((uint64_t)-1);
  
  // Clear then end of the file if necessary (no truncate op?).
  unsigned pos = of.tell();
  for (unsigned i=pos; i<istatsSize; ++i)
    of << '\n';
  
  of.flush();
}

void StatsTracker::doTrackByInstruction() {
  // Check if already initialized
  if (instructionGranularityTracking)
    return;

  theStatisticManager->useIndexedStats(executor.kmodule->infos->getMaxID());

  // Mark as enabled and initialized
  instructionGranularityTracking = true;
}

void StatsTracker::doTrackCoverage() {
  // Check if already initialized
  if (coverageTracker) {
    return;
  }

  coverageTracker = std::unique_ptr<CoverageTracker>(
      new CoverageTracker(*this, *executor.kmodule, executor.states, executor));
  coverageTracker->initializeCoverageTracking();
}

void StatsTracker::requestAutomaticDistanceMetricUpdates() {
  coverageTracker->requestAutomaticDistanceCalculation();
}

void StatsTracker::doTrackCodePath() {
  // Check if already initialized
  if (codePathGranularityTracking)
    return;

  codePathGranularityTracking = true;
}
