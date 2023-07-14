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
extern ty MinConfidence;
extern ty MaxConfidence;
extern ty Confident;
extern ty VeryConfident;
bool isConfident(ty conf);
bool isVeryConfident(ty conf);
bool isNormal(ty conf);
std::string toString(ty conf);
ty min(ty left, ty right);
}; // namespace confidence

void reportFalsePositive(confidence::ty confidence,
                         const std::vector<ReachWithError> &errors,
                         const std::string &id, std::string whatToIncrease);

} // namespace klee

#endif
