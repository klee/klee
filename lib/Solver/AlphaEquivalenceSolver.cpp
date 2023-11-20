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
                             AlphaBuilder &builder);
  std::vector<const Array *>
  changeVersion(const std::vector<const Array *> &objects,
                AlphaBuilder &builder);
  Assignment
  changeVersion(const std::vector<const Array *> &objects,
                const std::vector<SparseStorage<unsigned char>> &values,
                const ArrayCache::ArrayHashMap<const Array *> &reverse);
  Assignment
  changeVersion(const Assignment &a,
                const ArrayCache::ArrayHashMap<const Array *> &reverse);
  ref<SolverResponse> changeVersion(ref<SolverResponse> res,
                                    AlphaBuilder &builder);
};

ValidityCore
AlphaEquivalenceSolver::changeVersion(const ValidityCore &validityCore,
                                      AlphaBuilder &builder) {
  ValidityCore reverseValidityCore;
  if (isa<ConstantExpr>(validityCore.expr)) {
    reverseValidityCore.expr = validityCore.expr;
  } else {
    reverseValidityCore.expr = builder.reverseBuild(validityCore.expr);
  }
  for (auto e : validityCore.constraints) {
    reverseValidityCore.constraints.insert(builder.reverseBuild(e));
  }
  return reverseValidityCore;
}

Assignment AlphaEquivalenceSolver::changeVersion(
    const std::vector<const Array *> &objects,
    const std::vector<SparseStorage<unsigned char>> &values,
    const ArrayCache::ArrayHashMap<const Array *> &reverse) {
  std::vector<const Array *> reverseObjects;
  std::vector<SparseStorage<unsigned char>> reverseValues;
  for (size_t i = 0; i < objects.size(); i++) {
    if (reverse.count(objects.at(i)) != 0) {
      reverseObjects.push_back(reverse.at(objects.at(i)));
      reverseValues.push_back(values.at(i));
    }
  }
  return Assignment(reverseObjects, reverseValues);
}

std::vector<const Array *>
AlphaEquivalenceSolver::changeVersion(const std::vector<const Array *> &objects,
                                      AlphaBuilder &builder) {
  std::vector<const Array *> reverseObjects;
  for (size_t i = 0; i < objects.size(); i++) {
    reverseObjects.push_back(builder.buildArray(objects.at(i)));
  }
  return reverseObjects;
}

Assignment AlphaEquivalenceSolver::changeVersion(
    const Assignment &a,
    const ArrayCache::ArrayHashMap<const Array *> &reverse) {
  std::vector<const Array *> objects = a.keys();
  std::vector<SparseStorage<unsigned char>> values = a.values();
  return changeVersion(objects, values, reverse);
}

ref<SolverResponse>
AlphaEquivalenceSolver::changeVersion(ref<SolverResponse> res,
                                      AlphaBuilder &builder) {
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
    validityCore = changeVersion(validityCore, builder);
    reverseRes = new ValidResponse(validityCore);
  }
  return reverseRes;
}

bool AlphaEquivalenceSolver::computeValidity(const Query &query,
                                             PartialValidity &result) {
  AlphaBuilder builder(arrayCache);
  constraints_ty alphaQuery = builder.visitConstraints(query.constraints.cs());
  ref<Expr> alphaQueryExpr = builder.build(query.expr);
  return solver->impl->computeValidity(
      Query(ConstraintSet(alphaQuery, {}, {}), alphaQueryExpr, query.id),
      result);
}

bool AlphaEquivalenceSolver::computeTruth(const Query &query, bool &isValid) {
  AlphaBuilder builder(arrayCache);
  constraints_ty alphaQuery = builder.visitConstraints(query.constraints.cs());
  ref<Expr> alphaQueryExpr = builder.build(query.expr);
  return solver->impl->computeTruth(
      Query(ConstraintSet(alphaQuery, {}, {}), alphaQueryExpr, query.id),
      isValid);
}

bool AlphaEquivalenceSolver::computeValue(const Query &query,
                                          ref<Expr> &result) {
  AlphaBuilder builder(arrayCache);
  constraints_ty alphaQuery = builder.visitConstraints(query.constraints.cs());
  ref<Expr> alphaQueryExpr = builder.build(query.expr);
  return solver->impl->computeValue(
      Query(ConstraintSet(alphaQuery, {}, {}), alphaQueryExpr, query.id),
      result);
}

bool AlphaEquivalenceSolver::computeInitialValues(
    const Query &query, const std::vector<const Array *> &objects,
    std::vector<SparseStorage<unsigned char>> &values, bool &hasSolution) {
  AlphaBuilder builder(arrayCache);
  constraints_ty alphaQuery = builder.visitConstraints(query.constraints.cs());
  ref<Expr> alphaQueryExpr = builder.build(query.expr);
  const std::vector<const Array *> newObjects = changeVersion(objects, builder);

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
  ref<Expr> alphaQueryExpr = builder.build(query.expr);
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
  ref<Expr> alphaQueryExpr = builder.build(query.expr);
  if (!solver->impl->computeValidityCore(
          Query(ConstraintSet(alphaQuery, {}, {}), alphaQueryExpr, query.id),
          validityCore, isValid)) {
    return false;
  }
  validityCore = changeVersion(validityCore, builder);
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
