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
  ref<Expr> _c = visit(e.someBitVec8);
  ConstantExpr *c = dyn_cast<ConstantExpr>(_c);
  assert(c && "non constant bv in strfrobv8");
  char arr[1] = { (char)c->getZExtValue(8) };
  return Action::changeTo(StrConstExpr::create(arr));
}

std::string NormalizeZ3String(const std::string &s)
{
	unsigned i=0;
	int j=0;
	int i1=0;
	int i2=0;
	char c1=0;
	char c2=0;
	char normalized[1024]={0};
		
	for (i=0;i<s.size();i++)
	{
		if (s[i] != '\\')
		{
			normalized[j++]=s[i];
		}
		else
		{
			assert(s[i+1] == 'x');

			c1 = s[i+2];
			c2 = s[i+3];

			int c1_is_a_digit=0;
			int c2_is_a_digit=0;

			int c1_is_a_to_f=0;
			int c2_is_a_to_f=0;

			int c1_is_hex_valid=0;
			int c2_is_hex_valid=0;
			
			if (('0' <= c1) && (c1 <= '9')) { c1_is_a_digit=1; i1 = c1-'0'; }
			if (('0' <= c2) && (c2 <= '9')) { c2_is_a_digit=1; i2 = c2-'0'; }

			if (('a' <= c1) && (c1 <= 'f')) { c1_is_a_to_f =1; i1 = c1-'a'; }
			if (('a' <= c2) && (c2 <= 'f')) { c2_is_a_to_f =1; i2 = c2-'a';}

			if (c1_is_a_digit || c1_is_a_to_f) { c1_is_hex_valid=1; }
			if (c2_is_a_digit || c2_is_a_to_f) { c2_is_hex_valid=1; }
			
			assert(c1_is_hex_valid && c2_is_hex_valid);
			
			normalized[j++] = 16*i1+i2;

			i += 3;
		}
	}
	
	return normalized;
}

ExprVisitor::Action ExprEvaluator::visitFirstIndexOf(const StrFirstIdxOfExpr& sfi) {
    sfi.dump();
    ref<Expr> _haystack = visit(sfi.haystack);
    ref<Expr> _needle = visit(sfi.needle);

    StrConstExpr* haystack = dyn_cast<StrConstExpr>(_haystack);
    assert(haystack && "Haystack must be a constant string");
    std::string needle;
    if(ConstantExpr* n = dyn_cast<ConstantExpr>(_needle)) {
        needle = std::string(1,(char)n->getZExtValue(8));
    } else if(StrConstExpr* n = dyn_cast<StrConstExpr>(_needle)) {
        needle = n->value;
    } else {
      assert(false && "Needle must be constant bitvec");
    }

	size_t firstIndex=0;
	std::string normalizedNeedle  =NormalizeZ3String(needle);
	std::string normalizedHaystack=NormalizeZ3String(haystack->value);

	if (normalizedNeedle.size() == 0)
	{
		firstIndex = normalizedHaystack.size();
	}
	else
	{
		firstIndex = normalizedHaystack.find(normalizedNeedle);
	}
    // size_t firstIndex = (NormalizeZ3String(haystack->value)).find(NormalizeZ3String(needle));
    // size_t firstIndex = haystack->value.find(needle);
    // size_t firstIndex = haystack->value.find_first_of(needle);
    llvm::errs() << "haystack->value     is:" << haystack->value    << "END\n";
    llvm::errs() << "normalized haystack is:" << normalizedHaystack << "END\n";
    llvm::errs() << "needle              is:" << needle             << "END\n";
    llvm::errs() << "normalized needle   is:" << normalizedNeedle   << "END\n";
    llvm::errs() << "firstIndex          is:" << firstIndex         << "\n";
    assert(firstIndex != std::string::npos && "Character must be present");

    llvm::errs() << "Needle found at offset = " << firstIndex << "\n";
    return Action::changeTo(ConstantExpr::create(firstIndex, Expr::Int64));
}

ExprVisitor::Action ExprEvaluator::visitStrEq(const StrEqExpr &eqE) {
    eqE.dump();
    ref<Expr> _left = visit(eqE.s1);
    ref<Expr> _right = visit(eqE.s2);
    StrConstExpr* left = dyn_cast<StrConstExpr>(_left);
    StrConstExpr* right = dyn_cast<StrConstExpr>(_right);
    _right->dump();
    _left->dump();
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
      llvm::errs() << "substr starting from: " << offset->getZExtValue() << " of len " << length->getZExtValue() << "\n";
      StrVarExpr* se = dyn_cast<StrVarExpr>(subStrE.s);
      if(se) {
        int fixedLength=0;
        const Array *a = getStringArray(se->name);
        assert(a && "nullptr array from ab name");
        char c[1024]={0};
        fixedLength=length->getZExtValue();
        // char c[length->getZExtValue()];
        for (int i = offset->getZExtValue(); i < fixedLength; i++)
        {
            c[i] = (char)dyn_cast<ConstantExpr>(getInitialValue(*a, i))->getZExtValue(8);
//            printf("i: %d, c[i] %c\n", i, c[i]);

            if (c[i] == '\\')
            {
            	fixedLength += 3;
            }
        }
        return Action::changeTo(StrConstExpr::create(c));
      } else {
        return Action::doChildren();
      }
    } else {
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
      llvm::errs() << "char at return " << c[0] << "\n";
      fprintf(stderr,"c is '%s'\n", c);
      ref<Expr> ret = StrConstExpr::create(c);
      ret->dump();
      return Action::changeTo(ret);
    } else {
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
