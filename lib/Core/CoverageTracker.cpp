//===-- CoverageTracker.cpp -------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "CoverageTracker.h"
#include "klee/Config/Version.h"

#include "llvm/ADT/ilist.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/CommandLine.h"

#if LLVM_VERSION_CODE < LLVM_VERSION(3, 5)
#include "llvm/Support/CFG.h"
#include "llvm/Support/CallSite.h"
#else
#include "llvm/IR/CFG.h"
#include "llvm/IR/CallSite.h"
#endif

#include <algorithm>
#include <iterator>
#include <set>
#include <utility>

#include "CoreStats.h"
#include "Executor.h"
#include "StatsTracker.h"
#include "klee/ExecutionState.h"
#include "klee/Internal/Module/InstructionInfoTable.h"
#include "klee/Internal/Module/KInstIterator.h"
#include "klee/Internal/Module/KInstruction.h"
#include "klee/Internal/Module/KModule.h"
#include "klee/Internal/Support/ModuleUtil.h"
#include "klee/Statistic.h"
#include "klee/Statistics.h"

namespace {
llvm::cl::OptionCategory
    CoverageTracking("Coverage Tracking",
                     "Controls which coverage information should be tracked");

// XXX I really would like to have dynamic rate control for something like this.
llvm::cl::opt<double> UncoveredUpdateInterval("uncovered-update-interval",
                                              llvm::cl::init(30.),
                                              llvm::cl::desc("(default=30.0s)"),
                                              llvm::cl::cat(CoverageTracking));

llvm::cl::opt<bool> TrackBranchCoverage(
    "track-coverage-branch", llvm::cl::init(false),
    llvm::cl::desc("Track coverage of branches (default: false)"),
    llvm::cl::cat(CoverageTracking));
} // namespace

using namespace klee;

class UpdateReachableTimer : public Executor::Timer {
  CoverageTracker &coverageTracker;

public:
  UpdateReachableTimer(CoverageTracker &_coverageTracker)
      : coverageTracker(_coverageTracker) {}

  void run() { coverageTracker.computeReachableUncovered(); }
};

/// Returns a list of director successors of the provided instructions
///
/// The list will contain one item most of the time or multiple in case
/// the instruction is a conditional branch or switch
///
/// XXX What happens with noreturn, unreachable?
static std::vector<const llvm::Instruction *>
getSuccs(const llvm::Instruction *i) {
  auto ParentBasicBlock = i->getParent();
  std::vector<const llvm::Instruction *> res;

  if (i == ParentBasicBlock->getTerminator()) {
    for (auto it = llvm::succ_begin(ParentBasicBlock),
              ie = llvm::succ_end(ParentBasicBlock);
         it != ie; ++it)
      res.push_back(&*(it->begin()));
  } else {
    res.push_back(&*(++llvm::BasicBlock::const_iterator(i)));
  }

  return res;
}

/// Check for special cases where we statically know an instruction is
/// uncoverable. Currently the case is an unreachable instruction
/// following a noreturn call; the instruction is really only there to
/// satisfy LLVM's termination requirement.
static bool instructionIsCoverable(llvm::Instruction *i) {
  return i->getOpcode() != llvm::Instruction::Unreachable;
}

void CoverageTracker::calculateDistanceToReturn() {
  const llvm::Module *m = kmodule.module.get();
  const InstructionInfoTable &infos = *kmodule.infos;
  StatisticManager &sm = *theStatisticManager;

  // Compute call targets.
  for (auto &Func : *m) {
    for (auto &BB : Func) {
      for (auto &Inst : BB) {
        if (!isa<llvm::CallInst>(Inst) && !isa<llvm::InvokeInst>(Inst))
          continue;
        llvm::ImmutableCallSite cs(&Inst);
        if (isa<llvm::InlineAsm>(cs.getCalledValue())) {
          // We can never call through here so assume no targets
          // (which should be correct anyhow).
          callTargets.insert(
              std::make_pair(&Inst, std::vector<const llvm::Function *>()));
        } else if (const llvm::Function *target =
                       getDirectCallTarget(cs, /*moduleIsFullyLinked=*/true)) {
          callTargets[&Inst].push_back(target);
        } else {
          // It would be nice to use alias information
          // instead of assuming all indirect calls hit all escaping
          // functions, eh?
          callTargets[&Inst] = std::vector<const llvm::Function *>(
              kmodule.escapingFunctions.begin(),
              kmodule.escapingFunctions.end());
        }
      }
    }
  }

  // Reflexively compute function callers for callTargets.
  for (auto &callerCalleePair : callTargets) {
    for (auto &callee : callerCalleePair.second) {
      functionCallers[callee].push_back(callerCalleePair.first);
    }
  }

  // Assign every function a shortest path value, i.e.
  // if a function is called, what is the shortest path possible to return from
  // it 0 marks it no shortest path to return (e.g. noreturn functions).
  std::vector<const llvm::Instruction *> instructions;
  for (auto &Func : *m) {
    if (Func.isDeclaration()) {
      if (Func.doesNotReturn()) {
        functionShortestPath[&Func] = 0;
      } else {
        // Mark external calls
        // XXX quite small value, favours external calls
        functionShortestPath[&Func] = 1;
      }
    } else {
      // We start marking a function being unreachable
      functionShortestPath[&Func] = 0;

      // there's a corner case here when a function only includes a single
      // instruction (a ret). in that case, we MUST update
      // functionShortestPath, or it will remain 0 (erroneously indicating
      // that no return instructions are reachable)

      // If the function immediately returns, mark it as reachable
      if (isa<llvm::ReturnInst>(Func.getEntryBlock().getFirstNonPHI()))
        functionShortestPath[&Func] = 1;
    }

    // Mark all the return instructions of a function to have a shortest path
    // set to 1 (the instruction itself)
    for (auto &BB : Func) {
      for (auto &Inst : BB) {
        instructions.push_back(&Inst);
        unsigned id = infos.getInfo(&Inst).id;
        sm.setIndexedValue(stats::minDistToReturn, id,
                           isa<llvm::ReturnInst>(Inst) ? 1 : 0);
      }
    }
  }

  // propagate bottom-up, from the return instructions to the entry of the
  // function the shortest distance to the return instruction
  std::reverse(instructions.begin(), instructions.end());
  bool changed;
  do {
    changed = false;
    for (auto inst : instructions) {
      unsigned bestThrough = 0;
      if (isa<llvm::CallInst>(inst) || isa<llvm::InvokeInst>(inst)) {
        // Check every possible callee of this instruction
        // and take the shortest one
        for (auto calledFunction : callTargets[inst]) {
          uint64_t dist = functionShortestPath[calledFunction];
          if (!dist)
            continue;
          dist = 1 + dist; // count call/invoke instruction itself
          // update to the smallest or if not set yet
          if (bestThrough == 0 || dist < bestThrough)
            bestThrough = dist;
        }
      } else {
        // one instruction, one step
        bestThrough = 1;
      }

      // skip, if no way for this instruction found
      if (!bestThrough)
        continue;

      // retrieve the current distance to the return value
      unsigned id = infos.getInfo(inst).id;
      uint64_t best,
          oldBest = best = sm.getIndexedValue(stats::minDistToReturn, id);

      // and combine it with the shortest path among all direct successors
      for (auto successor : getSuccs(inst)) {
        uint64_t dist = sm.getIndexedValue(stats::minDistToReturn,
                                           infos.getInfo(successor).id);
        if (!dist)
          continue;

        // select shortest path among the successors
        uint64_t val = bestThrough + dist;
        if (best == 0 || val < best)
          best = val;
      }

      // Check if anything changed
      if (best == oldBest)
        continue;

      // update instruction to the new best value
      sm.setIndexedValue(stats::minDistToReturn, id, best);
      changed = true;
    }
  } while (changed);
}

void CoverageTracker::computeReachableUncovered(const KModule *km) {
  const llvm::Module *m = km->module.get();
  const InstructionInfoTable &infos = *km->infos;
  StatisticManager &sm = *theStatisticManager;

  // Initialise the minimum distance with the information of still
  // uncovered instructions
  // 0 indicates the instruction is already covered
  std::vector<const llvm::Instruction *> instructions;
  for (auto &Func : *m) {
    for (auto &BB : Func) {
      for (auto &Inst : BB) {
        unsigned id = infos.getInfo(&Inst).id;
        instructions.push_back(&Inst);
        sm.setIndexedValue(
            stats::minDistToUncovered, id,
            sm.getIndexedValue(stats::uncoveredInstructions, id));
      }
    }
  }

  std::reverse(instructions.begin(), instructions.end());
  bool changed;
  do {
    changed = false;
    for (auto &inst : instructions) {
      uint64_t shortestDistToUncovered,
          oldBest = shortestDistToUncovered = sm.getIndexedValue(
              stats::minDistToUncovered, infos.getInfo(inst).id);
      unsigned minDistToReturn = 0;

      if (isa<llvm::CallInst>(inst) || isa<llvm::InvokeInst>(inst)) {
        // Find shortest path to return instruction and shortest path to
        // uncovered instruction
        minDistToReturn =
            sm.getIndexedValue(stats::minDistToReturn, infos.getInfo(inst).id);

        // Retrieve the shortest path to the next uncovered instruction for all
        // possibly called functions
        for (auto fnIt : callTargets[inst]) {
          // Get the distance of the callee
          uint64_t calleeDist = sm.getIndexedValue(
              stats::minDistToUncovered, infos.getFunctionInfo(fnIt).id);
          if (!calleeDist)
            continue;

          calleeDist = 1 + calleeDist; // count instruction itself
          if (shortestDistToUncovered == 0 ||
              calleeDist < shortestDistToUncovered)
            shortestDistToUncovered = calleeDist;
        }
      } else {
        minDistToReturn = 1;
      }

      if (minDistToReturn) {
        for (auto successorInt : getSuccs(inst)) {
          uint64_t distUncoveredNext = sm.getIndexedValue(
              stats::minDistToUncovered, infos.getInfo(successorInt).id);
          if (!distUncoveredNext)
            continue;

          uint64_t val = minDistToReturn + distUncoveredNext;
          if (shortestDistToUncovered == 0 || val < shortestDistToUncovered)
            shortestDistToUncovered = val;
        }
      }

      // if nothing changed, move on
      if (shortestDistToUncovered == oldBest)
        continue;

      // update with the newest shortest distance to uncovered
      sm.setIndexedValue(stats::minDistToUncovered, infos.getInfo(inst).id,
                         shortestDistToUncovered);
      changed = true;
    }
  } while (changed);
}

void CoverageTracker::updateAllStateCoverage() {
  // Update all states
  // XXX Do that more clever
  for (auto &state : states) {
    uint64_t currentFrameMinDist = 0;
    for (auto sfIt = state->stack.begin(), sf_ie = state->stack.end();
         sfIt != sf_ie; ++sfIt) {
      auto next = sfIt + 1;
      KInstIterator kii;
      if (next == state->stack.end()) {
        kii = state->pc;
      } else {
        kii = next->caller;
        ++kii;
      }
      sfIt->minDistToUncoveredOnReturn = currentFrameMinDist;
      currentFrameMinDist = computeMinDistToUncovered(kii, currentFrameMinDist);
    }
  }
}

uint64_t klee::computeMinDistToUncovered(const KInstruction *ki,
                                         uint64_t minDistAtRA) {
  StatisticManager &sm = *theStatisticManager;

  if (minDistAtRA == 0) // unreachable on return, best is local
    return sm.getIndexedValue(stats::minDistToUncovered, ki->info->id);

  uint64_t minDistLocal =
      sm.getIndexedValue(stats::minDistToUncovered, ki->info->id);
  uint64_t distToReturn =
      sm.getIndexedValue(stats::minDistToReturn, ki->info->id);

  // return unreachable, best is local
  if (distToReturn == 0)
    return minDistLocal;

  // if no local instruction is uncovered, go back immediately
  if (!minDistLocal)
    return distToReturn + minDistAtRA;

  // select the shortest of either a locally reachable uncovered instruction or
  // the one on return
  return std::min(minDistLocal, distToReturn + minDistAtRA);
}

void CoverageTracker::computeReachableUncovered() {
  if (!initReachableUncovered) {
    initReachableUncovered = true;
    calculateDistanceToReturn();
  }
  computeReachableUncovered(executor.kmodule.get());
  updateAllStateCoverage();
}


void CoverageTracker::initializeCoverageTracking() {

  // Setup the coverage-based tracking

  // We need to request instruction-based tracking to make this work
  statsTracker.doTrackByInstruction();

  // Calculate uncovered instructions and uncovered branches
  for (const auto &kf : kmodule.functions) {

    // skip functions that we are not interested in covering (we assume that
    // everything is already covered)
    if (!kf->trackCoverage)
      continue;

    for (unsigned i = 0; i < kf->numInstructions; ++i) {
      KInstruction *ki = kf->instructions[i];

      unsigned id = ki->info->id;
      theStatisticManager->setIndex(id);
      if (instructionIsCoverable(ki->inst))
        ++stats::uncoveredInstructions;

      if (llvm::BranchInst *bi = dyn_cast<llvm::BranchInst>(ki->inst))
        if (!bi->isUnconditional())
          numBranches++;
    }
  }
}

void CoverageTracker::requestAutomaticDistanceCalculation() {
  if (automaticDistanceUpdate)
    return;
  automaticDistanceUpdate = true;

  computeReachableUncovered();
  executor.addTimer(new UpdateReachableTimer(*this), UncoveredUpdateInterval);
}

void CoverageTracker::stepInstruction(ExecutionState &es) {
  const InstructionInfo &ii = *es.pc->info;
  StackFrame &sf = es.stack.back();

  if (es.instsSinceCovNew)
    ++es.instsSinceCovNew;

  // Check if this covers an uncovered instruction
  if (sf.kf->trackCoverage && instructionIsCoverable(es.pc->inst) &&
      !theStatisticManager->getIndexedValue(stats::coveredInstructions,
                                            ii.id)) {
    es.coveredNew = true;
    es.instsSinceCovNew = 1;
    ++stats::coveredInstructions;
    stats::uncoveredInstructions += (uint64_t)-1;

    // Update list of newly covered lines
    es.coveredLines[&ii.file].insert(ii.line);
  }
}

bool CoverageTracker::userRequestedCoverageTracking() {
  return TrackBranchCoverage;
}

void CoverageTracker::markBranchVisited(ExecutionState *visitedTrue,
                                        ExecutionState *visitedFalse) {
  if (TrackBranchCoverage)
    return;
  unsigned id = theStatisticManager->getIndex();
  uint64_t hasTrue =
      theStatisticManager->getIndexedValue(stats::trueBranches, id);
  uint64_t hasFalse =
      theStatisticManager->getIndexedValue(stats::falseBranches, id);
  if (visitedTrue && !hasTrue) {
    visitedTrue->coveredNew = true;
    visitedTrue->instsSinceCovNew = 1;
    ++stats::trueBranches;
    if (hasFalse) {
      ++fullBranches;
      --partialBranches;
    } else
      ++partialBranches;
    hasTrue = 1;
  }
  if (visitedFalse && !hasFalse) {
    visitedFalse->coveredNew = true;
    visitedFalse->instsSinceCovNew = 1;
    ++stats::falseBranches;
    if (hasTrue) {
      ++fullBranches;
      --partialBranches;
    } else
      ++partialBranches;
  }
}

void CoverageTracker::updateFrameCoverage(ExecutionState &es) {
  if (!automaticDistanceUpdate)
    return;
  StackFrame &sf = es.stack.back();

  // Get DistanceToUncovered of parent stackframe if available
  uint64_t minDistAtRA =
      es.stack.size() > 1
          ? es.stack[es.stack.size() - 2].minDistToUncoveredOnReturn
          : 0;

  sf.minDistToUncoveredOnReturn =
      sf.caller ? computeMinDistToUncovered(sf.caller, minDistAtRA) : 0;
}
