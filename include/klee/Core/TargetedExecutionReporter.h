//===-- TargetedExecutionReporter.h ------------------------------*- C++-*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_TARGETEDEXECUTIONREPORTER_H
#define KLEE_TARGETEDEXECUTIONREPORTER_H

#include "klee/Module/SarifReport.h"

#include <unordered_set>

namespace klee {

namespace confidence {
using ty = double;
static ty MinConfidence = 0.0;
static ty MaxConfidence = 100.0;
static ty Confident = 90.0;
static ty VeryConfident = 99.0;
bool isConfident(ty conf);
bool isVeryConfident(ty conf);
bool isNormal(ty conf);
std::string toString(ty conf);
ty min(ty left, ty right);
}; // namespace confidence

void reportFalsePositive(confidence::ty confidence,
                         const std::set<ReachWithError> &errors, unsigned id,
                         std::string whatToIncrease);

} // namespace klee

#endif
