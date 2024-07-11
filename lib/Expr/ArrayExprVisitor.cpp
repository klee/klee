//===-- ArrayExprVisitor.cpp ----------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Expr/ArrayExprVisitor.h"

using namespace klee;

//------------------------------ HELPER FUNCTIONS ---------------------------//

void ArrayExprHelper::collectAlternatives(
    const SelectExpr &se, std::vector<ref<Expr>> &alternatives) {
  if (isa<SelectExpr>(se.trueExpr)) {
    collectAlternatives(*cast<SelectExpr>(se.trueExpr), alternatives);
  } else {
    alternatives.push_back(se.trueExpr);
  }
  if (isa<SelectExpr>(se.falseExpr)) {
    collectAlternatives(*cast<SelectExpr>(se.falseExpr), alternatives);
  } else {
    alternatives.push_back(se.falseExpr);
  }
}

//--------------------------- INDEX-BASED OPTIMIZATION-----------------------//
ExprVisitor::Action
ConstantArrayExprVisitor::visitConcat(const ConcatExpr &ce) {
  ref<ReadExpr> base = ce.hasOrderedReads();
  return base ? visitRead(*base) : Action::doChildren();
}

ExprVisitor::Action ConstantArrayExprVisitor::visitRead(const ReadExpr &re) {
  // It is an interesting ReadExpr if it contains a concrete array
  // that is read at a symbolic index
  if (re.updates.root->isConstantArray() && !isa<ConstantExpr>(re.index)) {
    for (const auto *un = re.updates.head.get(); un; un = un->next.get()) {
      if (!isa<ConstantExpr>(un->index) || !isa<ConstantExpr>(un->value)) {
        incompatible = true;
        return Action::skipChildren();
      }
    }
    IndexCompatibilityExprVisitor compatible;
    compatible.visit(re.index);
    if (compatible.isCompatible()) {
      if (arrays.count(re.updates.root) > 0) {
        const auto &indices = arrays[re.updates.root];
        if (!indices.empty() && indices.front() != re.index) {
          // Another possible index to resolve, currently unsupported
          incompatible = true;
          return Action::skipChildren();
        }
      }
      arrays[re.updates.root].push_back(re.index);
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

//-------------------------- VALUE-BASED OPTIMIZATION------------------------//
ExprVisitor::Action ArrayReadExprVisitor::visitConcat(const ConcatExpr &ce) {
  ref<ReadExpr> base = ce.hasOrderedReads();
  if (base) {
    return inspectRead(const_cast<ConcatExpr *>(&ce), ce.getWidth(), *base);
  }
  return Action::doChildren();
}
ExprVisitor::Action ArrayReadExprVisitor::visitRead(const ReadExpr &re) {
  return inspectRead(const_cast<ReadExpr *>(&re), re.getWidth(), re);
}
// This method is a mess because I want to avoid looping over the UpdateList
// values twice
ExprVisitor::Action ArrayReadExprVisitor::inspectRead(ref<Expr> hash,
                                                      Expr::Width width,
                                                      const ReadExpr &re) {
  // pre(*): index is symbolic
  if (!isa<ConstantExpr>(re.index) && re.updates.root->getRange() == 8) {
    if (readInfo.find(&re) == readInfo.end()) {
      if (re.updates.root->isSymbolicArray() && !re.updates.head) {
        return Action::doChildren();
      }
      if (re.updates.head) {
        // Check preconditions on UpdateList nodes
        bool hasConcreteValues = false;
        for (const auto *un = re.updates.head.get(); un; un = un->next.get()) {
          // Symbolic case - \inv(update): index is concrete
          if (!isa<ConstantExpr>(un->index)) {
            incompatible = true;
            break;
          } else if (!isa<ConstantExpr>(un->value)) {
            // We tell the optimization that there is a symbolic value,
            // otherwise we rely on the concrete optimization procedure
            symbolic = true;
          } else if (re.updates.root->isSymbolicArray() &&
                     isa<ConstantExpr>(un->value)) {
            // We can optimize symbolic array, but only if they have
            // at least one concrete value
            hasConcreteValues = true;
          }
        }
        // Symbolic case - if array is symbolic, then we need at least one
        // concrete value
        if (re.updates.root->isSymbolicArray()) {
          if (hasConcreteValues)
            symbolic = true;
          else
            incompatible = true;
        }

        if (incompatible)
          return Action::skipChildren();
      }
      symbolic |= re.updates.root->isSymbolicArray();
      reads.push_back(&re);
      readInfo.emplace(&re, std::make_pair(hash, width));
    }
    return Action::skipChildren();
  }
  return Action::doChildren();
}

ExprVisitor::Action
ArrayValueOptReplaceVisitor::visitConcat(const ConcatExpr &ce) {
  auto found = optimized.find(const_cast<ConcatExpr *>(&ce));
  if (found != optimized.end()) {
    return Action::changeTo((*found).second.get());
  }
  return Action::doChildren();
}
ExprVisitor::Action ArrayValueOptReplaceVisitor::visitRead(const ReadExpr &re) {
  auto found = optimized.find(const_cast<ReadExpr *>(&re));
  if (found != optimized.end()) {
    return Action::changeTo((*found).second.get());
  }
  return Action::doChildren();
}
