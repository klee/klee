//===-- Constraints.cpp ---------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Expr/Constraints.h"

#include "klee/Expr/Assignment.h"
#include "klee/Expr/ExprUtil.h"
#include "klee/Expr/ExprVisitor.h"
#include "klee/Module/KModule.h"
#include "klee/Support/OptionCategories.h"

#include "llvm/IR/Function.h"
#include "llvm/Support/CommandLine.h"

#include <map>

using namespace klee;

namespace {
llvm::cl::opt<bool> RewriteEqualities(
    "rewrite-equalities",
    llvm::cl::desc("Rewrite existing constraints when an equality with a "
                   "constant is added (default=true)"),
    llvm::cl::init(true), llvm::cl::cat(SolvingCat));
} // namespace

class ExprReplaceVisitor : public ExprVisitor {
private:
  ref<Expr> src, dst;

public:
  ExprReplaceVisitor(const ref<Expr> &_src, const ref<Expr> &_dst)
      : src(_src), dst(_dst) {}

  Action visitExpr(const Expr &e) override {
    if (e == *src) {
      return Action::changeTo(dst);
    }
    return Action::doChildren();
  }

  Action visitExprPost(const Expr &e) override {
    if (e == *src) {
      return Action::changeTo(dst);
    }
    return Action::doChildren();
  }
};

class ExprReplaceVisitor2 : public ExprVisitor {
private:
  ExprHashMap<ref<Expr>> &replacements;
  ExprHashSet &conflictExpressions;
  Expr::States &result;

public:
  explicit ExprReplaceVisitor2(ExprHashMap<ref<Expr>> &_replacements,
                               ExprHashSet &_conflictExpressions,
                               Expr::States &_result)

      : ExprVisitor(true), replacements(_replacements),
        conflictExpressions(_conflictExpressions), result(_result) {}

  Action visitExprPost(const Expr &e) override {
    auto it = replacements.find(ref<Expr>(const_cast<Expr *>(&e)));
    if (it != replacements.end()) {
      ref<Expr> equality = EqExpr::create(it->first, it->second);
      conflictExpressions.insert(equality);
      return Action::changeTo(it->second);
    }
    return Action::doChildren();
  }

  ref<Expr> findConflict(const ref<Expr> &e) {
    ref<Expr> eSimplified = visit(e);
    result = Expr::States::Undefined;
    if (eSimplified->getWidth() != Expr::Bool ||
        !isa<ConstantExpr>(*eSimplified)) {
      conflictExpressions.clear();
      return eSimplified;
    }

    if (eSimplified->isTrue() == true) {
      result = Expr::States::True;
    }
    if (eSimplified->isFalse() == true) {
      result = Expr::States::False;
    }

    return eSimplified;
  }
};

bool ConstraintManager::rewriteConstraints(ExprVisitor &visitor) {
  ConstraintSet old;
  Assignment concretization = constraints.getConcretization();
  bool changed = false;

  std::swap(constraints, old);
  for (auto &ce : old) {
    ref<Expr> e = visitor.visit(ce);

    if (e != ce) {
      addConstraintInternal(e); // enable further reductions
      changed = true;
    } else {
      constraints.push_back(ce);
    }
  }

  constraints.updateConcretization(concretization);
  return changed;
}

ref<Expr> ConstraintManager::simplifyExpr(const ConstraintSet &constraints,
                                          const ref<Expr> &e) {
  ExprHashSet cE;
  Expr::States r;
  return simplifyExpr(constraints, e, cE, r);
}

ref<Expr> ConstraintManager::simplifyExpr(const ConstraintSet &constraints,
                                          const ref<Expr> &e,
                                          ExprHashSet &conflictExpressions,
                                          Expr::States &result) {

  if (isa<ConstantExpr>(e))
    return e;

  ExprHashMap<ref<Expr>> equalities;

  for (auto &constraint : constraints) {
    if (const EqExpr *ee = dyn_cast<EqExpr>(constraint)) {
      if (isa<ConstantExpr>(ee->left)) {
        equalities.insert(std::make_pair(ee->right, ee->left));
      } else {
        equalities.insert(
            std::make_pair(constraint, ConstantExpr::alloc(1, Expr::Bool)));
      }
    } else {
      equalities.insert(
          std::make_pair(constraint, ConstantExpr::alloc(1, Expr::Bool)));
    }
  }

  return ExprReplaceVisitor2(equalities, conflictExpressions, result)
      .findConflict(e);
}

void ConstraintManager::addConstraintInternal(const ref<Expr> &e) {
  // rewrite any known equalities and split Ands into different conjuncts

  switch (e->getKind()) {
  case Expr::Constant:
    assert(cast<ConstantExpr>(e)->isTrue() &&
           "attempt to add invalid (false) constraint");
    break;

    // split to enable finer grained independence and other optimizations
  case Expr::And: {
    BinaryExpr *be = cast<BinaryExpr>(e);
    addConstraintInternal(be->left);
    addConstraintInternal(be->right);
    break;
  }

  case Expr::Eq: {
    if (RewriteEqualities) {
      // XXX: should profile the effects of this and the overhead.
      // traversing the constraints looking for equalities is hardly the
      // slowest thing we do, but it is probably nicer to have a
      // ConstraintSet ADT which efficiently remembers obvious patterns
      // (byte-constant comparison).
      BinaryExpr *be = cast<BinaryExpr>(e);
      if (isa<ConstantExpr>(be->left)) {
        ExprReplaceVisitor visitor(be->right, be->left);
        rewriteConstraints(visitor);
      }
    }
    constraints.push_back(e);
    break;
  }

  default:
    constraints.push_back(e);
    break;
  }
}

void ConstraintManager::addConstraint(const ref<Expr> &e) {
  ref<Expr> simplified = simplifyExpr(constraints, e);
  addConstraintInternal(simplified);
}

void ConstraintManager::addConstraint(const ref<Expr> &e,
                                      const Assignment &symcretes) {
  addConstraint(e);
  constraints.updateConcretization(symcretes);
}

ConstraintManager::ConstraintManager(ConstraintSet &_constraints)
    : constraints(_constraints) {}

bool ConstraintSet::empty() const { return constraints.empty(); }

klee::ConstraintSet::constraint_iterator ConstraintSet::begin() const {
  return constraints.begin();
}

klee::ConstraintSet::constraint_iterator ConstraintSet::end() const {
  return constraints.end();
}

size_t ConstraintSet::size() const noexcept { return constraints.size(); }

ConstraintSet::ConstraintSet(constraints_ty cs)
    : constraints(std::move(cs)), concretization(Assignment(true)) {}

ConstraintSet::ConstraintSet(ExprHashSet cs)
    : constraints(), concretization(Assignment(true)) {
  constraints.insert(constraints.end(), cs.begin(), cs.end());
}

ConstraintSet::ConstraintSet()
    : constraints(), concretization(Assignment(true)) {}

void ConstraintSet::push_back(const ref<Expr> &e) { constraints.push_back(e); }

void ConstraintSet::updateConcretization(const Assignment &c) {
  concretization = c;
}

/**
 * @brief Copies the current constraint set and adds expression e.
 *
 * Ideally, this function should accept variadic arguments pack
 * and unpack them with fold expressions, but this feature availible only
 * from C++17.
 *
 * @return copied and modified constraint set.
 */
ConstraintSet ConstraintSet::withExpr(ref<Expr> e) const {
  ConstraintSet newConstraintSet = *this;
  newConstraintSet.push_back(e);
  return newConstraintSet;
}

void ConstraintSet::dump() const {
  llvm::errs() << "Constraints [\n";
  for (const auto &constraint : constraints)
    constraint->dump();

  llvm::errs() << "]\n";
}

std::vector<const Array *> ConstraintSet::gatherArrays() const {
  std::vector<const Array *> arrays;
  findObjects(constraints.begin(), constraints.end(), arrays);
  return arrays;
}

std::vector<const Array *> ConstraintSet::gatherSymcreteArrays() const {
  std::unordered_set<const Array *> arrays;
  // TODO: this can be replaced with LLVM library function llvm::copy_if
  // from LLVM X (>6) version?
  for (const Array *array : gatherArrays()) {
    if (array->source->isSymcrete()) {
      arrays.insert(array);
    }
  }
  return std::vector<const Array *>(arrays.begin(), arrays.end());
}

std::set<ref<Expr>> ConstraintSet::asSet() const {
  std::set<ref<Expr>> ret;
  for (auto i : constraints) {
    ret.insert(i);
  }
  return ret;
}

const Assignment &ConstraintSet::getConcretization() const {
  return concretization;
}
