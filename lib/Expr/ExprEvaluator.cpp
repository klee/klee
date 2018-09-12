//===-- ExprEvaluator.cpp -------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Expr/ExprEvaluator.h"

using namespace klee;

ExprVisitor::Action ExprEvaluator::evalRead(const UpdateList &ul,
                                            unsigned index) {
  for (auto un = ul.head; !un.isNull(); un = un->next) {
    ref<Expr> ui = visit(un->index);
    
    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(ui)) {
      if (CE->getZExtValue() == index)
        return Action::changeTo(visit(un->value));
    } else {
      // update index is unknown, so may or may not be index, we
      // cannot guarantee value. we can rewrite to read at this
      // version though (mostly for debugging).
      
      return Action::changeTo(ReadExpr::create(UpdateList(ul.root, un), 
                                               ConstantExpr::alloc(index, 
                                                                   ul.root->getDomain())));
    }
  }
  
  if (ul.root->isConstantArray() && index < ul.root->size)
    return Action::changeTo(ul.root->constantValues[index]);

  return Action::changeTo(getInitialValue(*ul.root, index));
}

ExprVisitor::Action ExprEvaluator::visitExpr(const Expr &e) {
  // Evaluate all constant expressions here, in case they weren't folded in
  // construction. Don't do this for reads though, because we want them to go to
  // the normal rewrite path.
  unsigned N = e.getNumKids();
  if (!N || isa<ReadExpr>(e))
    return Action::doChildren();

  for (unsigned i = 0; i != N; ++i)
    if (!isa<ConstantExpr>(e.getKid(i)))
      return Action::doChildren();

  ref<Expr> Kids[3];
  for (unsigned i = 0; i != N; ++i) {
    assert(i < 3);
    Kids[i] = e.getKid(i);
  }

  return Action::changeTo(e.rebuild(Kids));
}

ExprVisitor::Action ExprEvaluator::visitRead(const ReadExpr &re) {
  ref<Expr> v = visit(re.index);
  
  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(v)) {
    return evalRead(re.updates, CE->getZExtValue());
  } else {
    return Action::doChildren();
  }
}

// we need to check for div by zero during partial evaluation,
// if this occurs then simply ignore the 0 divisor and use the
// original expression.
ExprVisitor::Action ExprEvaluator::protectedDivOperation(const BinaryExpr &e) {
  ref<Expr> kids[2] = { visit(e.left),
                        visit(e.right) };

  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(kids[1]))
    if (CE->isZero())
      kids[1] = e.right;

  if (kids[0]!=e.left || kids[1]!=e.right) {
    return Action::changeTo(e.rebuild(kids));
  } else {
    return Action::skipChildren();
  }
}

ExprVisitor::Action ExprEvaluator::visitUDiv(const UDivExpr &e) { 
  return protectedDivOperation(e); 
}
ExprVisitor::Action ExprEvaluator::visitSDiv(const SDivExpr &e) { 
  return protectedDivOperation(e); 
}
ExprVisitor::Action ExprEvaluator::visitURem(const URemExpr &e) { 
  return protectedDivOperation(e); 
}
ExprVisitor::Action ExprEvaluator::visitSRem(const SRemExpr &e) { 
  return protectedDivOperation(e); 
}

ExprVisitor::Action ExprEvaluator::visitExprPost(const Expr& e) {
  // When evaluating an assignment we should fold NotOptimizedExpr
  // nodes so we can fully evaluate.
  if (e.getKind() == Expr::NotOptimized) {
    return Action::changeTo(static_cast<const NotOptimizedExpr&>(e).src);
  }
  return Action::skipChildren();
}
