//===-- AlphaEquivalenceSolver.cpp-----------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Expr/SymbolicSource.h"
#include "klee/Solver/SolverUtil.h"

#include "klee/Solver/Solver.h"

#include "klee/Expr/AlphaBuilder.h"
#include "klee/Expr/Assignment.h"
#include "klee/Expr/Constraints.h"
#include "klee/Expr/ExprHashMap.h"
#include "klee/Solver/SolverImpl.h"
#include "klee/Support/Debug.h"

#include "klee/Support/CompilerWarning.h"
DISABLE_WARNING_PUSH
DISABLE_WARNING_DEPRECATED_DECLARATIONS
#include "llvm/Support/Casting.h"
#include "llvm/Support/raw_ostream.h"
DISABLE_WARNING_POP

using namespace klee;
using namespace llvm;

class AlphaEquivalenceSolver : public SolverImpl {
private:
  std::unique_ptr<Solver> solver;
  ArrayCache &arrayCache;

public:
  AlphaEquivalenceSolver(std::unique_ptr<Solver> solver,
                         ArrayCache &_arrayCache)
      : solver(std::move(solver)), arrayCache(_arrayCache) {}

  bool computeTruth(const Query &, bool &isValid);
  bool computeValidity(const Query &, PartialValidity &result);
  bool computeValue(const Query &, ref<Expr> &result);
  bool computeInitialValues(const Query &query,
                            const std::vector<const Array *> &objects,
                            std::vector<SparseStorage<unsigned char>> &values,
                            bool &hasSolution);
  bool check(const Query &query, ref<SolverResponse> &result);
  bool computeValidityCore(const Query &query, ValidityCore &validityCore,
                           bool &isValid);
  SolverRunStatus getOperationStatusCode();
  char *getConstraintLog(const Query &);
  void setCoreSolverTimeout(time::Span timeout);
  void notifyStateTermination(std::uint32_t id);
  ValidityCore changeVersion(const ValidityCore &validityCore,
                             const ExprHashMap<ref<Expr>> &reverse);
  const std::vector<const Array *>
  changeVersion(const std::vector<const Array *> &objects,
                const ArrayCache::ArrayHashMap<const Array *> &reverse);
  Assignment
  changeVersion(const Assignment &a,
                const ArrayCache::ArrayHashMap<const Array *> &reverse);
  ref<SolverResponse> changeVersion(ref<SolverResponse> res,
                                    const AlphaBuilder &builder);
  ref<SolverResponse> createAlphaVersion(ref<SolverResponse> res,
                                         const AlphaBuilder &builder);
};

ValidityCore
AlphaEquivalenceSolver::changeVersion(const ValidityCore &validityCore,
                                      const ExprHashMap<ref<Expr>> &reverse) {
  ValidityCore reverseValidityCore;
  assert(reverse.find(validityCore.expr) != reverse.end());
  reverseValidityCore.expr = reverse.at(validityCore.expr);
  for (auto e : validityCore.constraints) {
    assert(reverse.find(e) != reverse.end());
    reverseValidityCore.constraints.insert(reverse.at(e));
  }
  return reverseValidityCore;
}

const std::vector<const Array *> AlphaEquivalenceSolver::changeVersion(
    const std::vector<const Array *> &objects,
    const ArrayCache::ArrayHashMap<const Array *> &reverse) {
  std::vector<const Array *> reverseObjects;
  for (auto it : objects) {
    reverseObjects.push_back(reverse.at(it));
  }
  return reverseObjects;
}

Assignment AlphaEquivalenceSolver::changeVersion(
    const Assignment &a,
    const ArrayCache::ArrayHashMap<const Array *> &reverse) {
  std::vector<const Array *> objects = a.keys();
  std::vector<SparseStorage<unsigned char>> values = a.values();
  objects = changeVersion(objects, reverse);
  return Assignment(objects, values);
}

ref<SolverResponse>
AlphaEquivalenceSolver::changeVersion(ref<SolverResponse> res,
                                      const AlphaBuilder &builder) {
  ref<SolverResponse> reverseRes;
  if (!isa<InvalidResponse>(res) && !isa<ValidResponse>(res)) {
    return res;
  }

  if (isa<InvalidResponse>(res)) {
    Assignment a = cast<InvalidResponse>(res)->initialValues();
    a = changeVersion(a, builder.reverseAlphaArrayMap);
    reverseRes = new InvalidResponse(a.bindings);
  } else {
    ValidityCore validityCore;
    res->tryGetValidityCore(validityCore);
    validityCore = changeVersion(validityCore, builder.reverseExprMap);
    reverseRes = new ValidResponse(validityCore);
  }
  return reverseRes;
}

ref<SolverResponse>
AlphaEquivalenceSolver::createAlphaVersion(ref<SolverResponse> res,
                                           const AlphaBuilder &builder) {
  if (!res || !isa<InvalidResponse>(res)) {
    return res;
  }

  Assignment a = cast<InvalidResponse>(res)->initialValues();
  changeVersion(a, builder.alphaArrayMap);
  return new InvalidResponse(a.bindings);
}

bool AlphaEquivalenceSolver::computeValidity(const Query &query,
                                             PartialValidity &result) {
  AlphaBuilder builder(arrayCache);
  constraints_ty alphaQuery = builder.visitConstraints(query.constraints.cs());
  ref<Expr> alphaQueryExpr = builder.visitExpr(query.expr);
  return solver->impl->computeValidity(
      Query(ConstraintSet(alphaQuery, {}, {}), alphaQueryExpr, query.id),
      result);
}

bool AlphaEquivalenceSolver::computeTruth(const Query &query, bool &isValid) {
  AlphaBuilder builder(arrayCache);
  constraints_ty alphaQuery = builder.visitConstraints(query.constraints.cs());
  ref<Expr> alphaQueryExpr = builder.visitExpr(query.expr);
  return solver->impl->computeTruth(
      Query(ConstraintSet(alphaQuery, {}, {}), alphaQueryExpr, query.id),
      isValid);
}

bool AlphaEquivalenceSolver::computeValue(const Query &query,
                                          ref<Expr> &result) {
  AlphaBuilder builder(arrayCache);
  constraints_ty alphaQuery = builder.visitConstraints(query.constraints.cs());
  ref<Expr> alphaQueryExpr = builder.visitExpr(query.expr);
  return solver->impl->computeValue(
      Query(ConstraintSet(alphaQuery, {}, {}), alphaQueryExpr, query.id),
      result);
}

bool AlphaEquivalenceSolver::computeInitialValues(
    const Query &query, const std::vector<const Array *> &objects,
    std::vector<SparseStorage<unsigned char>> &values, bool &hasSolution) {
  AlphaBuilder builder(arrayCache);
  constraints_ty alphaQuery = builder.visitConstraints(query.constraints.cs());
  ref<Expr> alphaQueryExpr = builder.visitExpr(query.expr);
  const std::vector<const Array *> newObjects =
      changeVersion(objects, builder.alphaArrayMap);

  if (!solver->impl->computeInitialValues(
          Query(ConstraintSet(alphaQuery, {}, {}), alphaQueryExpr, query.id),
          newObjects, values, hasSolution)) {
    return false;
  }
  return true;
}

bool AlphaEquivalenceSolver::check(const Query &query,
                                   ref<SolverResponse> &result) {
  AlphaBuilder builder(arrayCache);

  constraints_ty alphaQuery = builder.visitConstraints(query.constraints.cs());
  ref<Expr> alphaQueryExpr = builder.visitExpr(query.expr);
  result = createAlphaVersion(result, builder);
  if (!solver->impl->check(
          Query(ConstraintSet(alphaQuery, {}, {}), alphaQueryExpr, query.id),
          result)) {
    return false;
  }

  result = changeVersion(result, builder);
  return true;
}

bool AlphaEquivalenceSolver::computeValidityCore(const Query &query,
                                                 ValidityCore &validityCore,
                                                 bool &isValid) {
  AlphaBuilder builder(arrayCache);

  constraints_ty alphaQuery = builder.visitConstraints(query.constraints.cs());
  ref<Expr> alphaQueryExpr = builder.visitExpr(query.expr);
  if (!solver->impl->computeValidityCore(
          Query(ConstraintSet(alphaQuery, {}, {}), alphaQueryExpr, query.id),
          validityCore, isValid)) {
    return false;
  }
  validityCore = changeVersion(validityCore, builder.reverseExprMap);
  return true;
}

SolverImpl::SolverRunStatus AlphaEquivalenceSolver::getOperationStatusCode() {
  return solver->impl->getOperationStatusCode();
}

char *AlphaEquivalenceSolver::getConstraintLog(const Query &query) {
  return solver->impl->getConstraintLog(query);
}

void AlphaEquivalenceSolver::setCoreSolverTimeout(time::Span timeout) {
  solver->impl->setCoreSolverTimeout(timeout);
}

void AlphaEquivalenceSolver::notifyStateTermination(std::uint32_t id) {
  solver->impl->notifyStateTermination(id);
}

std::unique_ptr<Solver>
klee::createAlphaEquivalenceSolver(std::unique_ptr<Solver> s,
                                   ArrayCache &arrayCache) {
  return std::make_unique<Solver>(
      std::make_unique<AlphaEquivalenceSolver>(std::move(s), arrayCache));
}
