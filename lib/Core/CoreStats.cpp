//===-- CoreStats.cpp -----------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "CoreStats.h"
#include "klee/Support/ErrorHandling.h"


using namespace klee;

Statistic stats::allocations("Allocations", "Alloc");
Statistic stats::coveredInstructions("CoveredInstructions", "Icov");
Statistic stats::externalCalls("ExternalCalls", "ExtC");
Statistic stats::falseBranches("FalseBranches", "Bf");
Statistic stats::forkTime("ForkTime", "Ftime");
Statistic stats::forks("Forks", "Forks");
Statistic stats::inhibitedForks("InhibitedForks", "InhibForks");
Statistic stats::instructionRealTime("InstructionRealTimes", "Ireal");
Statistic stats::instructionTime("InstructionTimes", "Itime");
Statistic stats::instructions("Instructions", "I");
Statistic stats::minDistToReturn("MinDistToReturn", "Rdist");
Statistic stats::minDistToUncovered("MinDistToUncovered", "UCdist");
Statistic stats::resolveTime("ResolveTime", "Rtime");
Statistic stats::solverTime("SolverTime", "Stime");
Statistic stats::states("States", "States");
Statistic stats::trueBranches("TrueBranches", "Bt");
Statistic stats::uncoveredInstructions("UncoveredInstructions", "Iuncov");


// branch stats and setter

#undef BTYPE
#define BTYPE(Name,I) Statistic stats::branches ## Name("Branches"#Name, "Br"#Name);
BRANCH_TYPES

void stats::incBranchStat(BranchType reason, std::uint32_t value) {
#undef BTYPE
#define BTYPE(N,I) case BranchType::N : stats::branches ## N += value; break;
  switch (reason) {
    BRANCH_TYPES
  default:
    klee_error("Illegal branch type in incBranchStat(): %hhu",
               static_cast<std::uint8_t>(reason));
  }
}


// termination types

#undef TCLASS
#define TCLASS(Name,I) Statistic stats::termination ## Name("Termination"#Name, "Trm"#Name);
TERMINATION_CLASSES
