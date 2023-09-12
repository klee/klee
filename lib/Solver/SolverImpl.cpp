//===-- SolverImpl.cpp ----------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Solver/SolverImpl.h"
#include "klee/Expr/Constraints.h"
#include "klee/Expr/Expr.h"
#include "klee/Solver/Solver.h"
#include "klee/Support/ErrorHandling.h"

using namespace klee;

SolverImpl::~SolverImpl() {}

bool SolverImpl::computeValidity(const Query &query, PartialValidity &result) {
  bool isTrue, isFalse;
  if (!computeTruth(query, isTrue))
    return false;
  if (isTrue) {
    result = PValidity::MustBeTrue;
  } else {
    if (!computeTruth(query.negateExpr(), isFalse))
      return false;
    result = isFalse ? PValidity::MustBeFalse : PValidity::TrueOrFalse;
  }
  return true;
}

bool SolverImpl::computeValidity(const Query &query,
                                 ref<SolverResponse> &queryResult,
                                 ref<SolverResponse> &negatedQueryResult) {
  if (!check(query, queryResult))
    return false;
  if (!check(query.negateExpr(), negatedQueryResult))
    return false;
  return true;
}

bool SolverImpl::check(const Query &query, ref<SolverResponse> &result) {
  std::vector<const Array *> objects;
  findSymbolicObjects(query, objects);
  std::vector<SparseStorage<unsigned char>> values;

  bool hasSolution;

  if (!computeInitialValues(query, objects, values, hasSolution)) {
    return false;
  }

  if (hasSolution) {
    result = new InvalidResponse(objects, values);
  } else {
    result = new ValidResponse(ValidityCore(
        ValidityCore::constraints_typ(query.constraints.cs().begin(),
                                      query.constraints.cs().end()),
        query.expr));
  }
  return true;
}

bool SolverImpl::computeValidityCore(const Query &query,
                                     ValidityCore &validityCore,
                                     bool &isValid) {
  if (!computeTruth(query, isValid)) {
    return false;
  }

  if (isValid) {
    validityCore = ValidityCore(
        ValidityCore::constraints_typ(query.constraints.cs().begin(),
                                      query.constraints.cs().end()),
        query.expr);
  }
  return true;
}

bool SolverImpl::computeMinimalUnsignedValue(const Query &query,
                                             ref<ConstantExpr> &result) {
  bool mustBeTrue;

  // Fast path check
  auto fastexpr = EqExpr::createIsZero(query.expr);
  if (auto ce = dyn_cast<ConstantExpr>(fastexpr)) {
    if (ce->isTrue()) {
      result = ConstantExpr::create(0, query.expr->getWidth());
      return true;
    }
  } else {
    if (!computeTruth(query.withExpr(fastexpr).negateExpr(), mustBeTrue)) {
      return false;
    }
    if (!mustBeTrue) {
      result = ConstantExpr::create(0, query.expr->getWidth());
      return true;
    }
  }

  // At least one value must satisfy constraints
  ref<ConstantExpr> left = ConstantExpr::create(0, query.expr->getWidth());
  ref<ConstantExpr> right = ConstantExpr::create(1, query.expr->getWidth());

  /* It is a good idea to find the appropriate bounds to start.
  To do this we need to find x, such that if 'expr > 2**x', then
  number of queries will be around x+-1 to find the upperbound. So if
  we solve 'log log(2**base - 2**x) == x', we will find optimal 'x'. */

  /* In other words we solve 2**base = 2**x + 2**(2**x). As function
  in the right part grows monotonously, we can use binary search
  to approximate solution, or just pick x = log_2(base)
  as 2**x == o(2**(2**x)). */

  assert(query.expr->getWidth() > 0);
  bool isHighRightBound = false;
  int rightmostWidthBit = sizeof(unsigned long long) * CHAR_BIT - 1 -
                          __builtin_clzll(query.expr->getWidth());
  ref<Expr> inequalityValueOptimizatonExpr = ShlExpr::create(
      ConstantExpr::create(1, query.expr->getWidth()),
      ConstantExpr::create(rightmostWidthBit, query.expr->getWidth()));

  if (!computeTruth(query.withExpr(UgtExpr::create(
                        query.expr, inequalityValueOptimizatonExpr)),
                    isHighRightBound)) {
    return false;
  }

  if (isHighRightBound) {
    left = ConstantExpr::create(rightmostWidthBit, query.expr->getWidth());
    right =
        ConstantExpr::create(query.expr->getWidth(), query.expr->getWidth());
    /* while (left + 1 < right) */
    while (cast<ConstantExpr>(
               UltExpr::create(AddExpr::create(left, ConstantExpr::create(
                                                         1, left->getWidth())),
                               right))
               ->isTrue()) {
      ref<ConstantExpr> middle =
          LShrExpr::create(AddExpr::create(left, right),
                           ConstantExpr::create(1, left->getWidth()));
      ref<Expr> valueMustBeGreaterExpr =
          Simplificator::simplifyExpr(
              query.constraints,
              UgtExpr::create(
                  query.expr,
                  ShlExpr::create(ConstantExpr::create(1, middle->getWidth()),
                                  middle)))
              .simplified;

      bool mustBeGreater;
      if (!computeTruth(query.withExpr(valueMustBeGreaterExpr),
                        mustBeGreater)) {
        return false;
      }

      if (mustBeGreater) {
        left = middle;
      } else {
        right = middle;
      }
    }
    left = ShlExpr::create(ConstantExpr::create(1, left->getWidth()), left);
    /* Special case for overflow. */
    if (EqExpr::create(
            right, ConstantExpr::create(right->getWidth(), right->getWidth()))
            ->isTrue()) {
      right = SubExpr::create(
          ShlExpr::create(ConstantExpr::create(1, right->getWidth()), right),
          ConstantExpr::create(1, right->getWidth()));
    } else {
      right =
          ShlExpr::create(ConstantExpr::create(1, right->getWidth()), right);
    }
  } else {
    // Compute the right border
    bool firstIteration = true;
    do {
      if (!firstIteration) {
        left = right;
        right = cast<ConstantExpr>(
            ShlExpr::create(right, ConstantExpr::create(1, right->getWidth())));
      }
      ref<Expr> valueMustBeGreaterExpr =
          Simplificator::simplifyExpr(query.constraints,
                                      UgtExpr::create(query.expr, right))
              .simplified;
      if (!computeTruth(query.withExpr(valueMustBeGreaterExpr), mustBeTrue)) {
        return false;
      }
      firstIteration = false;
    } while (mustBeTrue);
  }

  // Binary search the least value for expr from the given query
  while (cast<ConstantExpr>(
             UltExpr::create(AddExpr::create(left, ConstantExpr::create(
                                                       1, left->getWidth())),
                             right))
             ->isTrue()) {
    ref<ConstantExpr> middle = cast<ConstantExpr>(ZExtExpr::create(
        LShrExpr::create(
            AddExpr::create(ZExtExpr::create(left, left->getWidth() + 1),
                            ZExtExpr::create(right, right->getWidth() + 1)),
            ConstantExpr::create(1, right->getWidth() + 1)),
        right->getWidth()));
    ref<Expr> valueMustBeGreaterExpr =
        Simplificator::simplifyExpr(query.constraints,
                                    UgtExpr::create(query.expr, middle))
            .simplified;

    bool mustBeGreater;
    if (!computeTruth(query.withExpr(valueMustBeGreaterExpr), mustBeGreater)) {
      return false;
    }

    if (mustBeGreater) {
      left = middle;
    } else {
      right = middle;
    }
  }

  result = right;
  return true;
}

const char *SolverImpl::getOperationStatusString(SolverRunStatus statusCode) {
  switch (statusCode) {
  case SOLVER_RUN_STATUS_SUCCESS_SOLVABLE:
    return "OPERATION SUCCESSFUL, QUERY IS SOLVABLE";
  case SOLVER_RUN_STATUS_SUCCESS_UNSOLVABLE:
    return "OPERATION SUCCESSFUL, QUERY IS UNSOLVABLE";
  case SOLVER_RUN_STATUS_FAILURE:
    return "OPERATION FAILED";
  case SOLVER_RUN_STATUS_TIMEOUT:
    return "SOLVER TIMEOUT";
  case SOLVER_RUN_STATUS_FORK_FAILED:
    return "FORK FAILED";
  case SOLVER_RUN_STATUS_INTERRUPTED:
    return "SOLVER PROCESS INTERRUPTED";
  case SOLVER_RUN_STATUS_UNEXPECTED_EXIT_CODE:
    return "UNEXPECTED SOLVER PROCESS EXIT CODE";
  case SOLVER_RUN_STATUS_WAITPID_FAILED:
    return "WAITPID FAILED FOR SOLVER PROCESS";
  default:
    return "UNRECOGNIZED OPERATION STATUS";
  }
}
