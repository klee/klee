//===-- ExprRangeEvaluator.h ------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_EXPRRANGEEVALUATOR_H
#define KLEE_EXPRRANGEEVALUATOR_H

#include "klee/Expr.h"
#include "klee/util/Bits.h"

namespace klee {

/*
class ValueType {
public:
  ValueType(); // empty range
  ValueType(uint64_t value);
  ValueType(uint64_t min, uint64_t max);
  
  bool mustEqual(const uint64_t b);
  bool mustEqual(const ValueType &b);
  bool mayEqual(const uint64_t b);  
  bool mayEqual(const ValueType &b);

  bool isFullRange(unsigned width);

  ValueType set_union(ValueType &);
  ValueType set_intersection(ValueType &);
  ValueType set_difference(ValueType &);

  ValueType binaryAnd(ValueType &);
  ValueType binaryOr(ValueType &);
  ValueType binaryXor(ValueType &);
  ValueType concat(ValueType &, unsigned width);
  ValueType add(ValueType &, unsigned width);
  ValueType sub(ValueType &, unsigned width);
  ValueType mul(ValueType &, unsigned width);
  ValueType udiv(ValueType &, unsigned width);
  ValueType sdiv(ValueType &, unsigned width);
  ValueType urem(ValueType &, unsigned width);
  ValueType srem(ValueType &, unsigned width);

  uint64_t min();
  uint64_t max();
  int64_t minSigned(unsigned width);
  int64_t maxSigned(unsigned width);
}
*/

template<class T>
class ExprRangeEvaluator {
protected:
  /// getInitialReadRange - Return a range for the initial value of the given
  /// array (which may be constant), for the given range of indices.
  virtual T getInitialReadRange(const Array &os, T index) = 0;

  T evalRead(const UpdateList &ul, T index);

public:
  ExprRangeEvaluator() {}
  virtual ~ExprRangeEvaluator() {}

  T evaluate(const ref<Expr> &e);
};

template<class T>
T ExprRangeEvaluator<T>::evalRead(const UpdateList &ul,
                                  T index) {
  T res;

  for (const UpdateNode *un=ul.head; un; un=un->next) {
    T ui = evaluate(un->index);

    if (ui.mustEqual(index)) {
      return res.set_union(evaluate(un->value));
    } else if (ui.mayEqual(index)) {
      res = res.set_union(evaluate(un->value));
      if (res.isFullRange(8)) {
        return res;
      } 
    }
  }
  
  return res.set_union(getInitialReadRange(*ul.root, index));
}

template<class T>
T ExprRangeEvaluator<T>::evaluate(const ref<Expr> &e) {
  switch (e->getKind()) {
  case Expr::Constant:
    return T(cast<ConstantExpr>(e));

  case Expr::NotOptimized: 
    break;

  case Expr::Read: {
    const ReadExpr *re = cast<ReadExpr>(e);
    T index = evaluate(re->index);

    assert(re->updates.root && re->getWidth() == re->updates.root->range && "unexpected multibyte read");

    return evalRead(re->updates, index);
  }

  case Expr::Select: {
    const SelectExpr *se = cast<SelectExpr>(e);
    T cond = evaluate(se->cond);
      
    if (cond.mustEqual(1)) {
      return evaluate(se->trueExpr);
    } else if (cond.mustEqual(0)) {
      return evaluate(se->falseExpr);
    } else {
      return evaluate(se->trueExpr).set_union(evaluate(se->falseExpr));
    }
  }

    // XXX these should be unrolled to ensure nice inline
  case Expr::Concat: {
    const Expr *ep = e.get();
    T res(0);
    for (unsigned i=0; i<ep->getNumKids(); i++)
      res = res.concat(evaluate(ep->getKid(i)),8);
    return res;
  }

    // Arithmetic

  case Expr::Add: {
    const BinaryExpr *be = cast<BinaryExpr>(e);
    unsigned width = be->left->getWidth();
    return evaluate(be->left).add(evaluate(be->right), width);
  }
  case Expr::Sub: {
    const BinaryExpr *be = cast<BinaryExpr>(e);
    unsigned width = be->left->getWidth();
    return evaluate(be->left).sub(evaluate(be->right), width);
  }
  case Expr::Mul: {
    const BinaryExpr *be = cast<BinaryExpr>(e);
    unsigned width = be->left->getWidth();
    return evaluate(be->left).mul(evaluate(be->right), width);
  }
  case Expr::UDiv: {
    const BinaryExpr *be = cast<BinaryExpr>(e);
    unsigned width = be->left->getWidth();
    return evaluate(be->left).udiv(evaluate(be->right), width);
  }
  case Expr::SDiv: {
    const BinaryExpr *be = cast<BinaryExpr>(e);
    unsigned width = be->left->getWidth();
    return evaluate(be->left).sdiv(evaluate(be->right), width);
  }
  case Expr::URem: {
    const BinaryExpr *be = cast<BinaryExpr>(e);
    unsigned width = be->left->getWidth();
    return evaluate(be->left).urem(evaluate(be->right), width);
  }
  case Expr::SRem: {
    const BinaryExpr *be = cast<BinaryExpr>(e);
    unsigned width = be->left->getWidth();
    return evaluate(be->left).srem(evaluate(be->right), width);
  }

    // Binary

  case Expr::And: {
    const BinaryExpr *be = cast<BinaryExpr>(e);
    return evaluate(be->left).binaryAnd(evaluate(be->right));
  }
  case Expr::Or: {
    const BinaryExpr *be = cast<BinaryExpr>(e);
    return evaluate(be->left).binaryOr(evaluate(be->right));
  }
  case Expr::Xor: {
    const BinaryExpr *be = cast<BinaryExpr>(e);
    return evaluate(be->left).binaryXor(evaluate(be->right));
  }
  case Expr::Shl: {
    //    BinaryExpr *be = cast<BinaryExpr>(e);
    //    unsigned width = be->left->getWidth();
    //    return evaluate(be->left).shl(evaluate(be->right), width);
    break;
  }
  case Expr::LShr: {
    //    BinaryExpr *be = cast<BinaryExpr>(e);
    //    unsigned width = be->left->getWidth();
    //    return evaluate(be->left).lshr(evaluate(be->right), width);
    break;
  }
  case Expr::AShr: {
    //    BinaryExpr *be = cast<BinaryExpr>(e);
    //    unsigned width = be->left->getWidth();
    //    return evaluate(be->left).ashr(evaluate(be->right), width);
    break;
  }

    // Comparison

  case Expr::Eq: {
    const BinaryExpr *be = cast<BinaryExpr>(e);
    T left = evaluate(be->left);
    T right = evaluate(be->right);
      
    if (left.mustEqual(right)) {
      return T(1);
    } else if (!left.mayEqual(right)) {
      return T(0);
    }
    break;
  }

  case Expr::Ult: {
    const BinaryExpr *be = cast<BinaryExpr>(e);
    T left = evaluate(be->left);
    T right = evaluate(be->right);
      
    if (left.max() < right.min()) {
      return T(1);
    } else if (left.min() >= right.max()) {
      return T(0);
    }
    break;
  }
  case Expr::Ule: {
    const BinaryExpr *be = cast<BinaryExpr>(e);
    T left = evaluate(be->left);
    T right = evaluate(be->right);
      
    if (left.max() <= right.min()) {
      return T(1);
    } else if (left.min() > right.max()) {
      return T(0);
    }
    break;
  }
  case Expr::Slt: {
    const BinaryExpr *be = cast<BinaryExpr>(e);
    T left = evaluate(be->left);
    T right = evaluate(be->right);
    unsigned bits = be->left->getWidth();

    if (left.maxSigned(bits) < right.minSigned(bits)) {
      return T(1);
    } else if (left.minSigned(bits) >= right.maxSigned(bits)) {
      return T(0);
    }
    break;
  }
  case Expr::Sle: {
    const BinaryExpr *be = cast<BinaryExpr>(e);
    T left = evaluate(be->left);
    T right = evaluate(be->right);
    unsigned bits = be->left->getWidth();
      
    if (left.maxSigned(bits) <= right.minSigned(bits)) {
      return T(1);
    } else if (left.minSigned(bits) > right.maxSigned(bits)) {
      return T(0);
    }
    break;
  }

  case Expr::Ne:
  case Expr::Ugt:
  case Expr::Uge:
  case Expr::Sgt:
  case Expr::Sge:
    assert(0 && "invalid expressions (uncanonicalized)");

  default:
    break;
  }

  return T(0, bits64::maxValueOfNBits(e->getWidth()));
}

}

#endif
