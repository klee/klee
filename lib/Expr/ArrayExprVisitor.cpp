#include "klee/util/ArrayExprVisitor.h"

#include "klee/Internal/Support/ErrorHandling.h"

#include <algorithm>

using namespace klee;

//------------------------------ HELPER FUNCTIONS ---------------------------//
bool ArrayExprHelper::isReadExprAtOffset(ref<Expr> e, const ReadExpr *base,
                                         ref<Expr> offset) {
  const ReadExpr *re = dyn_cast<ReadExpr>(e.get());
  if (!re || (re->getWidth() != Expr::Int8))
    return false;
  return SubExpr::create(re->index, base->index) == offset;
}

ReadExpr *ArrayExprHelper::hasOrderedReads(const ConcatExpr &ce) {
  const ReadExpr *base = dyn_cast<ReadExpr>(ce.getKid(0));

  // right now, all Reads are byte reads but some
  // transformations might change this
  if (!base || base->getWidth() != Expr::Int8)
    return NULL;

  // Get stride expr in proper index width.
  Expr::Width idxWidth = base->index->getWidth();
  ref<Expr> strideExpr = ConstantExpr::alloc(-1, idxWidth);
  ref<Expr> offset = ConstantExpr::create(0, idxWidth);

  ref<Expr> e = ce.getKid(1);

  // concat chains are unbalanced to the right
  while (e->getKind() == Expr::Concat) {
    offset = AddExpr::create(offset, strideExpr);
    if (!isReadExprAtOffset(e->getKid(0), base, offset))
      return NULL;
    e = e->getKid(1);
  }

  offset = AddExpr::create(offset, strideExpr);
  if (!isReadExprAtOffset(e, base, offset))
    return NULL;

  return cast<ReadExpr>(e.get());
}

//--------------------------- INDEX-BASED OPTIMIZATION-----------------------//
ExprVisitor::Action
ConstantArrayExprVisitor::visitConcat(const ConcatExpr &ce) {
  ReadExpr *base = ArrayExprHelper::hasOrderedReads(ce);
  if (base) {
    // It is an interesting ReadExpr if it contains a concrete array
    // that is read at a symbolic index
    if (base->updates.root->isConstantArray() &&
        !isa<ConstantExpr>(base->index)) {
      for (const UpdateNode *un = base->updates.head; un; un = un->next) {
        if (!isa<ConstantExpr>(un->index) || !isa<ConstantExpr>(un->value)) {
          incompatible = true;
          return Action::skipChildren();
        }
      }
      IndexCompatibilityExprVisitor compatible;
      compatible.visit(base->index);
      if (compatible.isCompatible() &&
          addedIndexes.find(base->index.get()->hash()) == addedIndexes.end()) {
        if (arrays.find(base->updates.root) == arrays.end()) {
          arrays.insert(
              std::make_pair(base->updates.root, std::vector<ref<Expr> >()));
        } else {
          // Another possible index to resolve, currently unsupported
          incompatible = true;
          return Action::skipChildren();
        }
        arrays.find(base->updates.root)->second.push_back(base->index);
        addedIndexes.insert(base->index.get()->hash());
      } else if (compatible.hasInnerReads()) {
        // This Read has an inner Read, we want to optimize the inner one
        // to create a cascading effect during assignment evaluation
        return Action::doChildren();
      }
      return Action::skipChildren();
    }
  }
  return Action::doChildren();
}
ExprVisitor::Action ConstantArrayExprVisitor::visitRead(const ReadExpr &re) {
  // It is an interesting ReadExpr if it contains a concrete array
  // that is read at a symbolic index
  if (re.updates.root->isConstantArray() && !isa<ConstantExpr>(re.index)) {
    for (const UpdateNode *un = re.updates.head; un; un = un->next) {
      if (!isa<ConstantExpr>(un->index) || !isa<ConstantExpr>(un->value)) {
        incompatible = true;
        return Action::skipChildren();
      }
    }
    IndexCompatibilityExprVisitor compatible;
    compatible.visit(re.index);
    if (compatible.isCompatible() &&
        addedIndexes.find(re.index.get()->hash()) == addedIndexes.end()) {
      if (arrays.find(re.updates.root) == arrays.end()) {
        arrays.insert(
            std::make_pair(re.updates.root, std::vector<ref<Expr> >()));
      } else {
        // Another possible index to resolve, currently unsupported
        incompatible = true;
        return Action::skipChildren();
      }
      arrays.find(re.updates.root)->second.push_back(re.index);
      addedIndexes.insert(re.index.get()->hash());
    } else if (compatible.hasInnerReads()) {
      // This Read has an inner Read, we want to optimize the inner one
      // to create a cascading effect during assignment evaluation
      return Action::doChildren();
    }
    return Action::skipChildren();
  } else if (re.updates.root->isSymbolicArray()) {
    incompatible = true;
  }

  return Action::doChildren();
}

ExprVisitor::Action
IndexCompatibilityExprVisitor::visitRead(const ReadExpr &re) {
  if (re.updates.head) {
    compatible = false;
    return Action::skipChildren();
  } else if (re.updates.root->isConstantArray() &&
             !isa<ConstantExpr>(re.index)) {
    compatible = false;
    inner = true;
    return Action::skipChildren();
  }
  return Action::doChildren();
}
ExprVisitor::Action IndexCompatibilityExprVisitor::visitURem(const URemExpr &) {
  compatible = false;
  return Action::skipChildren();
}
ExprVisitor::Action IndexCompatibilityExprVisitor::visitSRem(const SRemExpr &) {
  compatible = false;
  return Action::skipChildren();
}
ExprVisitor::Action IndexCompatibilityExprVisitor::visitOr(const OrExpr &) {
  compatible = false;
  return Action::skipChildren();
}

ExprVisitor::Action
IndexTransformationExprVisitor::visitConcat(const ConcatExpr &ce) {
  if (ReadExpr *re = dyn_cast<ReadExpr>(ce.getKid(0))) {
    if (re->updates.root->hash() == array->hash() && width < ce.getWidth()) {
      if (width == Expr::InvalidWidth)
        width = ce.getWidth();
    }
  } else if (ReadExpr *re = dyn_cast<ReadExpr>(ce.getKid(1))) {
    if (re->updates.root->hash() == array->hash() && width < ce.getWidth()) {
      if (width == Expr::InvalidWidth)
        width = ce.getWidth();
    }
  }
  return Action::doChildren();
}
ExprVisitor::Action IndexTransformationExprVisitor::visitMul(const MulExpr &e) {
  if (isa<ConstantExpr>(e.getKid(0)))
    mul = e.getKid(0);
  else if (isa<ConstantExpr>(e.getKid(0)))
    mul = e.getKid(1);
  return Action::doChildren();
}
