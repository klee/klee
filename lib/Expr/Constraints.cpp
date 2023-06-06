//===-- Constraints.cpp ---------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Expr/Constraints.h"

#include "klee/Expr/ArrayExprVisitor.h"
#include "klee/Expr/Assignment.h"
#include "klee/Expr/Expr.h"
#include "klee/Expr/ExprHashMap.h"
#include "klee/Expr/ExprUtil.h"
#include "klee/Expr/ExprVisitor.h"
#include "klee/Expr/Path.h"
#include "klee/Expr/Symcrete.h"
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
  std::vector<std::reference_wrapper<const ExprHashMap<ref<Expr>>>>
      replacements;

public:
  explicit ExprReplaceVisitor2(const ExprHashMap<ref<Expr>> &_replacements)
      : ExprVisitor(true), replacements({_replacements}) {}

  Action visitExprPost(const Expr &e) override {
    for (auto i = replacements.rbegin(); i != replacements.rend(); i++) {
      if (i->get().count(ref<Expr>(const_cast<Expr *>(&e)))) {
        return Action::changeTo(i->get().at(ref<Expr>(const_cast<Expr *>(&e))));
      }
    }
    return Action::doChildren();
  }

  ref<Expr> processSelect(const SelectExpr &sexpr) {
    auto cond = visit(sexpr.cond);

    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(cond)) {
      return CE->isTrue() ? visit(sexpr.trueExpr) : visit(sexpr.falseExpr);
    }

    ExprHashMap<ref<Expr>> localReplacements;
    if (const EqExpr *ee = dyn_cast<EqExpr>(cond)) {
      if (isa<ConstantExpr>(ee->left)) {
        localReplacements.insert(std::make_pair(ee->right, ee->left));
      } else {
        localReplacements.insert(
            std::make_pair(cond, ConstantExpr::alloc(1, Expr::Bool)));
      }
    } else {
      localReplacements.insert(
          std::make_pair(cond, ConstantExpr::alloc(1, Expr::Bool)));
    }

    replacements.push_back(localReplacements);
    visited.pushFrame();
    auto trueExpr = visit(sexpr.trueExpr);
    visited.popFrame();
    replacements.pop_back();

    // Reuse for false branch replacements
    localReplacements.clear();
    localReplacements.insert(
        std::make_pair(cond, ConstantExpr::alloc(0, Expr::Bool)));

    replacements.push_back(localReplacements);
    visited.pushFrame();
    auto falseExpr = visit(sexpr.falseExpr);
    visited.popFrame();
    replacements.pop_back();

    ref<Expr> seres = SelectExpr::create(cond, trueExpr, falseExpr);

    auto res = visitExprPost(*seres.get());
    if (res.kind == Action::ChangeTo) {
      seres = res.argument;
    }
    return seres;
  }

  UpdateList processUpdateList(const UpdateList &updates) {
    UpdateList newUpdates = UpdateList(updates.root, 0);
    std::stack<ref<UpdateNode>> forward;

    for (auto it = updates.head; !it.isNull(); it = it->next) {
      forward.push(it);
    }

    while (!forward.empty()) {
      ref<UpdateNode> UNode = forward.top();
      forward.pop();
      ref<Expr> newIndex = visit(UNode->index);
      ref<Expr> newValue = visit(UNode->value);
      newUpdates.extend(newIndex, newValue);
    }
    return newUpdates;
  }

  ref<Expr> processRead(const ReadExpr &re) {
    UpdateList updates = processUpdateList(re.updates);

    ref<Expr> index = visit(re.index);
    ref<Expr> reres = ReadExpr::create(updates, index);
    Action res = visitExprPost(*reres.get());
    if (res.kind == Action::ChangeTo) {
      reres = res.argument;
    }
    return reres;
  }

  ref<Expr> processOrderedRead(const ConcatExpr &ce, const ReadExpr &base) {
    UpdateList updates = processUpdateList(base.updates);
    std::stack<ref<Expr>> forward;

    ref<ReadExpr> read = cast<ReadExpr>(ce.getLeft());
    ref<Expr> index = visit(read->index);
    ref<Expr> e = ce.getKid(1);
    forward.push(ReadExpr::create(updates, index));

    while (auto concat = dyn_cast<ConcatExpr>(e)) {
      read = cast<ReadExpr>(concat->getLeft());
      index = visit(read->index);
      forward.push(ReadExpr::create(updates, index));
      e = concat->getRight();
    }

    read = cast<ReadExpr>(e);
    index = visit(read->index);
    forward.push(ReadExpr::create(updates, index));

    ref<Expr> reres = forward.top();
    forward.pop();

    while (!forward.empty()) {
      ref<Expr> newRead = forward.top();
      forward.pop();
      reres = ConcatExpr::create(newRead, reres);
    }

    return reres;
  }

  Action visitSelect(const SelectExpr &sexpr) override {
    return Action::changeTo(processSelect(sexpr));
  }

  Action visitConcat(const ConcatExpr &concat) override {
    const ReadExpr *base = ArrayExprHelper::hasOrderedReads(concat);
    if (base) {
      return Action::changeTo(processOrderedRead(concat, *base));
    } else {
      return Action::doChildren();
    }
  }
  Action visitRead(const ReadExpr &re) override {
    return Action::changeTo(processRead(re));
  }
};

ConstraintSet::ConstraintSet(constraints_ty cs, symcretes_ty symcretes,
                             Assignment concretization)
    : _constraints(cs), _symcretes(symcretes), _concretization(concretization) {
}

ConstraintSet::ConstraintSet() : _concretization(Assignment(true)) {}

void ConstraintSet::addConstraint(ref<Expr> e, const Assignment &delta) {
  _constraints.insert(e);
  for (auto i : delta.bindings) {
    _concretization.bindings[i.first] = i.second;
  }
}

IDType Symcrete::idCounter = 0;

void ConstraintSet::addSymcrete(ref<Symcrete> s,
                                const Assignment &concretization) {
  _symcretes.insert(s);
  for (auto i : s->dependentArrays()) {
    _concretization.bindings[i] = concretization.bindings.at(i);
  }
}

bool ConstraintSet::isSymcretized(ref<Expr> expr) const {
  for (auto symcrete : _symcretes) {
    if (symcrete->symcretized == expr) {
      return true;
    }
  }
  return false;
}

void ConstraintSet::rewriteConcretization(const Assignment &a) {
  for (auto i : a.bindings) {
    if (concretization().bindings.count(i.first)) {
      _concretization.bindings[i.first] = i.second;
    }
  }
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
  newConstraintSet.addConstraint(e, {});
  return newConstraintSet;
}

void ConstraintSet::print(llvm::raw_ostream &os) const {
  os << "Constraints [\n";
  for (const auto &constraint : _constraints) {
    constraint->print(os);
    os << "\n";
  }

  os << "]\n";
  os << "Symcretes [\n";
  for (const auto &symcrete : _symcretes) {
    symcrete->symcretized->print(os);
    os << "\n";
  }
  os << "]\n";
}

void ConstraintSet::dump() const { this->print(llvm::errs()); }

const constraints_ty &ConstraintSet::cs() const { return _constraints; }

const symcretes_ty &ConstraintSet::symcretes() const { return _symcretes; }

const Path &PathConstraints::path() const { return _path; }

const ExprHashMap<Path::PathIndex> &PathConstraints::indexes() const {
  return pathIndexes;
}

const Assignment &ConstraintSet::concretization() const {
  return _concretization;
}

const constraints_ty &PathConstraints::original() const { return _original; }

const ExprHashMap<ExprHashSet> &PathConstraints::simplificationMap() const {
  return _simplificationMap;
}

const ConstraintSet &PathConstraints::cs() const { return constraints; }

const PathConstraints::ordered_constraints_ty &
PathConstraints::orderedCS() const {
  return orderedConstraints;
}

void PathConstraints::advancePath(KInstruction *ki) { _path.advance(ki); }

void PathConstraints::advancePath(const Path &path) {
  _path = Path::concat(_path, path);
}

void PathConstraints::addConstraint(ref<Expr> e, const Assignment &delta,
                                    Path::PathIndex currIndex) {
  auto expr = Simplificator::simplifyExpr(constraints, e);
  if (auto ce = dyn_cast<ConstantExpr>(expr)) {
    assert(ce->isTrue() && "Attempt to add invalid constraint");
    return;
  }
  std::vector<ref<Expr>> exprs;
  Simplificator::splitAnds(expr, exprs);
  for (auto expr : exprs) {
    if (auto ce = dyn_cast<ConstantExpr>(expr)) {
      assert(ce->isTrue() && "Expression simplified to false");
      return;
    }
    _original.insert(expr);
    pathIndexes.insert({expr, currIndex});
    _simplificationMap[expr].insert(expr);
    orderedConstraints[currIndex].insert(expr);
    constraints.addConstraint(expr, delta);
  }
  auto simplificator = Simplificator(constraints);
  constraints = simplificator.simplify();
  ExprHashMap<ExprHashSet> newMap;
  for (auto i : simplificator.getSimplificationMap()) {
    ExprHashSet newSet;
    for (auto j : i.second) {
      for (auto k : _simplificationMap[j]) {
        newSet.insert(k);
      }
    }
    newMap.insert({i.first, newSet});
  }
  _simplificationMap = newMap;
}

void PathConstraints::addConstraint(ref<Expr> e, const Assignment &delta) {
  addConstraint(e, delta, _path.getCurrentIndex());
}

bool PathConstraints::isSymcretized(ref<Expr> expr) const {
  return constraints.isSymcretized(expr);
}

void PathConstraints::addSymcrete(ref<Symcrete> s,
                                  const Assignment &concretization) {
  constraints.addSymcrete(s, concretization);
}

void PathConstraints::rewriteConcretization(const Assignment &a) {
  constraints.rewriteConcretization(a);
}

PathConstraints PathConstraints::concat(const PathConstraints &l,
                                        const PathConstraints &r) {
  // TODO : How to handle symcretes and concretization?
  PathConstraints path = l;
  path._path = Path::concat(l._path, r._path);
  auto offset = l._path.KBlockSize();
  for (const auto &i : r._original) {
    path._original.insert(i);
    auto index = r.pathIndexes.at(i);
    index.block += offset;
    path.pathIndexes.insert({i, index});
    path.orderedConstraints[index].insert(i);
  }
  for (const auto &i : r.constraints.cs()) {
    path.constraints.addConstraint(i, {});
    if (r._simplificationMap.count(i)) {
      path._simplificationMap.insert({i, r._simplificationMap.at(i)});
    }
  }
  // Run the simplificator on the newly constructed set?
  return path;
}

ref<Expr> Simplificator::simplifyExpr(const constraints_ty &constraints,
                                      const ref<Expr> &expr) {
  if (isa<ConstantExpr>(expr))
    return expr;

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

  return ExprReplaceVisitor2(equalities).visit(expr);
}

ref<Expr> Simplificator::simplifyExpr(const ConstraintSet &constraints,
                                      const ref<Expr> &expr) {
  return simplifyExpr(constraints.cs(), expr);
}

ConstraintSet Simplificator::simplify() {
  assert(!simplificationDone);
  using EqualityMap = ExprHashMap<std::pair<ref<Expr>, ref<Expr>>>;
  EqualityMap equalities;
  bool changed = true;
  ExprHashMap<constraints_ty> map;
  constraints_ty current;
  constraints_ty next = constraints.cs();

  for (auto e : next) {
    map.insert({e, {e}});
    if (e->getKind() == Expr::Eq) {
      auto be = cast<BinaryExpr>(e);
      if (isa<ConstantExpr>(be->left)) {
        equalities.insert({e, {be->right, be->left}});
      }
    }
  }

  while (changed) {
    changed = false;
    std::vector<EqualityMap::value_type> newEqualitites;
    for (auto eq : equalities) {
      auto visitor = ExprReplaceVisitor(eq.second.first, eq.second.second);
      current = next;
      next.clear();
      for (auto expr : current) {
        ref<Expr> simplifiedExpr;
        if (expr != eq.first && RewriteEqualities) {
          simplifiedExpr = visitor.visit(expr);
        } else {
          simplifiedExpr = expr;
        }
        if (simplifiedExpr != expr) {
          changed = true;
          constraints_ty mapEntry;
          if (auto ce = dyn_cast<ConstantExpr>(simplifiedExpr)) {
            assert(ce->isTrue() && "Constraint simplified to false");
            continue;
          }
          if (map.count(expr)) {
            mapEntry = map.at(expr);
          }
          mapEntry.insert(eq.first);
          std::vector<ref<Expr>> simplifiedExprs;
          splitAnds(simplifiedExpr, simplifiedExprs);
          for (auto simplifiedExpr : simplifiedExprs) {
            if (simplifiedExpr->getKind() == Expr::Eq) {
              auto be = cast<BinaryExpr>(simplifiedExpr);
              if (isa<ConstantExpr>(be->left)) {
                newEqualitites.push_back(
                    {simplifiedExpr, {be->right, be->left}});
              }
            }
          }
          for (auto simplifiedExpr : simplifiedExprs) {
            for (auto i : mapEntry) {
              map[simplifiedExpr].insert(i);
            }
            next.insert(simplifiedExpr);
          }
        } else {
          next.insert(expr);
        }
      }
    }
    for (auto equality : newEqualitites) {
      equalities.insert(equality);
    }
  }

  for (auto entry : map) {
    if (next.count(entry.first)) {
      simplificationMap.insert(entry);
    }
  }
  simplificationDone = true;
  return ConstraintSet(next, constraints.symcretes(),
                       constraints.concretization());
}

ExprHashMap<constraints_ty> &Simplificator::getSimplificationMap() {
  assert(simplificationDone);
  return simplificationMap;
}

void Simplificator::splitAnds(ref<Expr> e, std::vector<ref<Expr>> &exprs) {
  if (auto andExpr = dyn_cast<AndExpr>(e)) {
    splitAnds(andExpr->getKid(0), exprs);
    splitAnds(andExpr->getKid(1), exprs);
  } else {
    exprs.push_back(e);
  }
}

std::vector<const Array *> ConstraintSet::gatherArrays() const {
  std::vector<const Array *> arrays;
  findObjects(_constraints.begin(), _constraints.end(), arrays);
  return arrays;
}

std::vector<const Array *> ConstraintSet::gatherSymcretizedArrays() const {
  std::unordered_set<const Array *> arrays;
  for (const ref<Symcrete> &symcrete : _symcretes) {
    arrays.insert(symcrete->dependentArrays().begin(),
                  symcrete->dependentArrays().end());
  }
  return std::vector<const Array *>(arrays.begin(), arrays.end());
}
