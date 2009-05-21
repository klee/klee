/********************************************************************
 * AUTHORS: Vijay Ganesh, David L. Dill
 *
 * BEGIN DATE: November, 2005
 *
 * LICENSE: Please view LICENSE file in the home dir of this Program
 ********************************************************************/
// -*- c++ -*-

#include "../AST/AST.h"
#include "../AST/ASTUtil.h"
namespace BEEV {

  //error printing
  static void BVConstEvaluatorError(CONSTANTBV::ErrCode e, const ASTNode& t){
    std::string ss("BVConstEvaluator:");
    ss += (const char*)BitVector_Error(e);	
    FatalError(ss.c_str(), t);
  }

#ifndef NATIVE_C_ARITH
  ASTNode BeevMgr::BVConstEvaluator(const ASTNode& t) {
    ASTNode OutputNode;
    Kind k = t.GetKind();

    if(CheckSolverMap(t,OutputNode))
      return OutputNode;
    OutputNode = t;

    unsigned int inputwidth = t.GetValueWidth();
    unsigned int outputwidth = inputwidth;
    CBV output = NULL;

    CBV tmp0 = NULL;
    CBV tmp1 = NULL;

    //saving some typing. BVPLUS does not use these variables. if the
    //input BVPLUS has two nodes, then we want to avoid setting these
    //variables.
    if(1 == t.Degree() ){
      tmp0 = BVConstEvaluator(t[0]).GetBVConst();
    }else if(2 == t.Degree() && k != BVPLUS ) {
      tmp0 = BVConstEvaluator(t[0]).GetBVConst();
      tmp1 = BVConstEvaluator(t[1]).GetBVConst();
    }

    switch(k) {
    case UNDEFINED:
    case READ:
    case WRITE:
    case SYMBOL:
      FatalError("BVConstEvaluator: term is not a constant-term",t);
      break;
    case BVCONST:
      //FIXME Handle this special case better
      OutputNode = t;
      break;
    case BVNEG:{
      output = CONSTANTBV::BitVector_Create(inputwidth,true);
      CONSTANTBV::Set_Complement(output,tmp0);
      OutputNode = CreateBVConst(output,outputwidth);
      break;
    }
    case BVSX: {
      output = CONSTANTBV::BitVector_Create(inputwidth,true);
      //unsigned * out0 = BVConstEvaluator(t[0]).GetBVConst();
      unsigned t0_width = t[0].GetValueWidth();
      if(inputwidth == t0_width) {
        CONSTANTBV::BitVector_Copy(output, tmp0);
        OutputNode = CreateBVConst(output, outputwidth);    
      }
      else {
        bool topbit_sign = (CONSTANTBV::BitVector_Sign(tmp0) < 0 );

        if(topbit_sign){
          CONSTANTBV::BitVector_Fill(output);
        }
        CONSTANTBV::BitVector_Interval_Copy(output, tmp0, 0, 0, t0_width);
        OutputNode = CreateBVConst(output, outputwidth);    
      }
      break;
    }
    case BVAND: {
      output = CONSTANTBV::BitVector_Create(inputwidth,true);
      CONSTANTBV::Set_Intersection(output,tmp0,tmp1);
      OutputNode = CreateBVConst(output, outputwidth);
      break;
    }
    case BVOR: {
      output = CONSTANTBV::BitVector_Create(inputwidth,true);
      CONSTANTBV::Set_Union(output,tmp0,tmp1);
      OutputNode = CreateBVConst(output, outputwidth);
      break;
    }
    case BVXOR: {
      output = CONSTANTBV::BitVector_Create(inputwidth,true);
      CONSTANTBV::Set_ExclusiveOr(output,tmp0,tmp1);
      OutputNode = CreateBVConst(output, outputwidth);
      break;
    }
    case BVSUB: {
      output = CONSTANTBV::BitVector_Create(inputwidth,true);
      bool carry = false;
      CONSTANTBV::BitVector_sub(output,tmp0,tmp1,&carry);
      OutputNode = CreateBVConst(output, outputwidth);    
      break;
    }
    case BVUMINUS: {
      output = CONSTANTBV::BitVector_Create(inputwidth,true);
      CONSTANTBV::BitVector_Negate(output, tmp0);
      OutputNode = CreateBVConst(output, outputwidth);    
      break;
    }
    case BVEXTRACT: {
      output = CONSTANTBV::BitVector_Create(inputwidth,true);
      tmp0 = BVConstEvaluator(t[0]).GetBVConst();
      unsigned int hi = GetUnsignedConst(BVConstEvaluator(t[1]));
      unsigned int low = GetUnsignedConst(BVConstEvaluator(t[2]));
      unsigned int len = hi-low+1;

      CONSTANTBV::BitVector_Destroy(output);
      output = CONSTANTBV::BitVector_Create(len, false);
      CONSTANTBV::BitVector_Interval_Copy(output, tmp0, 0, low, len);
      outputwidth = len;
      OutputNode = CreateBVConst(output, outputwidth);
      break;
    }
    //FIXME Only 2 inputs?
    case BVCONCAT: {
       output = CONSTANTBV::BitVector_Create(inputwidth,true);
      unsigned t0_width = t[0].GetValueWidth();
      unsigned t1_width = t[1].GetValueWidth();
      CONSTANTBV::BitVector_Destroy(output);
      
      output = CONSTANTBV::BitVector_Concat(tmp0, tmp1);
      outputwidth = t0_width + t1_width;
      OutputNode = CreateBVConst(output, outputwidth);
      
      break;
    }
    case BVMULT: {
       output = CONSTANTBV::BitVector_Create(inputwidth,true);
      CBV tmp = CONSTANTBV::BitVector_Create(2*inputwidth,true);
      CONSTANTBV::ErrCode e = CONSTANTBV::BitVector_Multiply(tmp,tmp0,tmp1);
      
      if(0 != e) {
        BVConstEvaluatorError(e,t);
      }
      //FIXME WHAT IS MY OUTPUT???? THE SECOND HALF of tmp?
      //CONSTANTBV::BitVector_Interval_Copy(output, tmp, 0, inputwidth, inputwidth);
      CONSTANTBV::BitVector_Interval_Copy(output, tmp, 0, 0, inputwidth);
      OutputNode = CreateBVConst(output, outputwidth);
      CONSTANTBV::BitVector_Destroy(tmp);
      break;
    }
    case BVPLUS: {
       output = CONSTANTBV::BitVector_Create(inputwidth,true);
      bool carry = false;
      ASTVec c = t.GetChildren();
      for(ASTVec::iterator it=c.begin(),itend=c.end();it!=itend;it++) {
	CBV kk = BVConstEvaluator(*it).GetBVConst();
	CONSTANTBV::BitVector_add(output,output,kk,&carry);
	carry = false;
	//CONSTANTBV::BitVector_Destroy(kk);
      }
      OutputNode = CreateBVConst(output, outputwidth);
      break;
    }
    //FIXME ANOTHER SPECIAL CASE
    case SBVDIV:
    case SBVMOD:{
      OutputNode = BVConstEvaluator(TranslateSignedDivMod(t));
      break;
    }
    case BVDIV: 
    case BVMOD: {
      CBV quotient = CONSTANTBV::BitVector_Create(inputwidth,true);
      CBV remainder = CONSTANTBV::BitVector_Create(inputwidth,true);
      
      // tmp0 is dividend, tmp1 is the divisor
      //All parameters to BitVector_Div_Pos must be distinct unlike BitVector_Divide
      //FIXME the contents of the second parameter to Div_Pos is destroyed
      //As tmp0 is currently the same as the copy belonging to an ASTNode t[0]
      //this must be copied.
      tmp0 = CONSTANTBV::BitVector_Clone(tmp0);      
      CONSTANTBV::ErrCode e= CONSTANTBV::BitVector_Div_Pos(quotient,tmp0,tmp1,remainder);
      CONSTANTBV::BitVector_Destroy(tmp0);
      
      if(0 != e) {
	//error printing
	if(counterexample_checking_during_refinement) {
          output = CONSTANTBV::BitVector_Create(inputwidth,true);
	  OutputNode = CreateBVConst(output, outputwidth);
	  bvdiv_exception_occured = true;
          
          //  CONSTANTBV::BitVector_Destroy(output);
	  break;
	}
	else {
	  BVConstEvaluatorError(e,t);
	}
      } //end of error printing

      //FIXME Not very standard in the current scheme
      if(BVDIV == k){
        OutputNode = CreateBVConst(quotient, outputwidth);
        CONSTANTBV::BitVector_Destroy(remainder);
      }else{
        OutputNode = CreateBVConst(remainder, outputwidth);
        CONSTANTBV::BitVector_Destroy(quotient);
      }

      break;
    }
    case ITE:
      if(ASTTrue == t[0])
	OutputNode = BVConstEvaluator(t[1]);
      else if(ASTFalse == t[0])
	OutputNode = BVConstEvaluator(t[2]);
      else
	FatalError("BVConstEvaluator: ITE condiional must be either TRUE or FALSE:",t);
      break;
    case EQ: 
      if(CONSTANTBV::BitVector_equal(tmp0,tmp1))
	OutputNode = ASTTrue;
      else
	OutputNode = ASTFalse;
      break;
    case NEQ:
      if(!CONSTANTBV::BitVector_equal(tmp0,tmp1))
	OutputNode = ASTTrue;
      else
	OutputNode = ASTFalse;
      break;
    case BVLT:
      if(-1 == CONSTANTBV::BitVector_Lexicompare(tmp0,tmp1))
	OutputNode = ASTTrue;
      else
	OutputNode = ASTFalse;
      break;
    case BVLE: {
      int comp = CONSTANTBV::BitVector_Lexicompare(tmp0,tmp1);
      if(comp <= 0)
	OutputNode = ASTTrue;
      else
	OutputNode = ASTFalse;
      break;
    }
    case BVGT:
      if(1 == CONSTANTBV::BitVector_Lexicompare(tmp0,tmp1))
	OutputNode = ASTTrue;
      else
	OutputNode = ASTFalse;
      break;
    case BVGE: {
      int comp = CONSTANTBV::BitVector_Lexicompare(tmp0,tmp1);
      if(comp >= 0)
	OutputNode = ASTTrue;
      else
	OutputNode = ASTFalse;
      break;
    } 
    case BVSLT:
      if(-1 == CONSTANTBV::BitVector_Compare(tmp0,tmp1))
	OutputNode = ASTTrue;
      else
	OutputNode = ASTFalse;
      break;
    case BVSLE: {
      signed int comp = CONSTANTBV::BitVector_Compare(tmp0,tmp1);
      if(comp <= 0)
	OutputNode = ASTTrue;
      else
	OutputNode = ASTFalse;
      break;
    }
    case BVSGT:
      if(1 == CONSTANTBV::BitVector_Compare(tmp0,tmp1))
	OutputNode = ASTTrue;
      else
	OutputNode = ASTFalse;
      break;
    case BVSGE: {
      int comp = CONSTANTBV::BitVector_Compare(tmp0,tmp1);
      if(comp >= 0)
	OutputNode = ASTTrue;
      else
	OutputNode = ASTFalse;
      break;
    } 
    default:
      FatalError("BVConstEvaluator: The input kind is not supported yet:",t);
      break;
    }
/*
    if(BVCONST != k){
     cerr<<inputwidth<<endl;
     cerr<<"------------------------"<<endl;
     t.LispPrint(cerr);
     cerr<<endl;
     OutputNode.LispPrint(cerr);
     cerr<<endl<<"------------------------"<<endl;
    }
*/
    UpdateSolverMap(t,OutputNode);
    //UpdateSimplifyMap(t,OutputNode,false);
    return OutputNode;
  }
#else
  //accepts 64 bit BVConst and sign extends it
  static unsigned long long int SXBVConst64(const ASTNode& t) {
    unsigned long long int c = t.GetBVConst();
    unsigned int len = t.GetValueWidth();

    unsigned long long int mask = 1;
    mask = mask << len-1;

    bool TopBit = (c & mask) ? true : false;
    if(!TopBit) return c;
    
    unsigned long long int sign = 0xffffffffffffffffLL;
    sign = sign << len-1;

    return (c | sign);
  }

  //FIXME: Ideally I would like the ASTNodes to be able to operate on
  //themselves (add, sub, concat, etc.) rather than doing a
  //GetBVConst() and then do the operation externally. For now,
  //this is the fastest path to completion.
  ASTNode BeevMgr::BVConstEvaluator(const ASTNode& t) {
    //cerr << "inside begin bcconstevaluator: " << t << endl;

    ASTNode OutputNode;
    if(CheckSolverMap(t,OutputNode))
      return OutputNode;
    OutputNode = ASTUndefined;

    Kind k = t.GetKind();
    unsigned long long int output = 0;
    unsigned inputwidth = t.GetValueWidth();
    ASTNode t0 = ASTUndefined;
    ASTNode t1 = ASTUndefined;
    if(2 == t.Degree()) {
      t0 = BVConstEvaluator(t[0]);
      t1 = BVConstEvaluator(t[1]);
    }
    switch(k) {
    case READ:
    case UNDEFINED:
    case WRITE:
    case SYMBOL:
      cerr << t;
      FatalError("BVConstEvaluator: term is not a constant-term",t);
      break;
    case BVCONST:
      return t;
      break;
    case BVNEG:
      //compute bitwise negation in C
      output = ~(BVConstEvaluator(t[0]).GetBVConst());
      break;
    case BVSX:
      output = SXBVConst64(BVConstEvaluator(t[0]));    
      break;
    case BVAND:
      output = t0.GetBVConst() & t1.GetBVConst();
      break;
    case BVOR:
      output = t0.GetBVConst() | t1.GetBVConst();
      break;
    case BVXOR:
      output = t0.GetBVConst() ^ t1.GetBVConst();
      break;
    case BVSUB:
      output = t0.GetBVConst() - t1.GetBVConst();
      break;
    case BVUMINUS:
      output = ~(BVConstEvaluator(t[0]).GetBVConst()) + 1;
      break;
    case BVEXTRACT: {
      unsigned long long int val = BVConstEvaluator(t[0]).GetBVConst();
      unsigned int hi = GetUnsignedConst(BVConstEvaluator(t[1]));
      unsigned int low = GetUnsignedConst(BVConstEvaluator(t[2]));

      if(!(0 <= hi <= 64))
	FatalError("ConstantEvaluator: hi bit in BVEXTRACT is > 32bits",t);
      if(!(0 <= low <= hi <= 64))
	FatalError("ConstantEvaluator: low bit in BVEXTRACT is > 32bits or hi",t);

      //64 bit mask.
      unsigned long long int mask1 = 0xffffffffffffffffLL;
      mask1 >>= 64-(hi+1);
      
      //extract val[hi:0]
      val &= mask1;
      //extract val[hi:low]
      val >>= low;
      output = val;
      break;
    }
    case BVCONCAT: {
      unsigned long long int q = BVConstEvaluator(t0).GetBVConst();
      unsigned long long int r = BVConstEvaluator(t1).GetBVConst();

      unsigned int qlen = t[0].GetValueWidth();
      unsigned int rlen = t[1].GetValueWidth();
      unsigned int slen = t.GetValueWidth();
      if(!(0 <  qlen + rlen  <= 64))
	FatalError("BVConstEvaluator:"
		   "lengths of childnodes of BVCONCAT are > 64:",t);

      //64 bit mask for q
      unsigned long long int qmask = 0xffffffffffffffffLL;     
      qmask >>= 64-qlen;
      //zero the useless bits of q
      q &= qmask;

      //64 bit mask for r
      unsigned long long int rmask = 0xffffffffffffffffLL;     
      rmask >>= 64-rlen;
      //zero the useless bits of r
      r &= rmask;
      
      //concatenate
      q <<= rlen;
      q |= r;

      //64 bit mask for output s
      unsigned long long int smask = 0xffffffffffffffffLL;
      smask >>= 64-slen;
      
      //currently q has the output
      output = q;      
      output &= smask;
      break;
    }
    case BVMULT: {
      output = t0.GetBVConst() * t1.GetBVConst();

      //64 bit mask
      unsigned long long int mask = 0xffffffffffffffffLL;
      mask = mask >> (64 - inputwidth);
      output &= mask;
      break;
    }
    case BVPLUS: {
      ASTVec c = t.GetChildren();
      for(ASTVec::iterator it=c.begin(),itend=c.end();it!=itend;it++)
	output += BVConstEvaluator(*it).GetBVConst();

      //64 bit mask
      unsigned long long int mask = 0xffffffffffffffffLL;
      mask = mask >> (64 -inputwidth);
      output &= mask;
      break;
    }
    case SBVDIV:
    case SBVMOD: {
      output = BVConstEvaluator(TranslateSignedDivMod(t)).GetBVConst();
      break;
    }
    case BVDIV: {
      if(0 == t1.GetBVConst()) {
	//if denominator is 0 then 
	//  (if refinement is ON then output is set to 0) 
	//   (else produce a fatal error)
	if(counterexample_checking_during_refinement) {
	  output = 0;
	  bvdiv_exception_occured = true;
	  break;
	}
	else {
	  FatalError("BVConstEvaluator: divide by zero not allowed:",t);
	}
      }

      output = t0.GetBVConst() / t1.GetBVConst();
      //64 bit mask
      unsigned long long int mask = 0xffffffffffffffffLL;
      mask = mask >> (64 - inputwidth);
      output &= mask;
      break;
    }
    case BVMOD: {
      if(0 == t1.GetBVConst()) {
	//if denominator is 0 then 
	//  (if refinement is ON then output is set to 0) 
	//   (else produce a fatal error)
	if(counterexample_checking_during_refinement) {
	  output = 0;
	  bvdiv_exception_occured = true;
	  break;
	}
	else {
	  FatalError("BVConstEvaluator: divide by zero not allowed:",t);
	}
      }

      output = t0.GetBVConst() % t1.GetBVConst();
      //64 bit mask
      unsigned long long int mask = 0xffffffffffffffffLL;
      mask = mask >> (64 - inputwidth);
      output &= mask;
      break;
    }
    case ITE:
      if(ASTTrue == t[0])
	OutputNode = BVConstEvaluator(t[1]);
      else if(ASTFalse == t[0])
	OutputNode = BVConstEvaluator(t[2]);
      else
	FatalError("BVConstEvaluator:" 
		   "ITE condiional must be either TRUE or FALSE:",t);
      break;
    case EQ:
      if(t0.GetBVConst() == t1.GetBVConst())
	OutputNode = ASTTrue;
      else
	OutputNode = ASTFalse;
      break;
    case NEQ:
      if(t0.GetBVConst() != t1.GetBVConst())
	OutputNode = ASTTrue;
      else
	OutputNode = ASTFalse;
      break;
      break;
    case BVLT: {
      unsigned long long n0 = t0.GetBVConst();
      unsigned long long n1 = t1.GetBVConst();
      if(n0 < n1)
	OutputNode = ASTTrue;
      else
	OutputNode = ASTFalse;
      break;
    }
    case BVLE:
      if(t0.GetBVConst() <= t1.GetBVConst())
	OutputNode = ASTTrue;
      else
	OutputNode = ASTFalse;
      break;
    case BVGT:
      if(t0.GetBVConst() > t1.GetBVConst())
	OutputNode = ASTTrue;
      else
	OutputNode = ASTFalse;
      break;
    case BVGE:
      if(t0.GetBVConst() >= t1.GetBVConst())
	OutputNode = ASTTrue;
      else
	OutputNode = ASTFalse;
      break;
    case BVSLT: {
      signed long long int n0 = SXBVConst64(t0);
      signed long long int n1 = SXBVConst64(t1);
      if(n0 < n1)
	OutputNode = ASTTrue;
      else
	OutputNode = ASTFalse;
      break;
    }
    case BVSLE: {
      signed long long int n0 = SXBVConst64(t0);
      signed long long int n1 = SXBVConst64(t1);
      if(n0 <= n1)
	OutputNode = ASTTrue;
      else
	OutputNode = ASTFalse;
      break;
    }
    case BVSGT: {   
      signed long long int n0 = SXBVConst64(t0);
      signed long long int n1 = SXBVConst64(t1);
      if(n0 > n1)
	OutputNode = ASTTrue;
      else
	OutputNode = ASTFalse;
      break;
    }
    case BVSGE: {   
      signed long long int n0 = SXBVConst64(t0);
      signed long long int n1 = SXBVConst64(t1);
      if(n0 >= n1)
	OutputNode = ASTTrue;
      else
	OutputNode = ASTFalse;
      break;
    }
    default:
      FatalError("BVConstEvaluator: The input kind is not supported yet:",t);
      break;
    }    
   
    if(ASTTrue  != OutputNode && ASTFalse != OutputNode)
      OutputNode = CreateBVConst(inputwidth, output);
    UpdateSolverMap(t,OutputNode);
    //UpdateSimplifyMap(t,OutputNode,false);
    return OutputNode;
  } //End of BVConstEvaluator
#endif
//In the block below is the old string based version
//It is included here as an easy reference while the current code is being worked on.

/*
  ASTNode BeevMgr::BVConstEvaluator(const ASTNode& t) {
    ASTNode OutputNode;
    Kind k = t.GetKind();

    if(CheckSolverMap(t,OutputNode))
      return OutputNode;
    OutputNode = t;

    unsigned int inputwidth = t.GetValueWidth();
    unsigned * output = CONSTANTBV::BitVector_Create(inputwidth,true);
    unsigned * One = CONSTANTBV::BitVector_Create(inputwidth,true);
    CONSTANTBV::ErrCode e = CONSTANTBV::BitVector_from_Bin(One, (unsigned char*)"1");
    //error printing
    if(0 != e) {
      std::string ss("BVConstEvaluator:");
      ss += (const char*)BitVector_Error(e);	
      FatalError(ss.c_str(), t);
    }

    unsigned * Zero = CONSTANTBV::BitVector_Create(inputwidth,true);
    unsigned int * iii = One;
    unsigned int * jjj = Zero;

    //saving some typing. BVPLUS does not use these variables. if the
    //input BVPLUS has two nodes, then we want to avoid setting these
    //variables.
    if(2 == t.Degree() && k != BVPLUS && k != BVCONCAT) {
      iii = ConvertToCONSTANTBV(BVConstEvaluator(t[0]).GetBVConst());
      jjj = ConvertToCONSTANTBV(BVConstEvaluator(t[1]).GetBVConst());
    }

    char * cccc;
    switch(k) {
    case UNDEFINED:
    case READ:
    case WRITE:
    case SYMBOL:
      FatalError("BVConstEvaluator: term is not a constant-term",t);
      break;
    case BVCONST:
      OutputNode = t;
      break;
    case BVNEG:{
      //AARON
      if (iii != One) free(iii);
      //AARON

      iii = ConvertToCONSTANTBV(BVConstEvaluator(t[0]).GetBVConst());     
      CONSTANTBV::Set_Complement(output,iii);
      cccc = (char *)CONSTANTBV::BitVector_to_Bin(output);
      OutputNode = CreateBVConst(cccc,2);
      break;
    }
    case BVSX: {
      unsigned * out0 = ConvertToCONSTANTBV(BVConstEvaluator(t[0]).GetBVConst());
      unsigned t0_width = t[0].GetValueWidth();
      if(inputwidth == t0_width) {
	cccc = (char *)CONSTANTBV::BitVector_to_Bin(out0);
	OutputNode = CreateBVConst(cccc,2);

	//AARON
	free(cccc);
	//AARON

	CONSTANTBV::BitVector_Destroy(out0);     
      }
      else {
	// FIXME: (Dill) I'm guessing that BitVector sign returns 1 if the
	// number is positive, 0 if 0, and -1 if negative.  But I'm only
	// guessing.
	signed int topbit_sign = (CONSTANTBV::BitVector_Sign(out0) < 0);
	//out1 is the sign-extension bits
	unsigned * out1 =  CONSTANTBV::BitVector_Create(inputwidth-t0_width,true);      
	if(topbit_sign)
	  CONSTANTBV::BitVector_Fill(out1);

	//AARON
	CONSTANTBV::BitVector_Destroy(output);
	//AARON

	output = CONSTANTBV::BitVector_Concat(out1,out0);
	cccc = (char *)CONSTANTBV::BitVector_to_Bin(output);
	OutputNode = CreateBVConst(cccc,2);

	//AARON
	free(cccc);
	//AARON

	CONSTANTBV::BitVector_Destroy(out0);
	CONSTANTBV::BitVector_Destroy(out1);
      }
      break;
    }
    case BVAND: {
      CONSTANTBV::Set_Intersection(output,iii,jjj);
      cccc = (char *)CONSTANTBV::BitVector_to_Bin(output);
      OutputNode = CreateBVConst(cccc,2);

      //AARON
      free(cccc);
      //AARON
      
      break;
    }
    case BVOR: {
      CONSTANTBV::Set_Union(output,iii,jjj);
      cccc = (char *)CONSTANTBV::BitVector_to_Bin(output);
      OutputNode = CreateBVConst(cccc,2);

      //AARON
      free(cccc);
      //AARON

      break;
    }
    case BVXOR: {
      CONSTANTBV::Set_ExclusiveOr(output,iii,jjj);
      cccc = (char *)CONSTANTBV::BitVector_to_Bin(output);
      OutputNode = CreateBVConst(cccc,2);

      //AARON
      free(cccc);
      //AARON

      break;
    }
    case BVSUB: {
      bool carry = false;
      CONSTANTBV::BitVector_sub(output,iii,jjj,&carry);
      cccc = (char *)CONSTANTBV::BitVector_to_Bin(output);
      OutputNode = CreateBVConst(cccc,2);

      //AARON
      free(cccc);
      //AARON

      break;
    }
    case BVUMINUS: {
      bool carry = false;

      //AARON
      if (iii != One) free(iii);
      //AARON

      iii = ConvertToCONSTANTBV(BVConstEvaluator(t[0]).GetBVConst());
      CONSTANTBV::Set_Complement(output,iii);
      CONSTANTBV::BitVector_add(output,output,One,&carry);
      cccc = (char *)CONSTANTBV::BitVector_to_Bin(output);
      OutputNode = CreateBVConst(cccc,2);

      //AARON
      free(cccc);
      //AARON

      break;
    }
    case BVEXTRACT: {
      string s(BVConstEvaluator(t[0]).GetBVConst());
      unsigned int hi = GetUnsignedConst(BVConstEvaluator(t[1]));
      unsigned int low = GetUnsignedConst(BVConstEvaluator(t[2]));

      //length of substr to chop
      unsigned int len = hi-low+1;
      //distance from MSB
      hi = s.size()-1 - hi;      
      string ss = s.substr(hi,len);
      OutputNode = CreateBVConst(ss.c_str(),2);
      break;
    }
    case BVCONCAT: {
      string s(BVConstEvaluator(t[0]).GetBVConst());
      string r(BVConstEvaluator(t[1]).GetBVConst());
  
      string q(s+r);
      OutputNode = CreateBVConst(q.c_str(),2);
      break;
    }
    case BVMULT: {
      unsigned * output1 = CONSTANTBV::BitVector_Create(2*inputwidth,true);
      CONSTANTBV::ErrCode e = CONSTANTBV::BitVector_Multiply(output1,iii,jjj);
      //error printing
      if(0 != e) {
	std::string ss("BVConstEvaluator:");
	ss += (const char*)BitVector_Error(e);	
	//destroy all the CONSTANTBV bitvectors
	CONSTANTBV::BitVector_Destroy(iii);
	CONSTANTBV::BitVector_Destroy(jjj);       
	CONSTANTBV::BitVector_Destroy(One);
	CONSTANTBV::BitVector_Destroy(Zero);
	FatalError(ss.c_str(), t);
      }

      cccc = (char *)CONSTANTBV::BitVector_to_Bin(output1);
      std::string s(cccc);

      //AARON
      free(cccc);
      //AARON

      s = s.substr(inputwidth,inputwidth);
      OutputNode = CreateBVConst(s.c_str(),2);
      CONSTANTBV::BitVector_Destroy(output1);
      break;
    }
    case BVPLUS: {
      bool carry = false;
      ASTVec c = t.GetChildren();
      for(ASTVec::iterator it=c.begin(),itend=c.end();it!=itend;it++) {
	unsigned int * kk = ConvertToCONSTANTBV(BVConstEvaluator(*it).GetBVConst());
	CONSTANTBV::BitVector_add(output,output,kk,&carry);
	carry = false;
	CONSTANTBV::BitVector_Destroy(kk);
      }
      cccc = (char *)CONSTANTBV::BitVector_to_Bin(output);
      OutputNode = CreateBVConst(cccc,2);

      //AARON
      free(cccc);
      //AARON

      break;
    }
    case SBVDIV:
    case SBVMOD: {
      OutputNode = BVConstEvaluator(TranslateSignedDivMod(t));
      break;
    }
    case BVDIV: {      
      unsigned * quotient = CONSTANTBV::BitVector_Create(inputwidth,true);
      unsigned * remainder = CONSTANTBV::BitVector_Create(inputwidth,true);
      // iii is dividend, jjj is the divisor
      CONSTANTBV::ErrCode e = CONSTANTBV::BitVector_Div_Pos(quotient,iii,jjj,remainder);

      if(0 != e) {
	//error printing
	if(counterexample_checking_during_refinement) {
	  OutputNode = CreateZeroConst(inputwidth);
	  bvdiv_exception_occured = true;
	  break;
	}
	else {
	  std::string ss("BVConstEvaluator:");
	  ss += (const char*)BitVector_Error(e);	
	  //destroy all the CONSTANTBV bitvectors
	  CONSTANTBV::BitVector_Destroy(iii);
	  CONSTANTBV::BitVector_Destroy(jjj);       
	  CONSTANTBV::BitVector_Destroy(One);
	  CONSTANTBV::BitVector_Destroy(Zero);

	  //AARON
	  iii = jjj = One = Zero = NULL;
	  //AARON

	  FatalError(ss.c_str(), t);
	}
      } //end of error printing

      cccc = (char *)CONSTANTBV::BitVector_to_Bin(quotient);
      OutputNode = CreateBVConst(cccc,2);

      //AARON
      free(cccc);
      CONSTANTBV::BitVector_Destroy(quotient);
      CONSTANTBV::BitVector_Destroy(remainder);
      //AARON

      break;
    }
   case BVMOD: {
      unsigned * quotient = CONSTANTBV::BitVector_Create(inputwidth,true);
      unsigned * remainder = CONSTANTBV::BitVector_Create(inputwidth,true);
      // iii is dividend, jjj is the divisor
      CONSTANTBV::ErrCode e = CONSTANTBV::BitVector_Div_Pos(quotient,iii,jjj,remainder);

      if(0 != e) {
	//error printing
	if(counterexample_checking_during_refinement) {
	  OutputNode = CreateZeroConst(inputwidth);
	  bvdiv_exception_occured = true;
	  break;
	}
	else {
	  std::string ss("BVConstEvaluator:");
	  ss += (const char*)BitVector_Error(e);
	  //destroy all the CONSTANTBV bitvectors
	  CONSTANTBV::BitVector_Destroy(iii);
	  CONSTANTBV::BitVector_Destroy(jjj);       
	  CONSTANTBV::BitVector_Destroy(One);
	  CONSTANTBV::BitVector_Destroy(Zero);	

	  //AARON
	  iii = jjj = One = Zero = NULL;
	  //AARON

	  FatalError(ss.c_str(), t);
	}
      } //end of errory printing

      cccc = (char *)CONSTANTBV::BitVector_to_Bin(remainder);
      OutputNode = CreateBVConst(cccc,2);

      //AARON
      free(cccc);
      CONSTANTBV::BitVector_Destroy(quotient);
      CONSTANTBV::BitVector_Destroy(remainder);
      //AARON

      break;
    }
    case ITE:
      if(ASTTrue == t[0])
	OutputNode = BVConstEvaluator(t[1]);
      else if(ASTFalse == t[0])
	OutputNode = BVConstEvaluator(t[2]);
      else
	FatalError("BVConstEvaluator: ITE condiional must be either TRUE or FALSE:",t);
      break;
    case EQ: 
      if(CONSTANTBV::BitVector_equal(iii,jjj))
	OutputNode = ASTTrue;
      else
	OutputNode = ASTFalse;
      break;
    case NEQ:
      if(!CONSTANTBV::BitVector_equal(iii,jjj))
	OutputNode = ASTTrue;
      else
	OutputNode = ASTFalse;
      break;
    case BVLT:
      if(-1 == CONSTANTBV::BitVector_Lexicompare(iii,jjj))
	OutputNode = ASTTrue;
      else
	OutputNode = ASTFalse;
      break;
    case BVLE: {
      int comp = CONSTANTBV::BitVector_Lexicompare(iii,jjj);
      if(comp <= 0)
	OutputNode = ASTTrue;
      else
	OutputNode = ASTFalse;
      break;
    }
    case BVGT:
      if(1 == CONSTANTBV::BitVector_Lexicompare(iii,jjj))
	OutputNode = ASTTrue;
      else
	OutputNode = ASTFalse;
      break;
    case BVGE: {
      int comp = CONSTANTBV::BitVector_Lexicompare(iii,jjj);
      if(comp >= 0)
	OutputNode = ASTTrue;
      else
	OutputNode = ASTFalse;
      break;
    } 
    case BVSLT:
      if(-1 == CONSTANTBV::BitVector_Compare(iii,jjj))
	OutputNode = ASTTrue;
      else
	OutputNode = ASTFalse;
      break;
    case BVSLE: {
      signed int comp = CONSTANTBV::BitVector_Compare(iii,jjj);
      if(comp <= 0)
	OutputNode = ASTTrue;
      else
	OutputNode = ASTFalse;
      break;
    }
    case BVSGT:
      if(1 == CONSTANTBV::BitVector_Compare(iii,jjj))
	OutputNode = ASTTrue;
      else
	OutputNode = ASTFalse;
      break;
    case BVSGE: {
      int comp = CONSTANTBV::BitVector_Compare(iii,jjj);
      if(comp >= 0)
	OutputNode = ASTTrue;
      else
	OutputNode = ASTFalse;
      break;
    } 
    default:
      FatalError("BVConstEvaluator: The input kind is not supported yet:",t);
      break;
    }



    // //destroy all the CONSTANTBV bitvectors
//     CONSTANTBV::BitVector_Destroy(iii);
//     CONSTANTBV::BitVector_Destroy(jjj);
//     CONSTANTBV::BitVector_Destroy(output);

//     if(k == BVNEG || k == BVUMINUS)
//       CONSTANTBV::BitVector_Destroy(One);
//     else if(k == BVAND   || k == BVOR  || k == BVXOR   || k == BVSUB || 
// 	    k == BVMULT  || k == EQ    || k == NEQ     || k == BVLT  ||
// 	    k == BVLE    || k == BVGT  || k == BVGE    || k == BVSLT ||
// 	    k == BVSLE   || k == BVSGT || k == BVSGE) {
//       CONSTANTBV::BitVector_Destroy(One);
//       CONSTANTBV::BitVector_Destroy(Zero);
//     }    

    //AARON
    if (output != NULL) CONSTANTBV::BitVector_Destroy(output);
    if (One != NULL) CONSTANTBV::BitVector_Destroy(One);
    if (Zero != NULL) CONSTANTBV::BitVector_Destroy(Zero);
    if (iii != NULL && iii != One) CONSTANTBV::BitVector_Destroy(iii);
    if (jjj != NULL && jjj != Zero) CONSTANTBV::BitVector_Destroy(jjj);
    //AARON
   
    UpdateSolverMap(t,OutputNode);
    //UpdateSimplifyMap(t,OutputNode,false);
    return OutputNode;
  }


  unsigned int * ConvertToCONSTANTBV(const char * s) {
    unsigned int length = strlen(s);
    unsigned char * ccc = (unsigned char *)s;
    unsigned *  iii = CONSTANTBV::BitVector_Create(length,true);
    CONSTANTBV::ErrCode e = CONSTANTBV::BitVector_from_Bin(iii,ccc);
    //error printing
    if(0 != e) {
      cerr << "ConverToCONSTANTBV: wrong bin value: " << BitVector_Error(e);      
      FatalError("");
    }
    
    return iii;
  }
*/
}; //end of namespace BEEV
