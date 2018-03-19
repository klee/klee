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
  if (!N || isa<ReadExpr>(e) || isa<BvToIntExpr>(e) || isa<StrFromBitVector8Expr>(e))
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
ExprVisitor::Action ExprEvaluator::visitBvToInt(const BvToIntExpr& e) {
  return Action::changeTo(e.bvExpr);
}
ExprVisitor::Action ExprEvaluator::visitStrFromBv8(const StrFromBitVector8Expr& e) {
  return Action::changeTo(e.someBitVec8);
}
ExprVisitor::Action ExprEvaluator::visitFirstIndexOf(const StrFirstIdxOfExpr& sfi) {
    ref<Expr> _haystack = visit(sfi.haystack);
    ref<Expr> _needle = visit(sfi.needle);

    _haystack->dump();
    _needle->dump();
    StrConstExpr* haystack = dyn_cast<StrConstExpr>(_haystack);
    assert(haystack && "Haystack must be a constant string");
    std::string needle;
    if(ConstantExpr* n = dyn_cast<ConstantExpr>(_needle)) {
        needle = std::string(1,(char)n->getZExtValue(8));
    } else if(StrConstExpr* n = dyn_cast<StrConstExpr>(_needle)) {
        needle = n->value;
    } else  
      assert(false && "Needle must be constant bitvec");
    size_t firstIndex = haystack->value.find_first_of(needle);
    assert(firstIndex != std::string::npos && "Character must be present");

    llvm::errs() << "Needle: !!!!" << firstIndex << "\n";
    return Action::changeTo(ConstantExpr::create(firstIndex, Expr::Int64));
}

ExprVisitor::Action ExprEvaluator::visitStrEq(const StrEqExpr &eqE) {
    ref<Expr> _left = visit(eqE.s1);
    ref<Expr> _right = visit(eqE.s2);
    StrConstExpr* left = dyn_cast<StrConstExpr>(_left);
    StrConstExpr* right = dyn_cast<StrConstExpr>(_right);
    assert(left != nullptr && "Non constant strign expr in eq expr");
    assert(right != nullptr && "Non constant strign expr in eq expr");
    return Action::changeTo(ConstantExpr::create(left->value == right->value, Expr::Bool));
}
ExprVisitor::Action ExprEvaluator::visitStrSubstr(const StrSubstrExpr &subStrE) {
    ref<Expr> _offset = visit(subStrE.offset);
    ref<Expr> _length = visit(subStrE.length);
    ConstantExpr* offset = dyn_cast<ConstantExpr>(_offset);
    ConstantExpr* length = dyn_cast<ConstantExpr>(_length);
    if(offset != nullptr && length != nullptr) {
      llvm::errs() << "substr " << offset->getZExtValue() << " of len " << length->getZExtValue() << "\n";
      StrVarExpr* se = dyn_cast<StrVarExpr>(subStrE.s);
      if(se) {
        const Array *a = getStringArray(se->name);
        assert(a && "nullptr array from ab name");
        char c[length->getZExtValue()];
        for(int i = offset->getZExtValue(); i < length->getZExtValue(); i++) {
            c[i] = (char)dyn_cast<ConstantExpr>(getInitialValue(*a, i))->getZExtValue(8);
        }
        return Action::changeTo(StrConstExpr::create(c));
      } else {
        subStrE.s->dump();
        return Action::doChildren();
      }
    } else {
      _offset->dump();
      _length->dump();
      assert(false && "Non constant offsets for substr");
      return Action::doChildren();
    }

}

ExprVisitor::Action ExprEvaluator::visitCharAt(const StrCharAtExpr &charAtE) {
    ref<Expr> _index = visit(charAtE.i);
    ConstantExpr* index = dyn_cast<ConstantExpr>(_index);
    if(index != nullptr) {
      llvm::errs() << "charat " << index->getZExtValue() << "\n";
      StrVarExpr* se = dyn_cast<StrVarExpr>(charAtE.s);
      int idx = index->getZExtValue();
      char c[2] = {0,0};
      if(se) {
        const Array *a = getStringArray(se->name);
        assert(a && "nullptr array from ab name");
        c[0] = (char)dyn_cast<ConstantExpr>(getInitialValue(*a, idx))->getZExtValue(8);

      } else {
        ref<Expr> _string = visit(charAtE.s);
        StrConstExpr* str = dyn_cast<StrConstExpr>(_string);
        assert(str != nullptr && "Failed to make the string constant");
        c[0] = str->value.at(idx); 
      }
      return Action::changeTo(StrConstExpr::create(c));
    } else {
      _index->dump();
      assert(false && "Non constant offsets for substr");
      return Action::doChildren();
    }

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
