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
  
  bool BeevMgr::CheckSimplifyMap(const ASTNode& key, 
				 ASTNode& output, bool pushNeg) {
    ASTNodeMap::iterator it, itend;
    it = pushNeg ? SimplifyNegMap.find(key) : SimplifyMap.find(key);
    itend = pushNeg ? SimplifyNegMap.end() : SimplifyMap.end();
    
    if(it != itend) {
      output = it->second;
      CountersAndStats("Successful_CheckSimplifyMap");
      return true;
    }

    if(pushNeg && (it = SimplifyMap.find(key)) != SimplifyMap.end()) {
      output = 
	(ASTFalse == it->second) ? 
	ASTTrue : 
	(ASTTrue == it->second) ? ASTFalse : CreateNode(NOT, it->second);
      CountersAndStats("2nd_Successful_CheckSimplifyMap");
      return true;
    }

    return false;
  }
  
  void BeevMgr::UpdateSimplifyMap(const ASTNode& key, const ASTNode& value, bool pushNeg) {
    if(pushNeg) 
      SimplifyNegMap[key] = value;
    else
      SimplifyMap[key] = value;
  }

  bool BeevMgr::CheckSubstitutionMap(const ASTNode& key, ASTNode& output) {
    ASTNodeMap::iterator it;
    if((it = SolverMap.find(key)) != SolverMap.end()) {
      output = it->second;
      return true;
    }
    return false;
  }
  
  bool BeevMgr::CheckSubstitutionMap(const ASTNode& key) {
    if(SolverMap.find(key) != SolverMap.end())	
      return true;
    else
      return false;
  }
  
  bool BeevMgr::UpdateSubstitutionMap(const ASTNode& e0, const ASTNode& e1) {
    int i = TermOrder(e0,e1);
    if(0 == i)
      return false;

    //e0 is of the form READ(Arr,const), and e1 is const, or
    //e0 is of the form var, and e1 is const    
    if(1 == i && !CheckSubstitutionMap(e0)) {
      SolverMap[e0] = e1;
      return true;
    }
    
    //e1 is of the form READ(Arr,const), and e0 is const, or
    //e1 is of the form var, and e0 is const
    if (-1 == i && !CheckSubstitutionMap(e1)) { 
      SolverMap[e1] = e0;
      return true;
    }

    return false;
  }

  bool BeevMgr::CheckMultInverseMap(const ASTNode& key, ASTNode& output) {
    ASTNodeMap::iterator it;
    if((it = MultInverseMap.find(key)) != MultInverseMap.end()) {
      output = it->second;
      return true;
    }
    return false;
  }

  void BeevMgr::UpdateMultInverseMap(const ASTNode& key, const ASTNode& value) {
      MultInverseMap[key] = value;
  }


  bool BeevMgr::CheckAlwaysTrueFormMap(const ASTNode& key) {
    ASTNodeSet::iterator it = AlwaysTrueFormMap.find(key);
    ASTNodeSet::iterator itend = AlwaysTrueFormMap.end();
    
    if(it != itend) {
      //cerr << "found:" << *it << endl;
      CountersAndStats("Successful_CheckAlwaysTrueFormMap");
      return true;
    }
    
    return false;
  }
  
  void BeevMgr::UpdateAlwaysTrueFormMap(const ASTNode& key) {
    AlwaysTrueFormMap.insert(key);
  }

  //if a is READ(Arr,const) or SYMBOL, and b is BVCONST then return 1
  //if b is READ(Arr,const) or SYMBOL, and a is BVCONST then return -1
  //
  //else return 0 by default
   int BeevMgr::TermOrder(const ASTNode& a, const ASTNode& b) {
    Kind k1 = a.GetKind();
    Kind k2 = b.GetKind();

    //a is of the form READ(Arr,const), and b is const, or
    //a is of the form var, and b is const
    if((k1 == READ 
	&& 
	a[0].GetKind() == SYMBOL && 
	a[1].GetKind() == BVCONST
	)
       && 
       (k2 == BVCONST)
       )
      return 1;

    if(k1 == SYMBOL) 
      return 1;

    //b is of the form READ(Arr,const), and a is const, or
    //b is of the form var, and a is const
    if((k1  == BVCONST)
       &&
       ((k2 == READ
	 && 
	 b[0].GetKind() == SYMBOL &&
	 b[1].GetKind() == BVCONST
	 ) 
	 ||
	k2 == SYMBOL
	))
      return -1;
    return 0;
  }

  //This function records all the const-indices seen so far for each
  //array. It populates the map '_arrayname_readindices' whose key is
  //the arrayname, and vlaue is a vector of read-indices.
  //
  //fill the arrayname_readindices vector if e0 is a READ(Arr,index)
  //and index is a BVCONST.  
  //
  //Since these arrayreads are being nuked and recorded in the
  //substitutionmap, we have to also record the fact that each
  //arrayread (e0 is of the form READ(Arr,const) here is represented
  //by a BVCONST (e1). This is necessary for later Leibnitz Axiom
  //generation
  void BeevMgr::FillUp_ArrReadIndex_Vec(const ASTNode& e0, const ASTNode& e1) {
    int i = TermOrder(e0,e1);
    if(0 == i) return;

    if(1 == i && e0.GetKind() != SYMBOL && !CheckSubstitutionMap(e0)) {
      _arrayname_readindices[e0[0]].push_back(e0[1]);
      //e0 is the array read : READ(A,i) and e1 is a bvconst
      _arrayread_symbol[e0] = e1;
      return;
    }
    if(-1 == i && e1.GetKind() != SYMBOL &&  !CheckSubstitutionMap(e1)) {
      _arrayname_readindices[e1[0]].push_back(e1[1]);
      //e0 is the array read : READ(A,i) and e1 is a bvconst
      _arrayread_symbol[e1] = e0;
      return;
    }
  }

  ASTNode BeevMgr::SimplifyFormula_NoRemoveWrites(const ASTNode& b, bool pushNeg) {
    Begin_RemoveWrites = false;
    ASTNode out = SimplifyFormula(b,pushNeg);
    return out;
  }

  ASTNode BeevMgr::SimplifyFormula_TopLevel(const ASTNode& b, bool pushNeg) {
    SimplifyMap.clear();
    SimplifyNegMap.clear();
    ASTNode out = SimplifyFormula(b,pushNeg);
    SimplifyMap.clear();
    SimplifyNegMap.clear();
    return out;
  }

  ASTNode BeevMgr::SimplifyFormula(const ASTNode& b, bool pushNeg){
    if(!optimize)
      return b;

    Kind kind = b.GetKind();
    if(BOOLEAN_TYPE != b.GetType()) {
      FatalError(" SimplifyFormula: You have input a nonformula kind: ",ASTUndefined,kind);
    }
    
    ASTNode a = b;
    ASTVec ca = a.GetChildren();
    if(!(IMPLIES == kind || 
	 ITE == kind ||
	 isAtomic(kind))) {
      SortByExprNum(ca);
      a = CreateNode(kind,ca);
    }

    ASTNode output;
    if(CheckSimplifyMap(a,output,pushNeg))
      return output;

    switch(kind){
    case AND:
    case OR:
      output = SimplifyAndOrFormula(a,pushNeg);
      break;
    case NOT:
      output = SimplifyNotFormula(a,pushNeg);
      break;
    case XOR:
      output = SimplifyXorFormula(a,pushNeg);
      break;
    case NAND:
      output = SimplifyNandFormula(a,pushNeg);
      break;
    case NOR:
      output = SimplifyNorFormula(a,pushNeg);
      break;
    case IFF:
      output = SimplifyIffFormula(a,pushNeg);
      break;
    case IMPLIES:
      output = SimplifyImpliesFormula(a,pushNeg);
      break;
    case ITE:
      output = SimplifyIteFormula(a,pushNeg);
      break;
    default:
      //kind can be EQ,NEQ,BVLT,BVLE,... or a propositional variable
      output = SimplifyAtomicFormula(a,pushNeg);
      //output = pushNeg ? CreateNode(NOT,a) : a;
      break;
    }

    //memoize
    UpdateSimplifyMap(a,output, pushNeg);
    return output;
  }
    
  ASTNode BeevMgr::SimplifyAtomicFormula(const ASTNode& a, bool pushNeg) {    
    if(!optimize) {
      return a;
    }

    ASTNode output;
    if(CheckSimplifyMap(a,output,pushNeg)) {
      return output;
    }

    ASTNode left,right;
    if(a.Degree() == 2) {
      //cerr << "Input to simplifyterm: left: " << a[0] << endl;
      left = SimplifyTerm(a[0]);
      //cerr << "Output of simplifyterm:left: " << left << endl;
      //cerr << "Input to simplifyterm: right: " << a[1] << endl;
      right = SimplifyTerm(a[1]);
      //cerr << "Output of simplifyterm:left: " << right << endl;
    }

    Kind kind = a.GetKind();
    switch(kind) {
    case TRUE:
      output = pushNeg ? ASTFalse : ASTTrue;
      break;
    case FALSE:
      output = pushNeg ? ASTTrue : ASTFalse;
      break;
    case SYMBOL:
      if(!CheckSolverMap(a,output)) {
	output = a;
      }
      output = pushNeg ? CreateNode(NOT,output) : output;           
      break;
    case BVGETBIT: {
      ASTNode term = SimplifyTerm(a[0]);
      ASTNode thebit = a[1];
      ASTNode zero = CreateZeroConst(1);
      ASTNode one = CreateOneConst(1);
      ASTNode getthebit = SimplifyTerm(CreateTerm(BVEXTRACT,1,term,thebit,thebit));
      if(getthebit == zero)
	output = pushNeg ? ASTTrue : ASTFalse;
      else if(getthebit == one)
	output = pushNeg ? ASTFalse : ASTTrue;
      else {
	output = CreateNode(BVGETBIT,term,thebit);
	output = pushNeg ? CreateNode(NOT,output) : output;
      }
      break;
    }
    case EQ:{
      output = CreateSimplifiedEQ(left,right);
      output = LhsMinusRhs(output);
      output = ITEOpt_InEqs(output);
      if(output == ASTTrue)
	output = pushNeg ? ASTFalse : ASTTrue;
      else if (output == ASTFalse)
	output = pushNeg ? ASTTrue : ASTFalse;
      else
	output = pushNeg ? CreateNode(NOT,output) : output;
      break;  
    } 
    case NEQ: {
      output = CreateSimplifiedEQ(left,right);
      output = LhsMinusRhs(output);
      if(output == ASTTrue)
      	output = pushNeg ? ASTTrue : ASTFalse;
      else if (output == ASTFalse)
      	output = pushNeg ? ASTFalse : ASTTrue;
      else
	output = pushNeg ? output : CreateNode(NOT,output);
      break;
    }
    case BVLT:
    case BVLE:
    case BVGT:
    case BVGE:
    case BVSLT:
    case BVSLE:
    case BVSGT:
    case BVSGE: {
      //output = CreateNode(kind,left,right);
      //output = pushNeg ? CreateNode(NOT,output) : output;      
      output = CreateSimplifiedINEQ(kind,left,right,pushNeg);
      break;
    }
    default:
      FatalError("SimplifyAtomicFormula: NO atomic formula of the kind: ",ASTUndefined,kind);
      break;      
    }

    //memoize
    UpdateSimplifyMap(a,output,pushNeg);
    return output;
  } //end of SimplifyAtomicFormula()

  ASTNode BeevMgr::CreateSimplifiedINEQ(Kind k, 
					const ASTNode& left, 
					const ASTNode& right, 
					bool pushNeg) {
    ASTNode output;
    if(BVCONST == left.GetKind() && BVCONST == right.GetKind()) {
      output = BVConstEvaluator(CreateNode(k,left,right));
      output = pushNeg ? (ASTFalse == output) ? ASTTrue : ASTFalse : output;
      return output;
    }

    unsigned len = left.GetValueWidth();
    ASTNode zero = CreateZeroConst(len);
    ASTNode one = CreateOneConst(len);
    ASTNode max = CreateMaxConst(len);
    switch(k){
    case BVLT:
      if(right == zero) {
	output = pushNeg ? ASTTrue : ASTFalse;
      }
      else if(left == right) {
	output = pushNeg ? ASTTrue : ASTFalse;
      }
      else if(one == right) {
	output = CreateSimplifiedEQ(left,zero);
	output = pushNeg ? CreateNode(NOT,output) : output;
      }
      else {
	output = pushNeg ? CreateNode(BVLE,right,left) : CreateNode(BVLT,left,right);
      }
      break;
    case BVLE:
      if(left == zero) {
	output = pushNeg ? ASTFalse : ASTTrue;
      }
      else if(left == right) {
	output = pushNeg ? ASTFalse : ASTTrue;
      }
      else if(max == right) {
	output = pushNeg ? ASTFalse : ASTTrue;
      }
      else if(zero == right) {
	output = CreateSimplifiedEQ(left,zero);
	output = pushNeg ? CreateNode(NOT,output) : output;
      }
      else {
	output = pushNeg ? CreateNode(BVLT,right,left) : CreateNode(BVLE,left,right);
      }
      break;
    case BVGT:
      if(left == zero) {
	output = pushNeg ? ASTTrue : ASTFalse;
      }
      else if(left == right) {
	output = pushNeg ? ASTTrue : ASTFalse;
      }
      else {
      output = pushNeg ? CreateNode(BVLE,left,right) : CreateNode(BVLT,right,left);
      }
      break;
    case BVGE:
      if(right == zero) {
	output = pushNeg ? ASTFalse : ASTTrue;
      }
      else if(left == right) {
	output = pushNeg ? ASTFalse : ASTTrue;
      }
      else {
      output = pushNeg ? CreateNode(BVLT,left,right) : CreateNode(BVLE,right,left);
      }
      break;
    case BVSLT:
    case BVSLE:
    case BVSGE:
    case BVSGT: {
      output = CreateNode(k,left,right);
      output = pushNeg ? CreateNode(NOT,output) : output;
    }
      break;
    default:
      FatalError("Wrong Kind");
      break;
    }

    return output;
  }

  //takes care of some simple ITE Optimizations in the context of equations
  ASTNode BeevMgr::ITEOpt_InEqs(const ASTNode& in) {
    CountersAndStats("ITEOpts_InEqs");

    if(!(EQ == in.GetKind() && optimize)) {
      return in;
    }

    ASTNode output;
    if(CheckSimplifyMap(in,output,false)) {
      return output;
    }

    ASTNode in1 = in[0];
    ASTNode in2 = in[1];
    Kind k1 = in1.GetKind();
    Kind k2 = in2.GetKind();
    if(in1 == in2) {
      //terms are syntactically the same
      output = ASTTrue;    
    }
    else if(BVCONST == k1 && BVCONST == k2) {
      //here the terms are definitely not syntactically equal but may
      //be semantically equal.
      output = ASTFalse;
    }
    else if(ITE == k1 && 
	    BVCONST == in1[1].GetKind() && 
	    BVCONST == in1[2].GetKind() && BVCONST == k2) {
      //if one side is a BVCONST and the other side is an ITE over
      //BVCONST then we can do the following optimization:
      //
      // c = ITE(cond,c,d) <=> cond
      //
      // similarly ITE(cond,c,d) = c <=> cond
      //
      // c = ITE(cond,d,c) <=> NOT(cond) 
      //
      //similarly ITE(cond,d,c) = d <=> NOT(cond)
      ASTNode cond = in1[0];
      if(in1[1] == in2) {
	//ITE(cond, c, d) = c <=> cond
	output = cond;
      }
      else if(in1[2] == in2) {
	cond = SimplifyFormula(cond,true);
	output = cond;
      }
      else {
	//last resort is to CreateNode
	output = CreateNode(EQ,in1,in2);   
      }    
    }    
    else if(ITE == k2 && 
	    BVCONST == in2[1].GetKind() && 
	    BVCONST == in2[2].GetKind() && BVCONST == k1) {
      ASTNode cond = in2[0];
      if(in2[1] == in1) {
	//ITE(cond, c, d) = c <=> cond
	output = cond;
      }
      else if(in2[2] == in1) {
	cond = SimplifyFormula(cond,true);
	output = cond;
      }
      else {
	//last resort is to CreateNode
	output = CreateNode(EQ,in1,in2);   
      }    
    }
    else {
      //last resort is to CreateNode
      output = CreateNode(EQ,in1,in2);   
    }
    
    UpdateSimplifyMap(in,output,false);
    return output;
 } //End of ITEOpts_InEqs()

  //Tries to simplify the input to TRUE/FALSE. if it fails, then
  //return the constructed equality
  ASTNode BeevMgr::CreateSimplifiedEQ(const ASTNode& in1, const ASTNode& in2) {
    CountersAndStats("CreateSimplifiedEQ");        
    Kind k1 = in1.GetKind();
    Kind k2 = in2.GetKind();

    if(!optimize) {
      return CreateNode(EQ,in1,in2);
    }
    
    if(in1 == in2)
      //terms are syntactically the same
      return ASTTrue;    
    
    //here the terms are definitely not syntactically equal but may be
    //semantically equal.    
    if(BVCONST == k1 && BVCONST == k2)
      return ASTFalse;
    
    //last resort is to CreateNode
    return CreateNode(EQ,in1,in2);
  }
  
  //accepts cond == t1, then part is t2, and else part is t3
  ASTNode BeevMgr::CreateSimplifiedTermITE(const ASTNode& in0, 
					   const ASTNode& in1, const ASTNode& in2) {
    ASTNode t0 = in0;
    ASTNode t1 = in1;
    ASTNode t2 = in2;
    CountersAndStats("CreateSimplifiedITE");
    if(!optimize) {
      if(t1.GetValueWidth() != t2.GetValueWidth()) {
	cerr << "t2 is : = " << t2;
	FatalError("CreateSimplifiedTermITE: the lengths of then and else branches don't match",t1);
      }
      if(t1.GetIndexWidth() != t2.GetIndexWidth()) {
	cerr << "t2 is : = " << t2;
	FatalError("CreateSimplifiedTermITE: the lengths of then and else branches don't match",t1);
      }
      return CreateTerm(ITE,t1.GetValueWidth(),t0,t1,t2);
    }

    if(t0 == ASTTrue)
      return t1;
    if (t0 == ASTFalse)
      return t2;    
    if(t1 == t2)
      return t1;    
    if(CheckAlwaysTrueFormMap(t0)) {
	return t1;
    }     
    if(CheckAlwaysTrueFormMap(CreateNode(NOT,t0)) || 
       (NOT == t0.GetKind() && CheckAlwaysTrueFormMap(t0[0]))) {
      return t2;
    }
    
    return CreateTerm(ITE,t1.GetValueWidth(),t0,t1,t2);
  }

  ASTNode BeevMgr::SimplifyAndOrFormula(const ASTNode& a, bool pushNeg) {
    ASTNode output;
    //cerr << "input:\n" << a << endl;

    if(CheckSimplifyMap(a,output,pushNeg))
      return output;

    ASTVec c, outvec;
    c = a.GetChildren();
    ASTNode flat = FlattenOneLevel(a);
    c = flat.GetChildren();
    SortByExprNum(c);

    Kind k = a.GetKind();
    bool isAnd = (k == AND) ? true : false;

    ASTNode annihilator = isAnd ? 
      (pushNeg ? ASTTrue : ASTFalse): 
      (pushNeg ? ASTFalse : ASTTrue);

    ASTNode identity = isAnd ? 
      (pushNeg ? ASTFalse : ASTTrue): 
      (pushNeg ? ASTTrue : ASTFalse);

    //do the work
    ASTVec::const_iterator next_it;
    for(ASTVec::const_iterator i=c.begin(),iend=c.end();i!=iend;i++) {
      ASTNode aaa = *i;
      next_it = i+1;
      bool nextexists = (next_it < iend);
      
      aaa = SimplifyFormula(aaa,pushNeg);
      if(annihilator == aaa) {
	//memoize
	UpdateSimplifyMap(*i,annihilator,pushNeg);
	UpdateSimplifyMap(a, annihilator,pushNeg);
	//cerr << "annihilator1: output:\n" << annihilator << endl;
	return annihilator;
      }
      ASTNode bbb = ASTFalse;
      if(nextexists) {
      	bbb = SimplifyFormula(*next_it,pushNeg);
      }      
      if(nextexists &&  bbb == aaa) {
      	//skip the duplicate aaa. *next_it will be included
      }
      else if(nextexists && 
      	      ((bbb.GetKind() == NOT && bbb[0] == aaa))) {
      	//memoize
      	UpdateSimplifyMap(a, annihilator,pushNeg);
      	//cerr << "annihilator2: output:\n" << annihilator << endl;
      	return annihilator;
      }
      else if(identity == aaa) {
	// //drop identites
      }
      else if((!isAnd && !pushNeg) ||
	      (isAnd && pushNeg)) { 
	outvec.push_back(aaa);	
      }
      else if((isAnd && !pushNeg) ||
	      (!isAnd && pushNeg)) {
	outvec.push_back(aaa);	
      }
      else {
	outvec.push_back(aaa);
      }
    }

    switch(outvec.size()) {
    case 0: {
      //only identities were dropped 
      output = identity;
      break;
    }
    case 1: {
      output = SimplifyFormula(outvec[0],false);
      break;
    }
    default: {
      output = (isAnd) ? 
	(pushNeg ? CreateNode(OR,outvec) : CreateNode(AND,outvec)): 
	(pushNeg ? CreateNode(AND,outvec) : CreateNode(OR,outvec));
      //output = FlattenOneLevel(output);
      break;
    }
    }
    //memoize
    UpdateSimplifyMap(a,output,pushNeg);
    //cerr << "output:\n" << output << endl;
    return output;
  } //end of SimplifyAndOrFormula


  ASTNode BeevMgr::SimplifyNotFormula(const ASTNode& a, bool pushNeg) {
    ASTNode output;
    if(CheckSimplifyMap(a,output,pushNeg))
      return output;

    if(!(a.Degree() == 1 && NOT == a.GetKind())) 
      FatalError("SimplifyNotFormula: input vector with more than 1 node",ASTUndefined);

    //if pushNeg is set then there is NOT on top
    unsigned int NotCount = pushNeg ? 1 : 0;
    ASTNode o = a;
    //count the number of NOTs in 'a'
    while(NOT == o.GetKind()) {
      o = o[0];
      NotCount++;
    }

    //pushnegation if there are odd number of NOTs
    bool pn = (NotCount % 2 == 0) ? false : true;

    if(CheckAlwaysTrueFormMap(o)) {
      output = pn ? ASTFalse : ASTTrue;
      return output;
    }

    if(CheckSimplifyMap(o,output,pn)) {
      return output;
    }

    if (ASTTrue == o) {
      output = pn ? ASTFalse : ASTTrue;
    }
    else if (ASTFalse == o) {
      output = pn ? ASTTrue : ASTFalse;
    }
    else {
      output = SimplifyFormula(o,pn);
    }
    //memoize
    UpdateSimplifyMap(o,output,pn);
    UpdateSimplifyMap(a,output,pushNeg);
    return output;
  }

  ASTNode BeevMgr::SimplifyXorFormula(const ASTNode& a, bool pushNeg) {
    ASTNode output;
    if(CheckSimplifyMap(a,output,pushNeg))
      return output;

    if (a.GetChildren().size() > 2) {
      FatalError("Simplify got an XOR with more than two children.");
    }

    ASTNode a0 = SimplifyFormula(a[0],false);
    ASTNode a1 = SimplifyFormula(a[1],false);        
    output = pushNeg ? CreateNode(IFF,a0,a1) : CreateNode(XOR,a0,a1);
    
    if(XOR == output.GetKind()) {
      a0 = output[0];
      a1 = output[1];
      if(a0 == a1)
	output = ASTFalse;
      else if((a0 == ASTTrue  && a1 == ASTFalse) ||
	      (a0 == ASTFalse && a1 == ASTTrue))
	output = ASTTrue;
    }

    //memoize
    UpdateSimplifyMap(a,output,pushNeg);
    return output;
  }

  ASTNode BeevMgr::SimplifyNandFormula(const ASTNode& a, bool pushNeg) {
    ASTNode output,a0,a1;
    if(CheckSimplifyMap(a,output,pushNeg))
      return output;

    //the two NOTs cancel out
    if(pushNeg) {
      a0 = SimplifyFormula(a[0],false);
      a1 = SimplifyFormula(a[1],false);
      output = CreateNode(AND,a0,a1);
    }
    else {
      //push the NOT implicit in the NAND
      a0 = SimplifyFormula(a[0],true);
      a1 = SimplifyFormula(a[1],true);
      output = CreateNode(OR,a0,a1);
    }

    //memoize
    UpdateSimplifyMap(a,output,pushNeg);
    return output;
  }

  ASTNode BeevMgr::SimplifyNorFormula(const ASTNode& a, bool pushNeg) {
    ASTNode output,a0,a1;
    if(CheckSimplifyMap(a,output,pushNeg))
      return output;

    //the two NOTs cancel out
    if(pushNeg) {
      a0 = SimplifyFormula(a[0],false);
      a1 = SimplifyFormula(a[1],false);
      output = CreateNode(OR,a0,a1);
    }
    else {
      //push the NOT implicit in the NAND
      a0 = SimplifyFormula(a[0],true);
      a1 = SimplifyFormula(a[1],true);
      output = CreateNode(AND,a0,a1);
    }

    //memoize
    UpdateSimplifyMap(a,output,pushNeg);
    return output;
  }

  ASTNode BeevMgr::SimplifyImpliesFormula(const ASTNode& a, bool pushNeg) {
    ASTNode output;
    if(CheckSimplifyMap(a,output,pushNeg))
      return output;

    if(!(a.Degree()==2 && IMPLIES==a.GetKind()))
      FatalError("SimplifyImpliesFormula: vector with wrong num of nodes",ASTUndefined);
    
    ASTNode c0,c1;
    if(pushNeg) {
      c0 = SimplifyFormula(a[0],false);
      c1 = SimplifyFormula(a[1],true);
      output = CreateNode(AND,c0,c1);
    }
    else {
      c0 = SimplifyFormula(a[0],false);
      c1 = SimplifyFormula(a[1],false);
      if(ASTFalse == c0) {
	output = ASTTrue;
      }
      else if (ASTTrue == c0) {
	output = c1;
      }
      else if (c0 == c1) {
	output = ASTTrue;
      }
      else if(CheckAlwaysTrueFormMap(c0)) {
	// c0 AND (~c0 OR c1) <==> c1
	//
	//applying modus ponens
	output = c1;
      }
      else if(CheckAlwaysTrueFormMap(c1)	||
	      CheckAlwaysTrueFormMap(CreateNode(NOT,c0)) ||
	      (NOT == c0.GetKind() && CheckAlwaysTrueFormMap(c0[0]))) {
	//(~c0 AND (~c0 OR c1)) <==> TRUE
	//
	//(c0 AND ~c0->c1) <==> TRUE
	output = ASTTrue;
      }
      else if (CheckAlwaysTrueFormMap(CreateNode(NOT,c1)) ||
	       (NOT == c1.GetKind() && CheckAlwaysTrueFormMap(c1[0]))) {
	//(~c1 AND c0->c1) <==> (~c1 AND ~c1->~c0) <==> ~c0
	//(c1 AND c0->~c1) <==> (c1 AND c1->~c0) <==> ~c0
	output = CreateNode(NOT,c0);
      }
      else {
	if(NOT == c0.GetKind()) {
	  output = CreateNode(OR,c0[0],c1);
	}
	else {
	  output = CreateNode(OR,CreateNode(NOT,c0),c1);
	}
      }
    }

    //memoize
    UpdateSimplifyMap(a,output,pushNeg);
    return output;
  }

  ASTNode BeevMgr::SimplifyIffFormula(const ASTNode& a, bool pushNeg) {
    ASTNode output;
    if(CheckSimplifyMap(a,output,pushNeg))
      return output;

    if(!(a.Degree()==2 && IFF==a.GetKind()))
      FatalError("SimplifyIffFormula: vector with wrong num of nodes",ASTUndefined);
    
    ASTNode c0 = a[0];
    ASTNode c1 = SimplifyFormula(a[1],false);

    if(pushNeg)
      c0 = SimplifyFormula(c0,true);
    else 
      c0 = SimplifyFormula(c0,false);

    if(ASTTrue == c0) {
      output = c1;
    }
    else if (ASTFalse == c0) {
      output = SimplifyFormula(c1,true);
    }
    else if (ASTTrue == c1) {
      output = c0;
    }
    else if (ASTFalse == c1) {
      output = SimplifyFormula(c0,true);
    }
    else if (c0 == c1) {
      output = ASTTrue;
    }
    else if((NOT == c0.GetKind() && c0[0] == c1) ||
	    (NOT == c1.GetKind() && c0 == c1[0])) {
      output = ASTFalse;
    }
    else if(CheckAlwaysTrueFormMap(c0)) {
      output = c1;
    }
    else if(CheckAlwaysTrueFormMap(c1)) {
      output = c0;
    }
    else if(CheckAlwaysTrueFormMap(CreateNode(NOT,c0))) {
      output = CreateNode(NOT,c1);
    }
    else if(CheckAlwaysTrueFormMap(CreateNode(NOT,c1))) {
      output = CreateNode(NOT,c0);
    }
    else {
      output = CreateNode(IFF,c0,c1);
    }

    //memoize
    UpdateSimplifyMap(a,output,pushNeg);
    return output;
  }

  ASTNode BeevMgr::SimplifyIteFormula(const ASTNode& b, bool pushNeg) {
    if(!optimize)
      return b;

    ASTNode output;
    if(CheckSimplifyMap(b,output,pushNeg))
      return output;

    if(!(b.Degree() == 3 && ITE == b.GetKind()))
      FatalError("SimplifyIteFormula: vector with wrong num of nodes",ASTUndefined);    
    
    ASTNode a = b;
    ASTNode t0 = SimplifyFormula(a[0],false);
    ASTNode t1,t2;
    if(pushNeg) {
      t1 = SimplifyFormula(a[1],true);
      t2 = SimplifyFormula(a[2],true);
    }
    else {
      t1 = SimplifyFormula(a[1],false);
      t2 = SimplifyFormula(a[2],false);
    }
    
    if(ASTTrue == t0) {
      output = t1;
    }
    else if (ASTFalse == t0) {
      output = t2;
    }
    else if (t1 == t2) {
      output = t1;
    }
    else if(ASTTrue == t1 && ASTFalse == t2) {
      output = t0;
    }
    else if(ASTFalse == t1 && ASTTrue == t2) {
      output = SimplifyFormula(t0,true);
    }
    else if(ASTTrue == t1) {
      output = CreateNode(OR,t0,t2);
    }
    else if(ASTFalse == t1) {
      output = CreateNode(AND,CreateNode(NOT,t0),t2);
    }
    else if(ASTTrue == t2) {
      output = CreateNode(OR,CreateNode(NOT,t0),t1);
    }
    else if(ASTFalse == t2) {
      output = CreateNode(AND,t0,t1);
    }
    else if(CheckAlwaysTrueFormMap(t0)) {
      output = t1;
    }
    else if(CheckAlwaysTrueFormMap(CreateNode(NOT,t0)) ||
    	    (NOT == t0.GetKind() && CheckAlwaysTrueFormMap(t0[0]))) {
      output = t2;
    }
    else {
      output = CreateNode(ITE,t0,t1,t2);
    }

    //memoize
    UpdateSimplifyMap(a,output,pushNeg);
    return output;    
  }

  //one level deep flattening
  ASTNode BeevMgr::FlattenOneLevel(const ASTNode& a) {
    Kind k = a.GetKind();
    if(!(BVPLUS == k || 
	 AND == k || OR == k
	 //|| BVAND == k 
	 //|| BVOR == k
	 )
       ) {
      return a;
    }
    
    ASTNode output;
    // if(CheckSimplifyMap(a,output,false)) {
    //       //check memo table
    //       //cerr << "output of SimplifyTerm Cache: " << output << endl;
    //       return output;
    //     }

    ASTVec c = a.GetChildren();
    ASTVec o;
    for(ASTVec::iterator it=c.begin(),itend=c.end();it!=itend;it++) {
      ASTNode aaa = *it;
      if(k == aaa.GetKind()) {
	ASTVec ac = aaa.GetChildren();
	o.insert(o.end(),ac.begin(),ac.end());
      }
      else
	o.push_back(aaa);      
    } 
    
    if(is_Form_kind(k))
      output = CreateNode(k,o);
    else
      output = CreateTerm(k,a.GetValueWidth(),o);

    //UpdateSimplifyMap(a,output,false);
    return output;
    //memoize
  } //end of flattenonelevel()

  ASTNode BeevMgr::SimplifyTerm_TopLevel(const ASTNode& b) {
    SimplifyMap.clear();
    SimplifyNegMap.clear();
    ASTNode out = SimplifyTerm(b);
    SimplifyNegMap.clear();
    SimplifyMap.clear();
    return out;
  }

  //This function simplifies terms based on their kind
  ASTNode BeevMgr::SimplifyTerm(const ASTNode& inputterm) {
    //cout << "SimplifyTerm: input: " << a << endl;
    if(!optimize) {
      return inputterm;
    }

    BVTypeCheck(inputterm);
    ASTNode output;
    if(wordlevel_solve && CheckSolverMap(inputterm,output)) {
      //cout << "SimplifyTerm: output: " << output << endl;
      return SimplifyTerm(output);     
    }

    if(CheckSimplifyMap(inputterm,output,false)) {
      //cerr << "output of SimplifyTerm Cache: " << output << endl;
      return output;
    }

    Kind k = inputterm.GetKind();    
    if(!is_Term_kind(k)) {
      FatalError("SimplifyTerm: You have input a Non-term",ASTUndefined);
    }

    unsigned int inputValueWidth = inputterm.GetValueWidth();
    switch(k) {
    case BVCONST:
      output = inputterm;
      break;
    case SYMBOL:
      if(CheckSolverMap(inputterm,output)) {
	return SimplifyTerm(output);
      }
      output = inputterm;
      break;
    case BVMULT:
    case BVPLUS:{
      if(BVMULT == k && 2 != inputterm.Degree()) {
	FatalError("SimplifyTerm: We assume that BVMULT is binary",inputterm);
      }
      
      ASTVec c = FlattenOneLevel(inputterm).GetChildren();
      SortByExprNum(c);
      ASTVec constkids, nonconstkids;

      //go through the childnodes, and separate constant and
      //nonconstant nodes. combine the constant nodes using the
      //constevaluator. if the resultant constant is zero and k ==
      //BVPLUS, then ignore it (similarily for 1 and BVMULT).  else,
      //add the computed constant to the nonconst vector, flatten,
      //sort, and create BVPLUS/BVMULT and return
      for(ASTVec::iterator it=c.begin(),itend=c.end();it!=itend;it++) {
	ASTNode aaa = SimplifyTerm(*it);
	if(BVCONST == aaa.GetKind()) {
	  constkids.push_back(aaa);
	}
	else {
	  nonconstkids.push_back(aaa);
	}
      }
      
      ASTNode one = CreateOneConst(inputValueWidth);
      ASTNode max = CreateMaxConst(inputValueWidth);
      ASTNode zero = CreateZeroConst(inputValueWidth);

      //initialize constoutput to zero, in case there are no elements
      //in constkids
      ASTNode constoutput = (k == BVPLUS) ? zero : one;

      if(1 == constkids.size()) {
	//only one element in constkids
	constoutput = constkids[0];
      } 
      else if (1 < constkids.size()) {
	//many elements in constkids. simplify it
	constoutput = CreateTerm(k,inputterm.GetValueWidth(),constkids);
	constoutput = BVConstEvaluator(constoutput);
      }

      if(BVMULT == k && zero == constoutput) {
	output = zero;
      }
      else if(BVMULT == k && 
	      1 == nonconstkids.size() && 
	      constoutput == max) {
	//useful special case opt: when input is BVMULT(max_const,t),
	//then output = BVUMINUS(t). this is easier on the bitblaster
	output = CreateTerm(BVUMINUS,inputValueWidth,nonconstkids);
      }
      else {
	if(0 < nonconstkids.size()) {
	  //nonconstkids is not empty. First, combine const and
	  //nonconstkids
	  if(BVPLUS == k && constoutput != zero) {
	    nonconstkids.push_back(constoutput);
	  }
	  else if(BVMULT == k && constoutput != one) {
	    nonconstkids.push_back(constoutput);
	  }

	  if(1 == nonconstkids.size()) {
	    //exactly one element in nonconstkids. output is exactly
	    //nonconstkids[0]
	    output = nonconstkids[0];
	  }
	  else {
	    //more than 1 element in nonconstkids. create BVPLUS term
	    SortByExprNum(nonconstkids);
	    output = CreateTerm(k,inputValueWidth,nonconstkids);
	    output = FlattenOneLevel(output);
	    output = DistributeMultOverPlus(output,true);
	    output = CombineLikeTerms(output);
 	  }
	}
	else {
	  //nonconstkids was empty, all childnodes were constant, hence
	  //constoutput is the output.
	  output = constoutput;
	}
      }
      if(BVMULT == output.GetKind() 
	 || BVPLUS == output.GetKind()
	 ) {
	ASTVec d = output.GetChildren();
	SortByExprNum(d);
      	output = CreateTerm(output.GetKind(),output.GetValueWidth(),d);
      }
      break;
    }
    case BVSUB: {
      ASTVec c = inputterm.GetChildren();
      ASTNode a0 = SimplifyTerm(inputterm[0]);
      ASTNode a1 = SimplifyTerm(inputterm[1]);
      unsigned int l = inputValueWidth;
      if(a0 == a1)
	output = CreateZeroConst(l);
      else {
	//covert x-y into x+(-y) and simplify. this transformation
	//triggers more simplifications
	a1 = SimplifyTerm(CreateTerm(BVUMINUS,l,a1));
	output = SimplifyTerm(CreateTerm(BVPLUS,l,a0,a1));
      }
      break;
    }
    case BVUMINUS: {
      //important to treat BVUMINUS as a special case, because it
      //helps in arithmetic transformations. e.g.  x + BVUMINUS(x) is
      //actually 0. One way to reveal this fact is to strip bvuminus
      //out, and replace with something else so that combineliketerms
      //can catch this fact.
      ASTNode a0 = SimplifyTerm(inputterm[0]);
      Kind k1 = a0.GetKind();
      unsigned int l = a0.GetValueWidth();
      ASTNode one = CreateOneConst(l);
      switch(k1) {
      case BVUMINUS:
	output = a0[0];
	break;
      case BVCONST: {
	output = BVConstEvaluator(CreateTerm(BVUMINUS,l,a0));
	break;
      }
      case BVNEG: {
	output = SimplifyTerm(CreateTerm(BVPLUS,l,a0[0],one));
	break;
      }
      case BVMULT: {
	if(BVUMINUS == a0[0].GetKind()) {
	  output = CreateTerm(BVMULT,l,a0[0][0],a0[1]);
	}
	else if(BVUMINUS == a0[1].GetKind()) {
	  output = CreateTerm(BVMULT,l,a0[0],a0[1][0]);
	}
	else {
	  ASTNode a00 = SimplifyTerm(CreateTerm(BVUMINUS,l,a0[0]));	
	  output = CreateTerm(BVMULT,l,a00,a0[1]);
	}
	break;
      }
      case BVPLUS: {
	//push BVUMINUS over all the monomials of BVPLUS. Simplify
	//along the way
	//
	//BVUMINUS(a1x1 + a2x2 + ...) <=> BVPLUS(BVUMINUS(a1x1) +
	//BVUMINUS(a2x2) + ...
	ASTVec c = a0.GetChildren();
	ASTVec o;
	for(ASTVec::iterator it=c.begin(),itend=c.end();it!=itend;it++) {
	  //Simplify(BVUMINUS(a1x1))
	  ASTNode aaa = SimplifyTerm(CreateTerm(BVUMINUS,l,*it));
	  o.push_back(aaa);
	}
	//simplify the bvplus
	output = SimplifyTerm(CreateTerm(BVPLUS,l,o));	
	break;
      }
      case BVSUB: {
	//BVUMINUS(BVSUB(x,y)) <=> BVSUB(y,x)
	output = SimplifyTerm(CreateTerm(BVSUB,l,a0[1],a0[0]));
	break;
      }
      case ITE: {
	//BVUMINUS(ITE(c,t1,t2)) <==> ITE(c,BVUMINUS(t1),BVUMINUS(t2))
	ASTNode c = a0[0];
	ASTNode t1 = SimplifyTerm(CreateTerm(BVUMINUS,l,a0[1]));
	ASTNode t2 = SimplifyTerm(CreateTerm(BVUMINUS,l,a0[2]));
	output = CreateSimplifiedTermITE(c,t1,t2);
	break;
      }
      default: {
	output = CreateTerm(BVUMINUS,l,a0);
	break;
      }
      }
      break;
    }
    case BVEXTRACT:{
      //it is important to take care of wordlevel transformation in
      //BVEXTRACT. it exposes oppurtunities for later simplification
      //and solving (variable elimination)
      ASTNode a0 = SimplifyTerm(inputterm[0]);
      Kind k1 = a0.GetKind();
      unsigned int a_len = inputValueWidth;
      
      //indices for BVEXTRACT
      ASTNode i = inputterm[1];
      ASTNode j = inputterm[2];
      ASTNode zero = CreateBVConst(32,0);
      //recall that the indices of BVEXTRACT are always 32 bits
      //long. therefore doing a GetBVUnsigned is ok.
      unsigned int i_val = GetUnsignedConst(i);
      unsigned int j_val = GetUnsignedConst(j);

      // a0[i:0] and len(a0)=i+1, then return a0
      if(0 == j_val && a_len == a0.GetValueWidth())
	return a0;

      switch(k1) {
      case BVCONST: {
	//extract the constant
	output = BVConstEvaluator(CreateTerm(BVEXTRACT,a_len,a0,i,j));
	break;
      }
      case BVCONCAT:{
	//assumes concatenation is binary
	//
	//input is of the form a0[i:j]
	//
	//a0 is the conatentation t@u, and a0[0] is t, and a0[1] is u
	ASTNode t = a0[0];
	ASTNode u = a0[1];
	unsigned int len_a0 = a0.GetValueWidth();
	unsigned int len_u = u.GetValueWidth();

	if(len_u > i_val) {
	  //Apply the following rule:
	  // (t@u)[i:j] <==> u[i:j], if len(u) > i
	  //
	  output = SimplifyTerm(CreateTerm(BVEXTRACT,a_len,u,i,j));
	}
	else if(len_a0 > i_val && j_val >= len_u) {
	  //Apply the rule:
	  // (t@u)[i:j] <==> t[i-len_u:j-len_u], if len(t@u) > i >= j >= len(u)
	  i = CreateBVConst(32, i_val - len_u);
	  j = CreateBVConst(32, j_val - len_u);
	  output = SimplifyTerm(CreateTerm(BVEXTRACT,a_len,t,i,j));
	}
	else {
	  //Apply the rule:
	  // (t@u)[i:j] <==> t[i-len_u:0] @ u[len_u-1:j]
	  i = CreateBVConst(32,i_val-len_u);
	  ASTNode m = CreateBVConst(32, len_u-1);
	  t = SimplifyTerm(CreateTerm(BVEXTRACT,i_val-len_u+1,t,i,zero));
	  u = SimplifyTerm(CreateTerm(BVEXTRACT,len_u-j_val,u,m,j));
	  output = CreateTerm(BVCONCAT,a_len,t,u);
	}
	break;
      }
      case BVPLUS:
      case BVMULT: {	
	// (BVMULT(n,t,u))[i:j] <==> BVMULT(i+1,t[i:0],u[i:0])[i:j]
	//similar rule for BVPLUS
	ASTVec c = a0.GetChildren();
	ASTVec o;
	for(ASTVec::iterator jt=c.begin(),jtend=c.end();jt!=jtend;jt++) {
	  ASTNode aaa = *jt;
	  aaa = SimplifyTerm(CreateTerm(BVEXTRACT,i_val+1,aaa,i,zero));
	  o.push_back(aaa);
	}
	output = CreateTerm(a0.GetKind(),i_val+1,o);	
	if(j_val != 0) {
	  //add extraction only if j is not zero
	  output = CreateTerm(BVEXTRACT,a_len,output,i,j);
	}
	break;
      }
      case BVAND:
      case BVOR:
      case BVXOR: {
	//assumes these operators are binary
	//
	// (t op u)[i:j] <==> t[i:j] op u[i:j]
	ASTNode t = a0[0];
	ASTNode u = a0[1];
	t = SimplifyTerm(CreateTerm(BVEXTRACT,a_len,t,i,j));
	u = SimplifyTerm(CreateTerm(BVEXTRACT,a_len,u,i,j));
	BVTypeCheck(t);
	BVTypeCheck(u);
	output = CreateTerm(k1,a_len,t,u);
	break;
      }
      case BVNEG:{
	// (~t)[i:j] <==> ~(t[i:j])
	ASTNode t = a0[0];
	t = SimplifyTerm(CreateTerm(BVEXTRACT,a_len,t,i,j));
	output = CreateTerm(BVNEG,a_len,t);
	break;
      }
      // case BVSX:{
// 	//(BVSX(t,n)[i:j] <==> BVSX(t,i+1), if n >= i+1 and j=0 
// 	ASTNode t = a0[0];
// 	unsigned int bvsx_len = a0.GetValueWidth();
// 	if(bvsx_len < a_len) {
// 	  FatalError("SimplifyTerm: BVEXTRACT over BVSX:" 
// 		     "the length of BVSX term must be greater than extract-len",inputterm);
// 	}
// 	if(j != zero) {
// 	  output = CreateTerm(BVEXTRACT,a_len,a0,i,j);	  
// 	}
// 	else {
// 	  output = CreateTerm(BVSX,a_len,t,CreateBVConst(32,a_len));
// 	}
// 	break;
//       }
      case ITE: {
	ASTNode t0 = a0[0];
	ASTNode t1 = SimplifyTerm(CreateTerm(BVEXTRACT,a_len,a0[1],i,j));
	ASTNode t2 = SimplifyTerm(CreateTerm(BVEXTRACT,a_len,a0[2],i,j));
	output = CreateSimplifiedTermITE(t0,t1,t2);
	break;
      }
      default: {
	output = CreateTerm(BVEXTRACT,a_len,a0,i,j);
	break;
      }
      }
      break;
    }
    case BVNEG: {
      ASTNode a0 = SimplifyTerm(inputterm[0]);
      unsigned len = inputValueWidth;
      switch(a0.GetKind()) {
      case BVCONST:
	output = BVConstEvaluator(CreateTerm(BVNEG,len,a0));
	break;
      case BVNEG:
	output = a0[0];
	break;
      // case ITE: {
// 	ASTNode cond = a0[0];
// 	ASTNode thenpart = SimplifyTerm(CreateTerm(BVNEG,len,a0[1]));
// 	ASTNode elsepart = SimplifyTerm(CreateTerm(BVNEG,len,a0[2]));
// 	output = CreateSimplifiedTermITE(cond,thenpart,elsepart);	
// 	break;
//       }
      default:
	output = CreateTerm(BVNEG,len,a0);
	break;
      }
      break;
    }
    case BVSX:{
      //a0 is the expr which is being sign extended
      ASTNode a0 = SimplifyTerm(inputterm[0]);
      //a1 represents the length of the term BVSX(a0)
      ASTNode a1 = inputterm[1];
      //output length of the BVSX term
      unsigned len = inputValueWidth;
      
      if(a0.GetValueWidth() == len) {
	//nothing to signextend
	return a0;
      }

      switch(a0.GetKind()) {
      case BVCONST:
	output = BVConstEvaluator(CreateTerm(BVSX,len,a0,a1));
	break;
      case BVNEG:
	output = CreateTerm(a0.GetKind(),len,CreateTerm(BVSX,len,a0[0],a1));
	break;
      case BVAND:
      case BVOR:
	//assuming BVAND and BVOR are binary
	output = CreateTerm(a0.GetKind(),len,
			    CreateTerm(BVSX,len,a0[0],a1),
			    CreateTerm(BVSX,len,a0[1],a1));
	break;
      case BVPLUS: {	
	//BVSX(m,BVPLUS(n,BVSX(t1),BVSX(t2))) <==> BVPLUS(m,BVSX(m,t1),BVSX(m,t2))
	ASTVec c = a0.GetChildren();
	bool returnflag = false;
	for(ASTVec::iterator it=c.begin(),itend=c.end();it!=itend;it++) {
	  if(BVSX != it->GetKind()) {
	    returnflag = true;
	    break;
	  }
	}
	if(returnflag) {
	  output = CreateTerm(BVSX,len,a0,a1);
	}
	else {
	  ASTVec o;
	  for(ASTVec::iterator it=c.begin(),itend=c.end();it!=itend;it++) {
	    ASTNode aaa = SimplifyTerm(CreateTerm(BVSX,len,*it,a1));
	    o.push_back(aaa);
	  }
	  output = CreateTerm(a0.GetKind(),len,o);
	}
	break;
      }
      case BVSX: {
	//if you have BVSX(m,BVSX(n,a)) then you can drop the inner
	//BVSX provided m is greater than n.
	a0 = SimplifyTerm(a0[0]);
	output = CreateTerm(BVSX,len,a0,a1);
	break;
      }
      case ITE: {
	ASTNode cond = a0[0];
	ASTNode thenpart = SimplifyTerm(CreateTerm(BVSX,len,a0[1],a1));
	ASTNode elsepart = SimplifyTerm(CreateTerm(BVSX,len,a0[2],a1));
	output = CreateSimplifiedTermITE(cond,thenpart,elsepart);
	break;
      }
      default:
	output = CreateTerm(BVSX,len,a0,a1);
	break;
      }    
      break;
    }
    case BVAND:
    case BVOR:{
      ASTNode max = CreateMaxConst(inputValueWidth);
      ASTNode zero = CreateZeroConst(inputValueWidth);

      ASTNode identity = (BVAND == k) ? max : zero;
      ASTNode annihilator = (BVAND == k) ? zero : max;
      ASTVec c = FlattenOneLevel(inputterm).GetChildren();
      SortByExprNum(c);
      ASTVec o;
      bool constant = true;
      for(ASTVec::iterator it=c.begin(),itend=c.end();it!=itend;it++) {
	ASTNode aaa = SimplifyTerm(*it);
	if(BVCONST != aaa.GetKind()) {
	  constant = false;
	}

	if(aaa == annihilator) {
	  output = annihilator;
	  //memoize
	  UpdateSimplifyMap(inputterm,output,false);
	  //cerr << "output of SimplifyTerm: " << output << endl;
	  return output;
	}
	
	if(aaa != identity) {
	  o.push_back(aaa);
	}
      }
      
      switch(o.size()) {
      case 0:
	output = identity;
	break;
      case 1:
	output = o[0];
	break;
      default:
	SortByExprNum(o);
	output = CreateTerm(k,inputValueWidth,o);
	if(constant) {
	  output = BVConstEvaluator(output);
	}     
	break;
      }
      break;
    }
    case BVCONCAT:{
      ASTNode t = SimplifyTerm(inputterm[0]);
      ASTNode u = SimplifyTerm(inputterm[1]);
      Kind tkind = t.GetKind();
      Kind ukind = u.GetKind();
      
      
      if(BVCONST == tkind && BVCONST == ukind) {
	output = BVConstEvaluator(CreateTerm(BVCONCAT,inputValueWidth,t,u));
      }
      else if(BVEXTRACT == tkind && 
	      BVEXTRACT == ukind && 
	      t[0] == u[0]) {
	//to handle the case x[m:n]@x[n-1:k] <==> x[m:k]
	ASTNode t_hi = t[1];
	ASTNode t_low = t[2];
	ASTNode u_hi = u[1];
	ASTNode u_low = u[2];
	ASTNode c = BVConstEvaluator(CreateTerm(BVPLUS,32,u_hi,CreateOneConst(32)));
	if(t_low == c) {
	  output = CreateTerm(BVEXTRACT,inputValueWidth,t[0],t_hi,u_low);
	}
	else {
	  output = CreateTerm(BVCONCAT,inputValueWidth,t,u);
	}
      }
      else {
	output = CreateTerm(BVCONCAT,inputValueWidth,t,u);
      }
      break;
    }
    case BVXOR:
    case BVXNOR:
    case BVNAND:
    case BVNOR:
    case BVLEFTSHIFT:
    case BVRIGHTSHIFT:
    case BVVARSHIFT:
    case BVSRSHIFT:
    case BVDIV:
    case BVMOD: {
      ASTVec c = inputterm.GetChildren();
      ASTVec o;
      bool constant = true;
      for(ASTVec::iterator it=c.begin(),itend=c.end();it!=itend;it++) {
	ASTNode aaa = SimplifyTerm(*it);
	if(BVCONST != aaa.GetKind()) {
	  constant = false;
	}
	o.push_back(aaa);
      }
      output = CreateTerm(k,inputValueWidth,o);
      if(constant)
	output = BVConstEvaluator(output);
      break;
    }
    case READ: {
      ASTNode out1;
      //process only if not  in the substitution map. simplifymap
      //has been checked already
      if(!CheckSubstitutionMap(inputterm,out1)) {
	if(WRITE == inputterm[0].GetKind()) {
	  //get rid of all writes
	  ASTNode nowrites = RemoveWrites_TopLevel(inputterm);
	  out1 = nowrites;
	}
	else if (ITE == inputterm[0].GetKind()){
	  ASTNode cond = SimplifyFormula(inputterm[0][0],false);
	  ASTNode arr1 = SimplifyTerm(inputterm[0][1]);
	  ASTNode arr2 = SimplifyTerm(inputterm[0][2]);

	  ASTNode index = SimplifyTerm(inputterm[1]);
	  
	  ASTNode read1 = CreateTerm(READ,inputValueWidth,arr1,index);
	  ASTNode read2 = CreateTerm(READ,inputValueWidth,arr2,index);
	  out1 = CreateSimplifiedTermITE(cond,read1,read2);
	}
	else {
	  //arr is a SYMBOL for sure
	  ASTNode arr = inputterm[0];
	  ASTNode index = SimplifyTerm(inputterm[1]);
	  out1 = CreateTerm(READ,inputValueWidth,arr,index);     
	}
      }
      //it is possible that after all the procesing the READ term
      //reduces to READ(Symbol,const) and hence we should check the
      //substitutionmap once again.      
      if(!CheckSubstitutionMap(out1,output))
	output = out1;      
      break;
    }
    case ITE: {
      ASTNode t0 = SimplifyFormula(inputterm[0],false);
      ASTNode t1 = SimplifyTerm(inputterm[1]);
      ASTNode t2 = SimplifyTerm(inputterm[2]);
      output = CreateSimplifiedTermITE(t0,t1,t2);      
      break;
    }
    case SBVMOD:
    case SBVDIV: {
      ASTVec c = inputterm.GetChildren();
      ASTVec o;
      for(ASTVec::iterator it=c.begin(),itend=c.end();it!=itend;it++) {
	ASTNode aaa = SimplifyTerm(*it);
	o.push_back(aaa);
      }
      output = CreateTerm(k,inputValueWidth,o);
      break;
    }
    case WRITE:     
    default:
      FatalError("SimplifyTerm: Control should never reach here:", inputterm, k);
      return inputterm;
      break;
    }

    //memoize
    UpdateSimplifyMap(inputterm,output,false);
    //cerr << "SimplifyTerm: output" << output << endl;
    return output;
  } //end of SimplifyTerm()


  //At the end of each simplification call, we want the output to be
  //always smaller or equal to the input in size.
  void BeevMgr::CheckSimplifyInvariant(const ASTNode& a, const ASTNode& output) {
    //Don't do the check in optimized mode
    if(optimize)
      return;

    if(NodeSize(a,true) < NodeSize(output,true)) {
      cerr << "lhs := " << a << endl;
      cerr << "NodeSize of lhs is: " << NodeSize(a, true) << endl;
      cerr << endl;
      cerr << "rhs := " << output << endl;
      cerr << "NodeSize of rhs is: " << NodeSize(output, true) << endl;
      FatalError("SimplifyFormula: The nodesize shoudl decrease from lhs to rhs: ",ASTUndefined);
    }
  }

  //this function assumes that the input is a vector of childnodes of
  //a BVPLUS term. it combines like terms and returns a bvplus
  //term. e.g. 1.x + 2.x is converted to 3.x
  ASTNode BeevMgr::CombineLikeTerms(const ASTNode& a) {
    if(BVPLUS != a.GetKind())
      return a;
    
    ASTNode output;
    if(CheckSimplifyMap(a,output,false)) {
      //check memo table
      //cerr << "output of SimplifyTerm Cache: " << output << endl;
      return output;
    }
    
    ASTVec c = a.GetChildren();
    //map from variables to vector of constants
    ASTNodeToVecMap vars_to_consts;
    //vector to hold constants
    ASTVec constkids;
    ASTVec outputvec;

    //useful constants
    unsigned int len = c[0].GetValueWidth();
    ASTNode one = CreateOneConst(len);
    ASTNode zero = CreateZeroConst(len);
    ASTNode max = CreateMaxConst(len);

    //go over the childnodes of the input bvplus, and collect like
    //terms in a map. the key of the map are the variables, and the
    //values are stored in a ASTVec
    for(ASTVec::const_iterator it=c.begin(),itend=c.end();it!=itend;it++){
      ASTNode aaa = *it;
      if(SYMBOL == aaa.GetKind()) {
	vars_to_consts[aaa].push_back(one);
      }
      else if(BVMULT == aaa.GetKind() && 
	      BVUMINUS == aaa[0].GetKind() &&
	      BVCONST == aaa[0][0].GetKind()) {
	//(BVUMINUS(c))*(y) <==> compute(BVUMINUS(c))*y
	ASTNode compute_const = BVConstEvaluator(aaa[0]);
	vars_to_consts[aaa[1]].push_back(compute_const);
      }
      else if(BVMULT == aaa.GetKind() && 
	      BVUMINUS == aaa[1].GetKind() &&
	      BVCONST == aaa[0].GetKind()) {
	//c*(BVUMINUS(y)) <==> compute(BVUMINUS(c))*y
	ASTNode cccc = BVConstEvaluator(CreateTerm(BVUMINUS,len,aaa[0]));
	vars_to_consts[aaa[1][0]].push_back(cccc);      
      }      
      else if(BVMULT == aaa.GetKind() && BVCONST == aaa[0].GetKind()) {
	//assumes that BVMULT is binary
	vars_to_consts[aaa[1]].push_back(aaa[0]);
      } 
      else if(BVMULT == aaa.GetKind() && BVUMINUS == aaa[0].GetKind()) {
	//(-1*x)*(y) <==> -1*(xy)
	ASTNode cccc = CreateTerm(BVMULT,len,aaa[0][0],aaa[1]);
	ASTVec cNodes = cccc.GetChildren();
	SortByExprNum(cNodes);
	vars_to_consts[cccc].push_back(max);
      }
      else if(BVMULT == aaa.GetKind() && BVUMINUS == aaa[1].GetKind()) {
	//x*(-1*y) <==> -1*(xy)
	ASTNode cccc = CreateTerm(BVMULT,len,aaa[0],aaa[1][0]);
	ASTVec cNodes = cccc.GetChildren();
	SortByExprNum(cNodes);
	vars_to_consts[cccc].push_back(max);      
      }
      else if(BVCONST == aaa.GetKind()) {
	constkids.push_back(aaa);
      }
      else if(BVUMINUS == aaa.GetKind()) {
	//helps to convert BVUMINUS into a BVMULT. here the max
	//constant represents -1. this transformation allows us to
	//conclude that x + BVUMINUS(x) is 0.
	vars_to_consts[aaa[0]].push_back(max);
      }
      else 
	vars_to_consts[aaa].push_back(one);
    } //end of for loop

    //go over the map from variables to vector of values. combine the
    //vector of values, multiply to the variable, and put the
    //resultant monomial in the output BVPLUS.
    for(ASTNodeToVecMap::iterator it=vars_to_consts.begin(),itend=vars_to_consts.end();
	it!=itend;it++){
      ASTVec ccc = it->second;
      
      ASTNode constant;
      if(1 < ccc.size()) {
	constant = CreateTerm(BVPLUS,ccc[0].GetValueWidth(),ccc);
	constant = BVConstEvaluator(constant);
      }
      else
	constant = ccc[0];
      
      //constant * var
      ASTNode monom;
      if(zero == constant) 
	monom = zero;
      else if (one == constant)
	monom = it->first;
      else
	monom = 
	  SimplifyTerm(CreateTerm(BVMULT,constant.GetValueWidth(),constant,it->first));
      if(zero != monom) {
	outputvec.push_back(monom);
      }
    } //end of for loop

    if(constkids.size() > 1) {
      ASTNode output = CreateTerm(BVPLUS,constkids[0].GetValueWidth(),constkids);
      output = BVConstEvaluator(output);
      if(output != zero)
	outputvec.push_back(output);
    }
    else if (constkids.size() == 1) {
      if(constkids[0] != zero)
	outputvec.push_back(constkids[0]);
    }

    if (outputvec.size() > 1) {
      output = CreateTerm(BVPLUS,len,outputvec);
    }
    else if(outputvec.size() == 1) {
      output = outputvec[0];
    }
    else {
      output = zero;
    }

    //memoize
    //UpdateSimplifyMap(a,output,false);
    return output;
  } //end of CombineLikeTerms()

  //accepts lhs and rhs, and returns lhs - rhs = 0. The function
  //assumes that lhs and rhs have already been simplified. although
  //this assumption is not needed for correctness, it is essential for
  //performance. The function also assumes that lhs is a BVPLUS
  ASTNode BeevMgr::LhsMinusRhs(const ASTNode& eq) {
    //if input is not an equality, simply return it
    if(EQ != eq.GetKind())
      return eq;

    ASTNode lhs = eq[0];
    ASTNode rhs = eq[1];
    Kind k_lhs = lhs.GetKind();
    Kind k_rhs = rhs.GetKind();
    //either the lhs has to be a BVPLUS or the rhs has to be a
    //BVPLUS
    if(!(BVPLUS == k_lhs || 
	 BVPLUS == k_rhs ||
	 (BVMULT == k_lhs && 
	  BVMULT == k_rhs)
	 )) {
      return eq;
    }

    ASTNode output;
    if(CheckSimplifyMap(eq,output,false)) {
      //check memo table
      //cerr << "output of SimplifyTerm Cache: " << output << endl;
      return output;
    }
    
    //if the lhs is not a BVPLUS, but the rhs is a BVPLUS, then swap
    //the lhs and rhs
    bool swap_flag = false;
    if(BVPLUS != k_lhs && BVPLUS == k_rhs) {
      ASTNode swap = lhs;
      lhs = rhs;
      rhs = swap;
      swap_flag = true;
    }

    unsigned int len = lhs.GetValueWidth();
    ASTNode zero = CreateZeroConst(len);
    //right is -1*(rhs): Simplify(-1*rhs)
    rhs = SimplifyTerm(CreateTerm(BVUMINUS,len,rhs));

    ASTVec lvec = lhs.GetChildren();
    ASTVec rvec = rhs.GetChildren();
    ASTNode lhsplusrhs;
    if(BVPLUS != lhs.GetKind() && BVPLUS != rhs.GetKind()) {
      lhsplusrhs = CreateTerm(BVPLUS,len,lhs,rhs); 
    }
    else if(BVPLUS == lhs.GetKind() && BVPLUS == rhs.GetKind()) {
      //combine the childnodes of the left and the right
      lvec.insert(lvec.end(),rvec.begin(),rvec.end());
      lhsplusrhs = CreateTerm(BVPLUS,len,lvec);
    }
    else if(BVPLUS == lhs.GetKind() && BVPLUS != rhs.GetKind()){
      lvec.push_back(rhs);
      lhsplusrhs = CreateTerm(BVPLUS,len,lvec);
    }
    else {
      //Control should never reach here
      FatalError("LhsMinusRhs: Control should never reach here\n");
    }

    //combine like terms
    output = CombineLikeTerms(lhsplusrhs);
    output = SimplifyTerm(output);
    //
    //Now make output into: lhs-rhs = 0
    output = CreateSimplifiedEQ(output,zero);
    //sort if BVPLUS
    if(BVPLUS == output.GetKind()) {
      ASTVec outv = output.GetChildren();
      SortByExprNum(outv);
      output = CreateTerm(BVPLUS,len,outv);
    }
    
    //memoize
    //UpdateSimplifyMap(eq,output,false);
    return output;  
  } //end of LhsMinusRHS()

  //THis function accepts a BVMULT(t1,t2) and distributes the mult
  //over plus if either or both t1 and t2 are BVPLUSes.
  //
  // x*(y1 + y2 + ...+ yn) <==> x*y1 + x*y2 + ... + x*yn
  //
  // (y1 + y2 + ...+ yn)*x <==> x*y1 + x*y2 + ... + x*yn
  //
  // The function assumes that the BVPLUSes have been flattened
  ASTNode BeevMgr::DistributeMultOverPlus(const ASTNode& a, bool startdistribution) {
    if(!startdistribution)
      return a;
    Kind k = a.GetKind();
    if(BVMULT != k)
      return a;

    ASTNode left = a[0];
    ASTNode right = a[1];
    Kind left_kind = left.GetKind();
    Kind right_kind = right.GetKind();

    ASTNode output;
    if(CheckSimplifyMap(a,output,false)) {
      //check memo table
      //cerr << "output of SimplifyTerm Cache: " << output << endl;
      return output;
    }

    //special case optimization: c1*(c2*t1) <==> (c1*c2)*t1
    if(BVCONST == left_kind && 
       BVMULT == right_kind && 
       BVCONST == right[0].GetKind()) {
      ASTNode c = BVConstEvaluator(CreateTerm(BVMULT,a.GetValueWidth(),left,right[0]));
      c = CreateTerm(BVMULT,a.GetValueWidth(),c,right[1]);
      return c;
      left = c[0];
      right = c[1];
      left_kind = left.GetKind();
      right_kind = right.GetKind();    
    }

    //special case optimization: c1*(t1*c2) <==> (c1*c2)*t1
    if(BVCONST == left_kind && 
       BVMULT == right_kind && 
       BVCONST == right[1].GetKind()) {
      ASTNode c = BVConstEvaluator(CreateTerm(BVMULT,a.GetValueWidth(),left,right[1]));
      c = CreateTerm(BVMULT,a.GetValueWidth(),c,right[0]);
      return c;
      left = c[0];
      right = c[1];
      left_kind = left.GetKind();
      right_kind = right.GetKind();    
    }

    //atleast one of left or right have to be BVPLUS
    if(!(BVPLUS == left_kind || BVPLUS == right_kind)) {
      return a;
    }
    
    //if left is BVPLUS and right is not, then swap left and right. we
    //can do this since BVMULT is communtative
    ASTNode swap;
    if(BVPLUS == left_kind && BVPLUS != right_kind) {
      swap = left;
      left = right;
      right = swap;
    }
    left_kind = left.GetKind();
    right_kind = right.GetKind();

    //by this point we are gauranteed that right is a BVPLUS, but left
    //may not be
    ASTVec rightnodes = right.GetChildren();
    ASTVec outputvec;
    unsigned len = a.GetValueWidth();
    ASTNode zero = CreateZeroConst(len);
    ASTNode one = CreateOneConst(len);
    if(BVPLUS != left_kind) {
      //if the multiplier is not a BVPLUS then we have a special case
      // x*(y1 + y2 + ...+ yn) <==> x*y1 + x*y2 + ... + x*yn
      if(zero == left) {
	outputvec.push_back(zero);
      }
      else if(one == left) {
	outputvec.push_back(left);
      }
      else {
	for(ASTVec::iterator j=rightnodes.begin(),jend=rightnodes.end();
	    j!=jend;j++) {
	  ASTNode out = SimplifyTerm(CreateTerm(BVMULT,len,left,*j));
	  outputvec.push_back(out);
	}
      }
    }
    else {
      ASTVec leftnodes = left.GetChildren();
      // (x1 + x2 + ... + xm)*(y1 + y2 + ...+ yn) <==> x1*y1 + x1*y2 +
      // ... + x2*y1 + ... + xm*yn
      for(ASTVec::iterator i=leftnodes.begin(),iend=leftnodes.end();
	  i!=iend;i++) {
	ASTNode multiplier = *i;
	for(ASTVec::iterator j=rightnodes.begin(),jend=rightnodes.end();
	    j!=jend;j++) {
	  ASTNode out = SimplifyTerm(CreateTerm(BVMULT,len,multiplier,*j));
	  outputvec.push_back(out);
	}
      }
    }
    
    //compute output here
    if(outputvec.size() > 1) {
      output = CombineLikeTerms(CreateTerm(BVPLUS,len,outputvec));
      output = SimplifyTerm(output);
    }
    else
      output = SimplifyTerm(outputvec[0]);

    //memoize
    //UpdateSimplifyMap(a,output,false);
    return output;
  } //end of distributemultoverplus()

  //converts the BVSX(len, a0) operator into ITE( check top bit,
  //extend a0 by 1, extend a0 by 0)
  ASTNode BeevMgr::ConvertBVSXToITE(const ASTNode& a) {
    if(BVSX != a.GetKind())
      return a;

    ASTNode output;
    if(CheckSimplifyMap(a,output,false)) {
      //check memo table
      //cerr << "output of ConvertBVSXToITE Cache: " << output << endl;
      return output;
    }
    
    ASTNode a0 = a[0];
    unsigned a_len = a.GetValueWidth();
    unsigned a0_len = a0.GetValueWidth();
    
    if(a0_len > a_len){
      FatalError("Trying to sign_extend a larger BV into a smaller BV");
      return ASTUndefined; //to stop the compiler from producing bogus warnings
    }
    
    //sign extend
    unsigned extensionlen = a_len-a0_len;
    if(0 == extensionlen) {
      UpdateSimplifyMap(a,output,false);
      return a;
    }

    std::string ones;
    for(unsigned c=0; c < extensionlen;c++)
      ones += '1';			   
    std::string zeros;
    for(unsigned c=0; c < extensionlen;c++)
      zeros += '0';
			   
    //string of oness of length extensionlen
    BEEV::ASTNode BVOnes = CreateBVConst(ones.c_str(),2);
    //string of zeros of length extensionlen
    BEEV::ASTNode BVZeros = CreateBVConst(zeros.c_str(),2);
			   
    //string of ones BVCONCAT a0
    BEEV::ASTNode concatOnes = CreateTerm(BEEV::BVCONCAT,a_len,BVOnes,a0);
    //string of zeros BVCONCAT a0
    BEEV::ASTNode concatZeros = CreateTerm(BEEV::BVCONCAT,a_len,BVZeros,a0);

    //extract top bit of a0
    BEEV::ASTNode hi = CreateBVConst(32,a0_len-1);
    BEEV::ASTNode low = CreateBVConst(32,a0_len-1);
    BEEV::ASTNode topBit = CreateTerm(BEEV::BVEXTRACT,1,a0,hi,low);

    //compare topBit of a0 with 0bin1
    BEEV::ASTNode condition = CreateSimplifiedEQ(CreateBVConst(1,1),topBit);

    //ITE(topbit = 0bin1, 0bin1111...a0, 0bin000...a0)
    output = CreateSimplifiedTermITE(condition,concatOnes,concatZeros);
    UpdateSimplifyMap(a,output,false);
    return output;
  } //end of ConvertBVSXToITE()


  ASTNode BeevMgr::RemoveWrites_TopLevel(const ASTNode& term) {
    if(READ != term.GetKind() && WRITE != term[0].GetKind()) {
      FatalError("RemovesWrites: Input must be a READ over a WRITE",term);
    }
    
    if(!Begin_RemoveWrites && 
       !SimplifyWrites_InPlace_Flag && 
       !start_abstracting) {
      return term;
    }
    else if(!Begin_RemoveWrites && 
	    SimplifyWrites_InPlace_Flag && 
	    !start_abstracting) {
      //return term;
      return SimplifyWrites_InPlace(term);
    }
    else {
      return RemoveWrites(term);
    }
  } //end of RemoveWrites_TopLevel()

  ASTNode BeevMgr::SimplifyWrites_InPlace(const ASTNode& term) {
    ASTNodeMultiSet WriteIndicesSeenSoFar;
    bool SeenNonConstWriteIndex = false;

    if(READ != term.GetKind() && 
	WRITE != term[0].GetKind()) {
      FatalError("RemovesWrites: Input must be a READ over a WRITE",term);
    }
    
    ASTNode output;
    if(CheckSimplifyMap(term,output,false)) {
      return output;
    }

    ASTVec writeIndices, writeValues;
    unsigned int width = term.GetValueWidth();
    ASTNode write = term[0];
    unsigned indexwidth = write.GetIndexWidth();
    ASTNode readIndex = SimplifyTerm(term[1]);
        
    do {
      ASTNode writeIndex = SimplifyTerm(write[1]);
      ASTNode writeVal = SimplifyTerm(write[2]);
          
      //compare the readIndex and the current writeIndex and see if they
      //simplify to TRUE or FALSE or UNDETERMINABLE at this stage
      ASTNode compare_readwrite_indices = 
	SimplifyFormula(CreateSimplifiedEQ(writeIndex,readIndex),false);
    
      //if readIndex and writeIndex are equal
      if(ASTTrue == compare_readwrite_indices && !SeenNonConstWriteIndex) {
	UpdateSimplifyMap(term,writeVal,false);
	return writeVal;
      }

      if(!(ASTTrue == compare_readwrite_indices || 
	   ASTFalse == compare_readwrite_indices)) {
	SeenNonConstWriteIndex = true;
      }

      //if (readIndex=writeIndex <=> FALSE)
      if(ASTFalse == compare_readwrite_indices 
	 ||
	 (WriteIndicesSeenSoFar.find(writeIndex) != WriteIndicesSeenSoFar.end())
	 ) {
	//drop the current level write
	//do nothing
      }
      else {
	writeIndices.push_back(writeIndex);
	writeValues.push_back(writeVal);
      }
      
      //record the write indices seen so far
      //if(BVCONST == writeIndex.GetKind()) {
	WriteIndicesSeenSoFar.insert(writeIndex);
	//}

      //Setup the write for the new iteration, one level inner write
      write = write[0];
    }while (SYMBOL != write.GetKind());

    ASTVec::reverse_iterator it_index = writeIndices.rbegin();
    ASTVec::reverse_iterator itend_index = writeIndices.rend();
    ASTVec::reverse_iterator it_values = writeValues.rbegin();
    ASTVec::reverse_iterator itend_values = writeValues.rend();

    //"write" must be a symbol at the control point before the
    //begining of the "for loop"

    for(;it_index!=itend_index;it_index++,it_values++) {
      write = CreateTerm(WRITE,width,write,*it_index,*it_values);
      write.SetIndexWidth(indexwidth);
    }

    output = CreateTerm(READ,width,write,readIndex);
    UpdateSimplifyMap(term,output,false);
    return output;
  } //end of SimplifyWrites_In_Place() 

  //accepts a read over a write and returns a term without the write
  //READ(WRITE(A i val) j) <==> ITE(i=j,val,READ(A,j)). We use a memo
  //table for this function called RemoveWritesMemoMap
  ASTNode BeevMgr::RemoveWrites(const ASTNode& input) {   
    //unsigned int width = input.GetValueWidth();
    if(READ != input.GetKind() || WRITE != input[0].GetKind()) {
      FatalError("RemovesWrites: Input must be a READ over a WRITE",input);
    }

    ASTNodeMap::iterator it;
    ASTNode output = input;
    if(CheckSimplifyMap(input,output,false)) {
      return output;
    }
        
    if(!start_abstracting && Begin_RemoveWrites) {
      output= ReadOverWrite_To_ITE(input);
    }

    if(start_abstracting) {
      ASTNode newVar;
      if(!CheckSimplifyMap(input,newVar,false)) {
	newVar = NewVar(input.GetValueWidth());
	ReadOverWrite_NewName_Map[input] = newVar;
	NewName_ReadOverWrite_Map[newVar] = input;

	UpdateSimplifyMap(input,newVar,false);
	ASTNodeStats("New Var Name which replace Read_Over_Write: ", newVar);
      }
      output = newVar;
    } //end of start_abstracting if condition

    //memoize
    UpdateSimplifyMap(input,output,false);
    return output;
  } //end of RemoveWrites()

  ASTNode BeevMgr::ReadOverWrite_To_ITE(const ASTNode& term) {
    unsigned int width = term.GetValueWidth();
    ASTNode input = term;
    if(READ != term.GetKind() || WRITE != term[0].GetKind()) {
      FatalError("RemovesWrites: Input must be a READ over a WRITE",term);
    }

    ASTNodeMap::iterator it;
    ASTNode output;
    // if(CheckSimplifyMap(term,output,false)) {
    //       return output;
    //     }
    
    ASTNode partialITE = term;
    ASTNode writeA = ASTTrue;
    ASTNode oldRead = term;
    //iteratively expand read-over-write
    do {
      ASTNode write = input[0];
      ASTNode readIndex = SimplifyTerm(input[1]);
      //DO NOT CALL SimplifyTerm() on write[0]. You will go into an
      //infinite loop
      writeA = write[0];
      ASTNode writeIndex = SimplifyTerm(write[1]);
      ASTNode writeVal = SimplifyTerm(write[2]);
      
      ASTNode cond = SimplifyFormula(CreateSimplifiedEQ(writeIndex,readIndex),false);
      ASTNode newRead = CreateTerm(READ,width,writeA,readIndex);
      ASTNode newRead_memoized = newRead;
      if(CheckSimplifyMap(newRead, newRead_memoized,false)) {
	newRead = newRead_memoized;
      }
      
      if(ASTTrue == cond && (term == partialITE)) {
	//found the write-value in the first iteration itself. return
	//it
	output = writeVal;
	UpdateSimplifyMap(term,output,false);
	return output;
      }
      
      if(READ == partialITE.GetKind() && WRITE == partialITE[0].GetKind()) {
	//first iteration or (previous cond==ASTFALSE and partialITE is a "READ over WRITE")
	partialITE = CreateSimplifiedTermITE(cond, writeVal, newRead);
      }
      else if (ITE == partialITE.GetKind()){
	//ITE(i1 = j, v1, R(A,j))
	ASTNode ElseITE = CreateSimplifiedTermITE(cond, writeVal, newRead);
	//R(W(A,i1,v1),j) <==> ITE(i1 = j, v1, R(A,j))
	UpdateSimplifyMap(oldRead,ElseITE,false);
	//ITE(i2 = j, v2, R(W(A,i1,v1),j)) <==> ITE(i2 = j, v2, ITE(i1 = j, v1, R(A,j)))
	partialITE = SimplifyTerm(partialITE);
      }
      else {
	FatalError("RemoveWrites: Control should not reach here\n");
      }
      
      if(ASTTrue == cond) {
	//no more iterations required
	output = partialITE;
	UpdateSimplifyMap(term,output,false);
	return output;
      }
      
      input = newRead;
      oldRead = newRead;
    } while(READ == input.GetKind() && WRITE == input[0].GetKind());
    
    output = partialITE;
    
    //memoize
    //UpdateSimplifyMap(term,output,false);
    return output;
  } //ReadOverWrite_To_ITE()

  //compute the multiplicative inverse of the input
  ASTNode BeevMgr::MultiplicativeInverse(const ASTNode& d) {
    ASTNode c = d;
    if(BVCONST != c.GetKind()) {
      FatalError("Input must be a constant", c);
    }

    if(!BVConstIsOdd(c)) {
      FatalError("MultiplicativeInverse: Input must be odd: ",c);
    }
    
    //cerr << "input to multinverse function is: " << d << endl;
    ASTNode inverse;
    if(CheckMultInverseMap(d,inverse)) {
      //cerr << "found the inverse of: " << d << "and it is: " << inverse << endl;
      return inverse;
    }

    inverse = c;
    unsigned inputwidth = c.GetValueWidth();

#ifdef NATIVE_C_ARITH
    ASTNode one = CreateOneConst(inputwidth);
    while(c != one) {
      //c = c*c
      c = BVConstEvaluator(CreateTerm(BVMULT,inputwidth,c,c));
      //inverse = invsere*c
      inverse = BVConstEvaluator(CreateTerm(BVMULT,inputwidth,inverse,c));
    }
#else
    //Compute the multiplicative inverse of c using the extended
    //euclidian algorithm
    //
    //create a '0' which is 1 bit long
    ASTNode onebit_zero = CreateZeroConst(1);
    //zero pad t0, i.e. 0 @ t0
    c = BVConstEvaluator(CreateTerm(BVCONCAT,inputwidth+1,onebit_zero,c));

    //construct 2^(inputwidth), i.e. a bitvector of length
    //'inputwidth+1', which is max(inputwidth)+1
    //
    //all 1's 
    ASTNode max = CreateMaxConst(inputwidth);
    //zero pad max
    max = BVConstEvaluator(CreateTerm(BVCONCAT,inputwidth+1,onebit_zero,max));
    //Create a '1' which has leading zeros of length 'inputwidth'
    ASTNode inputwidthplusone_one = CreateOneConst(inputwidth+1);    
    //add 1 to max
    max = CreateTerm(BVPLUS,inputwidth+1,max,inputwidthplusone_one);
    max = BVConstEvaluator(max);
    
    ASTNode zero = CreateZeroConst(inputwidth+1);
    ASTNode max_bvgt_0 = CreateNode(BVGT,max,zero);
    ASTNode quotient, remainder;
    ASTNode x, x1, x2;

    //x1 initialized to zero
    x1 = zero;
    //x2 initialized to one
    x2 = CreateOneConst(inputwidth+1);
    while (ASTTrue == BVConstEvaluator(max_bvgt_0)) {
      //quotient = (c divided by max)
      quotient = BVConstEvaluator(CreateTerm(BVDIV,inputwidth+1, c, max));

      //remainder of (c divided by max)
      remainder = BVConstEvaluator(CreateTerm(BVMOD,inputwidth+1, c, max));

      //x = x2 - q*x1
      x = CreateTerm(BVSUB,inputwidth+1,x2,CreateTerm(BVMULT,inputwidth+1,quotient,x1));
      x = BVConstEvaluator(x);

      //fix the inputs to the extended euclidian algo
      c = max;
      max = remainder;
      max_bvgt_0 = CreateNode(BVGT,max,zero);
      
      x2 = x1;
      x1 = x;
    }
    
    ASTNode hi = CreateBVConst(32,inputwidth-1);
    ASTNode low = CreateZeroConst(32);
    inverse = CreateTerm(BVEXTRACT,inputwidth,x2,hi,low);
    inverse = BVConstEvaluator(inverse);
#endif

    UpdateMultInverseMap(d,inverse);
    //cerr << "output of multinverse function is: " << inverse << endl;
    return inverse;
  } //end of MultiplicativeInverse()

  //returns true if the input is odd
  bool BeevMgr::BVConstIsOdd(const ASTNode& c) {
    if(BVCONST != c.GetKind()) {
      FatalError("Input must be a constant", c);
    }
   
    ASTNode zero = CreateZeroConst(1);
    ASTNode hi = CreateZeroConst(32);
    ASTNode low = hi;
    ASTNode lowestbit = CreateTerm(BVEXTRACT,1,c,hi,low);
    lowestbit =  BVConstEvaluator(lowestbit);

    if(lowestbit == zero) {
      return false;
    }
    else {
      return true;
    }
  } //end of BVConstIsOdd()

  //The big substitution function
  ASTNode BeevMgr::CreateSubstitutionMap(const ASTNode& a){
    if(!optimize)
      return a;

    ASTNode output = a;
    //if the variable has been solved for, then simply return it
    if(CheckSolverMap(a,output))
      return output;

    //traverse a and populate the SubstitutionMap 
    Kind k = a.GetKind();
    if(SYMBOL == k && BOOLEAN_TYPE == a.GetType()) {
      bool updated = UpdateSubstitutionMap(a,ASTTrue);
      output = updated ? ASTTrue : a;      
      return output;          
    }
    if(NOT == k
       && SYMBOL == a[0].GetKind()) {
      bool updated = UpdateSubstitutionMap(a[0],ASTFalse);
      output = updated ? ASTTrue : a;      
      return output;          
    }
    
    if(IFF == k) {
      ASTVec c = a.GetChildren();
      SortByExprNum(c);
      if(SYMBOL != c[0].GetKind() || 
    	 VarSeenInTerm(c[0],SimplifyFormula_NoRemoveWrites(c[1],false))) {
	return a;
      }
      bool updated = UpdateSubstitutionMap(c[0],c[1]);
      output = updated ? ASTTrue : a;      
      return output;      
    }
    
    if(EQ == k) {
      //fill the arrayname readindices vector if e0 is a
      //READ(Arr,index) and index is a BVCONST
      ASTVec c = a.GetChildren();
      SortByExprNum(c);
      FillUp_ArrReadIndex_Vec(c[0],c[1]);

      if(SYMBOL == c[0].GetKind() && 
	 VarSeenInTerm(c[0],SimplifyTerm(c[1]))) {
	return a;
      }

      if(1 == TermOrder(c[0],c[1]) &&
	 READ == c[0].GetKind() &&
	 VarSeenInTerm(c[0][0],SimplifyTerm(c[1]))) {
	return a;
      }
      bool updated = UpdateSubstitutionMap(c[0],c[1]);      
      output = updated ? ASTTrue : a;      
      return output;      
    }

    if(AND == k){
      ASTVec o;
      ASTVec c = a.GetChildren();
      for(ASTVec::iterator it = c.begin(),itend=c.end();it!=itend;it++) {
	UpdateAlwaysTrueFormMap(*it);
	ASTNode aaa = CreateSubstitutionMap(*it);
	
	if(ASTTrue != aaa) {
	  if(ASTFalse == aaa)
	    return ASTFalse;
	  else
	    o.push_back(aaa);
	}
      }
      if(o.size() == 0)
	return ASTTrue;

      if(o.size() == 1)
	return o[0];
      
      return CreateNode(AND,o);
    }
    return output;
  } //end of CreateSubstitutionMap()


  bool BeevMgr::VarSeenInTerm(const ASTNode& var, const ASTNode& term) {
    if(READ == term.GetKind() && 
       WRITE == term[0].GetKind() && !Begin_RemoveWrites) {
      return false;
    }

    if(READ == term.GetKind() && 
       WRITE == term[0].GetKind() && Begin_RemoveWrites) {
      return true;
    }

    ASTNodeMap::iterator it;    
    if((it = TermsAlreadySeenMap.find(term)) != TermsAlreadySeenMap.end()) {
      if(it->second == var) {
	return false;
      }
    }

    if(var == term) {
      return true;
    }

    for(ASTVec::const_iterator it=term.begin(),itend=term.end();it!=itend;it++){
      if(VarSeenInTerm(var,*it)) {
	return true;
      }
      else {
	TermsAlreadySeenMap[*it] = var;
      }
    }

    TermsAlreadySeenMap[term] = var;
    return false;
  }
};//end of namespace
