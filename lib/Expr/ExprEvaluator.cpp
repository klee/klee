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
  char arr[2] = { (char)c->getZExtValue(8), 0 };
  return Action::changeTo(StrConstExpr::create(arr));
}


static void printVectorString(std::vector<unsigned char> &ret) {
   for(auto &h : ret) llvm::errs() << (int)h << "-";
   //for(auto &h : ret) llvm::errs() << h << "-";
   llvm::errs() << "\n";
}
#define MAX_SIZE 1024
ExprVisitor::Action ExprEvaluator::visitStrVar(const StrVarExpr& se) {
//   llvm::errs() << "looking at array name " << se.name << "\n";
   const Array *a = getStringArray(se.name);
   if(a == nullptr) {
       //llvm::errs() << "Skipping " << se.name << "\n";
       return Action::skipChildren();
   }
//   llvm:: errs() << "array name " << a->name << "\n";
   assert(a && "nullptr array from ab name");
   std::vector<unsigned char> c(MAX_SIZE);

   char numBuf[3] = {0};
   numBuf[2] = 0;
   int idx = 0;
   int numBufIdx = -1; 
   int getChrIdx = 0;
   while(idx < 25) { //not null terminated
        //llvm::errs() << a <<  a->name <<  " In loop\n";
        ref<Expr> ch = getInitialValue(*a, getChrIdx);
        if(isa<NotOptimizedExpr>(ch)) break;
        c[idx] = (char)dyn_cast<ConstantExpr>(ch)->getZExtValue(8);
//        printf("%c: idx: %d, numBufIdx: %d, numBuf: %s\n", c[idx], idx, numBufIdx, numBuf);
//        llvm::errs() << "str at: ";
//        for(auto &h : c) llvm::errs() << (int)h << "-";
//        llvm::errs() << "\n";
//        llvm::errs() << " as string "<< std::string(c.begin(), c.end()) << "\n";
        if(numBufIdx >= 0) {
            numBuf[numBufIdx] = c[idx];
            numBufIdx++;
        }

        if(numBufIdx == 2) {
            numBufIdx = -1;
            c[idx] = (unsigned char)strtol(numBuf,NULL, 16);
        }

        if(idx > 0 && c[idx-1] == '\\') {
            idx--;
            switch(c[idx+1]) {
              case 'n': c[idx] = '\n'; break;
              case 'r': c[idx] = '\r'; break;
              case 'x': numBufIdx = 0; break;
              case '\\': c[idx] = '\\'; break;
            }
        }

        //printf("%c: idx: %d, numBufIdx: %d, numBuf: %s\n", c[idx], idx, numBufIdx, numBuf);
        //llvm::errs() << "str at: ";// << std::string(c.begin(), c.end()) << "\n";
        //for(auto &h : c) llvm::errs() << h << "-";
        //llvm::errs() << "\n";
        if(numBufIdx == -1) {
            idx++;
        }
        //llvm::errs() << "\n";
        getChrIdx++;
   }
   std::vector<unsigned char> ret(c.begin(), c.begin() + idx );
//   llvm::errs() << "Evaluated str var " << se.name << " to " ;//<< std::string(c.begin(), c.end()) << "\n";
//   printVectorString(ret);
   return Action::changeTo(StrConstExpr::alloc(ret));
}


ExprVisitor::Action ExprEvaluator::visitStrLen(const StrLengthExpr& sle) {
 //   sle.dump();
    ref<Expr> _s = visit(sle.s);
 //   _s->dump();
    StrConstExpr* s = dyn_cast<StrConstExpr>(_s);
   if(s == nullptr) return Action::skipChildren();
    assert(s && "_s must be a constant string");
//    std::string normalizedS = NormalizeZ3String(s->value);
    size_t len = s->data.size();
  //  llvm::errs() << "Strlen returning " <<  len << "\n";
    return Action::changeTo(ConstantExpr::create(len, Expr::Int64));
}

bool ends_with(std::string const & value, std::string const & ending)
{
    if (ending.size() > value.size()) return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

ExprVisitor::Action ExprEvaluator::visitContainsStringsExpr(const StrContainsExpr& sc)
{
    ref<Expr> _haystack = visit(sc.haystack);
    ref<Expr> _needle = visit(sc.needle);

    StrConstExpr* haystack = dyn_cast<StrConstExpr>(_haystack);
    if(haystack == nullptr) return Action::skipChildren();
    assert(haystack && "haystack must be a constant string");

    StrConstExpr* needle = dyn_cast<StrConstExpr>(_needle);
    if(needle == nullptr) return Action::skipChildren();
    assert(needle && "needle must be a constant string");

    std::string oren_haystack;
    std::string oren_needle;
 
    int i=0;
    
    for (i=0;i < haystack->data.size(); i++) { oren_haystack[i]=haystack->data[i]; }
    for (i=0;i < needle->data.size();   i++) { oren_needle[i]=needle->data[i]; }
    
    for (i=0;i < haystack->data.size(); i++) { llvm::errs() << haystack[i]; }
    llvm::errs() << " contains ";
    for (i=0;i < needle->data.size(); i++) { llvm::errs() << needle[i]; }
    llvm::errs() << " == ";

	bool res;
	
	if (oren_haystack.find(oren_needle) != std::string::npos)
	{
		res = true;
	}
	else
	{
		res = false;
	}

    if (res) { llvm::errs() << "True \n"; }
    else     { llvm::errs() << "False\n"; }
    
    return Action::changeTo(ConstantExpr::create(res,Expr::Bool));	
}

ExprVisitor::Action ExprEvaluator::visitSuffixStringsExpr(const StrSuffixExpr& se)
{
    ref<Expr> _s      = visit(se.s);
    ref<Expr> _suffix = visit(se.suffix);

    StrConstExpr* s = dyn_cast<StrConstExpr>(_s);
    if(s == nullptr) return Action::skipChildren();
    assert(s && "s must be a constant string");

    StrConstExpr* suffix = dyn_cast<StrConstExpr>(_suffix);
    if(suffix == nullptr) return Action::skipChildren();
    assert(suffix && "suffix must be a constant string");

    std::string oren_s;
    std::string oren_suffix;
 
    int i=0;
    
    for (i=0;i < s->data.size();      i++) { oren_s[i]=s->data[i]; }
    for (i=0;i < suffix->data.size(); i++) { oren_suffix[i]=suffix->data[i]; }
    
    for (i=0;i < s->data.size(); i++) { llvm::errs() << s[i]; }
    llvm::errs() << " contains ";
    for (i=0;i < suffix->data.size(); i++) { llvm::errs() << suffix[i]; }
    llvm::errs() << " == ";

	bool res = ends_with(oren_s,oren_suffix);

    if (res) { llvm::errs() << "True \n"; }
    else     { llvm::errs() << "False\n"; }
    
    return Action::changeTo(ConstantExpr::create(res,Expr::Bool));
}

ExprVisitor::Action ExprEvaluator::visitConcatStringsExpr(const StrConcatExpr& sc) {
//    sfi.dump();
    ref<Expr> _s1 = visit(sc.s1);
    ref<Expr> _s2 = visit(sc.s2);

    StrConstExpr* s1 = dyn_cast<StrConstExpr>(_s1);
    if(s1 == nullptr) return Action::skipChildren();
    assert(s1 && "s1 must be a constant string");

    StrConstExpr* s2 = dyn_cast<StrConstExpr>(_s2);
    if(s2 == nullptr) return Action::skipChildren();
    assert(s2 && "s2 must be a constant string");

    //auto firstIndex = std::search(haystack->data.begin(), haystack->data.end(),
    //            needle.begin(), needle.end());
    //    llvm::errs() << "Haystack: ";
    //    printVectorString(haystack->data);
    //    llvm::errs() << "Needle: ";
    //    printVectorString(needle);
    // size_t fstIdx = std::distance(haystack->data.begin(), firstIndex);
    // if(firstIndex == haystack->data.end()) {
    //        llvm::errs() << "Character not found\n";
    //    return Action::changeTo(ConstantExpr::create(-1, Expr::Int64));
    // }
    //assert(firstIndex != haystack->data.end()  && "Character must be present");
    // llvm::errs() << "Needle found at offset = " << fstIdx << "\n";
    std::string oren_s1;
    std::string oren_s2;
 
    int i=0;
    
    for (i=0;i < s1->data.size(); i++) { oren_s1[i]=s1->data[i]; }
    for (i=0;i < s2->data.size(); i++) { oren_s2[i]=s2->data[i]; }
    
    for (i=0;i < s1->data.size(); i++) { llvm::errs() << s1[i]; }
    llvm::errs() << " ++ ";
    for (i=0;i < s2->data.size(); i++) { llvm::errs() << s2[i]; }
    llvm::errs() << " == ";
    llvm::errs() << oren_s1+oren_s2 << "\n";
    
    return Action::changeTo(StrConstExpr::create(oren_s1+oren_s2));
}

ExprVisitor::Action ExprEvaluator::visitFirstIndexOf(const StrFirstIdxOfExpr& sfi) {
//    sfi.dump();
    ref<Expr> _haystack = visit(sfi.haystack);
    ref<Expr> _needle = visit(sfi.needle);

    StrConstExpr* haystack = dyn_cast<StrConstExpr>(_haystack);
    if(haystack == nullptr) return Action::skipChildren();
    assert(haystack && "Haystack must be a constant string");
    std::vector<unsigned char> needle;
    if(ConstantExpr* n = dyn_cast<ConstantExpr>(_needle)) {
        std::vector<unsigned char> tm(1);
        tm[0] = n->getZExtValue(8);
        needle = tm; //std::string(1,(char)n->getZExtValue(8));
    } else if(StrConstExpr* n = dyn_cast<StrConstExpr>(_needle)) {
        needle = n->data;
    } else {
      return Action::skipChildren();
      assert(false && "Needle must be constant bitvec");
    }

    auto firstIndex = std::search(haystack->data.begin(), haystack->data.end(),
                needle.begin(), needle.end());
//    llvm::errs() << "Haystack: ";
//    printVectorString(haystack->data);
//    llvm::errs() << "Needle: ";
//    printVectorString(needle);
    size_t fstIdx = std::distance(haystack->data.begin(), firstIndex);
    if(firstIndex == haystack->data.end()) {
//        llvm::errs() << "Character not found\n";
        return Action::changeTo(ConstantExpr::create(-1, Expr::Int64));
    }
    assert(firstIndex != haystack->data.end()  && "Character must be present");
 //   llvm::errs() << "Needle found at offset = " << fstIdx << "\n";
    return Action::changeTo(ConstantExpr::create(fstIdx, Expr::Int64));
}

ExprVisitor::Action ExprEvaluator::visitStrEq(const StrEqExpr &eqE) {
    ref<Expr> _left = visit(eqE.s1);
    ref<Expr> _right = visit(eqE.s2);
//    _left->dump();
//    _right->dump();
    StrConstExpr* left = dyn_cast<StrConstExpr>(_left);
    StrConstExpr* right = dyn_cast<StrConstExpr>(_right);
    if(left == nullptr) return Action::skipChildren();
    if(right == nullptr) return Action::skipChildren();
    assert(left != nullptr && "Non constant strign expr in eq expr");
    assert(right != nullptr && "Non constant strign expr in eq expr");
    return Action::changeTo(ConstantExpr::create(left->data == right->data, Expr::Bool));
}
ExprVisitor::Action ExprEvaluator::visitStrSubstr(const StrSubstrExpr &subStrE) {
//    subStrE.dump();
    ref<Expr> _offset = visit(subStrE.offset);
    ref<Expr> _length = visit(subStrE.length);
    ConstantExpr* offset = dyn_cast<ConstantExpr>(_offset);
    ConstantExpr* length = dyn_cast<ConstantExpr>(_length);
    if(offset != nullptr && length != nullptr) {
  //  llvm::errs() << "substr starting from: " << offset->getZExtValue() << " of len " << length->getZExtValue() << "\n";
      ref<Expr> _theString = visit(subStrE.s);
      StrConstExpr* theString = dyn_cast<StrConstExpr>(_theString);
      if(theString == nullptr) {
   //       llvm::errs() << "skiping substring\n";
          return Action::skipChildren();
      }
      assert(theString != nullptr && "Non constant strign expr in SubString expr");
      if(offset->Add(length)->getZExtValue() > theString->data.size()) return Action::skipChildren();
    //  llvm::errs() << "string size: " << theString->data.size() << "\n";
      std::vector<unsigned char> subStr(theString->data.begin() + offset->getZExtValue(),
                                        theString->data.begin() + offset->getZExtValue() + length->getZExtValue());
      return Action::changeTo(StrConstExpr::alloc(subStr));
    } else {
      return Action::skipChildren();
      assert(false && "Non constant offsets for substr");
    }

}

ExprVisitor::Action ExprEvaluator::visitCharAt(const StrCharAtExpr &charAtE) {
    ref<Expr> _index = visit(charAtE.i);
    ConstantExpr* index = dyn_cast<ConstantExpr>(_index);
    if(index != nullptr) {
//      llvm::errs() << "charat " << index->getZExtValue() << "\n";
      int idx = index->getZExtValue();
      char c[2] = {0,0};
      ref<Expr> _string = visit(charAtE.s);
      StrConstExpr* str = dyn_cast<StrConstExpr>(_string);
      if(str == nullptr) return Action::skipChildren();
      assert(str != nullptr && "Failed to make the string constant");
      if(idx >= str->data.size()) return Action::skipChildren();
      c[0] = str->data[idx]; 
//    fprintf(stderr,"c is '%s'\n", c);
      ref<Expr> ret = StrConstExpr::create(c);
      return Action::changeTo(ret);
    } else {
      return Action::skipChildren();
      assert(false && "Non constant offsets for substr");
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
