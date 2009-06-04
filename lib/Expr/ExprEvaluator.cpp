//===-- ExprEvaluator.cpp -------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/util/ExprEvaluator.h"

using namespace klee;

ExprVisitor::Action ExprEvaluator::evalRead(const UpdateList &ul,
                                            unsigned index) {
  for (const UpdateNode *un=ul.head; un; un=un->next) {
    ref<Expr> ui = visit(un->index);
    
    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(ui)) {
      if (CE->getConstantValue() == index)
        return Action::changeTo(visit(un->value));
    } else {
      // update index is unknown, so may or may not be index, we
      // cannot guarantee value. we can rewrite to read at this
      // version though (mostly for debugging).
      
      UpdateList fwd(ul.root, un, 0);
      return Action::changeTo(ReadExpr::create(fwd, 
                                               ConstantExpr::alloc(index, Expr::Int32)));
    }
  }
  
  return Action::changeTo(getInitialValue(*ul.root, index));
}

ExprVisitor::Action ExprEvaluator::visitRead(const ReadExpr &re) {
  ref<Expr> v = visit(re.index);
  
  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(v)) {
    return evalRead(re.updates, CE->getConstantValue());
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
    if (!CE->getConstantValue())
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
