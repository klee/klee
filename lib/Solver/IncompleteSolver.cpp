//===-- IncompleteSolver.cpp ----------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Solver/IncompleteSolver.h"

#include "klee/Expr/Constraints.h"
#include "klee/Solver/SolverUtil.h"

#include <utility>

using namespace klee;
using namespace llvm;

/***/

PartialValidity IncompleteSolver::computeValidity(const Query &query) {
  PartialValidity trueResult = computeTruth(query);

  if (trueResult == PValidity::MustBeTrue) {
    return PValidity::MustBeTrue;
  } else {
    PartialValidity falseResult = computeTruth(query.negateExpr());

    if (falseResult == PValidity::MustBeTrue) {
      return PValidity::MustBeFalse;
    } else {
      bool trueCorrect = trueResult != PValidity::None,
           falseCorrect = falseResult != PValidity::None;

      if (trueCorrect && falseCorrect) {
        return PValidity::TrueOrFalse;
      } else if (trueCorrect) { // ==> trueResult == MayBeFalse
        return PValidity::MayBeFalse;
      } else if (falseCorrect) { // ==> falseResult == MayBeFalse
        return PValidity::MayBeTrue;
      } else {
        return PValidity::None;
      }
    }
  }
}

/***/

StagedSolverImpl::StagedSolverImpl(std::unique_ptr<IncompleteSolver> primary,
                                   std::unique_ptr<Solver> secondary)
    : primary(std::move(primary)), secondary(std::move(secondary)) {}

bool StagedSolverImpl::computeTruth(const Query &query, bool &isValid) {
  PartialValidity trueResult = primary->computeTruth(query);

  if (trueResult != PValidity::None) {
    isValid = (trueResult == PValidity::MustBeTrue);
    return true;
  }

  return secondary->impl->computeTruth(query, isValid);
}

bool StagedSolverImpl::computeValidity(const Query &query,
                                       PartialValidity &result) {
  bool tmp;

  switch (primary->computeValidity(query)) {
  case PValidity::MustBeTrue:
    result = PValidity::MustBeTrue;
    break;
  case PValidity::MustBeFalse:
    result = PValidity::MustBeFalse;
    break;
  case PValidity::TrueOrFalse:
    result = PValidity::TrueOrFalse;
    break;
  case PValidity::MayBeTrue:
    if (secondary->impl->computeTruth(query, tmp)) {

      result = tmp ? PValidity::MustBeTrue : PValidity::TrueOrFalse;
    } else {
      result = PValidity::MayBeTrue;
    }
    break;
  case PValidity::MayBeFalse:
    if (secondary->impl->computeTruth(query.negateExpr(), tmp)) {
      result = tmp ? PValidity::MustBeFalse : PValidity::TrueOrFalse;
    } else {
      result = PValidity::MayBeFalse;
    }
    break;
  default:
    if (!secondary->impl->computeValidity(query, result))
      return false;
    break;
  }

  return true;
}

bool StagedSolverImpl::computeValue(const Query &query, ref<Expr> &result) {
  if (primary->computeValue(query, result))
    return true;

  return secondary->impl->computeValue(query, result);
}

bool StagedSolverImpl::computeInitialValues(
    const Query &query, const std::vector<const Array *> &objects,
    std::vector<SparseStorage<unsigned char>> &values, bool &hasSolution) {
  if (primary->computeInitialValues(query, objects, values, hasSolution))
    return true;

  return secondary->impl->computeInitialValues(query, objects, values,
                                               hasSolution);
}

bool StagedSolverImpl::check(const Query &query, ref<SolverResponse> &result) {
  std::vector<const Array *> objects;
  findSymbolicObjects(query, objects);
  std::vector<SparseStorage<unsigned char>> values;

  bool hasSolution;

  bool primaryResult =
      primary->computeInitialValues(query, objects, values, hasSolution);
  if (primaryResult && hasSolution) {
    result = new InvalidResponse(objects, values);
    return true;
  }

  return secondary->impl->check(query, result);
}

bool StagedSolverImpl::computeValidityCore(const Query &query,
                                           ValidityCore &validityCore,
                                           bool &isValid) {
  return secondary->impl->computeValidityCore(query, validityCore, isValid);
}

SolverImpl::SolverRunStatus StagedSolverImpl::getOperationStatusCode() {
  return secondary->impl->getOperationStatusCode();
}

char *StagedSolverImpl::getConstraintLog(const Query &query) {
  return secondary->impl->getConstraintLog(query);
}

void StagedSolverImpl::setCoreSolverTimeout(time::Span timeout) {
  secondary->impl->setCoreSolverTimeout(timeout);
}

void StagedSolverImpl::notifyStateTermination(std::uint32_t id) {
  secondary->impl->notifyStateTermination(id);
}
