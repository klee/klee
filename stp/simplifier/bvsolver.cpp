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
#include "bvsolver.h"

  //This file contains the implementation of member functions of
  //bvsolver class, which represents the bitvector arithmetic linear
  //solver. Please also refer the STP's CAV 2007 paper for the
  //complete description of the linear solver algorithm
  //
  //The bitvector solver is a partial solver, i.e. it does not solve
  //for all variables in the system of equations. it is
  //best-effort. it relies on the SAT solver to be complete.
  //
  //The BVSolver assumes that the input equations are normalized, and
  //have liketerms combined etc.
  //
  //0. Traverse top-down over the input DAG, looking for a conjunction
  //0. of equations. if you find one, then for each equation in the
  //0. conjunction, do the following steps.
  //
  //1. check for Linearity of the input equation
  //
  //2. Solve for a "chosen" variable. The variable should occur
  //2. exactly once and must have an odd coeff. Refer STP's CAV 2007
  //2. paper for actual solving procedure
  //
  //4. Outside the solver, Substitute and Re-normalize the input DAG 
namespace BEEV {  
  //check the solver map for 'key'. If key is present, then return the
  //value by reference in the argument 'output'
  bool BVSolver::CheckAlreadySolvedMap(const ASTNode& key, ASTNode& output) {
    ASTNodeMap::iterator it;
    if((it = FormulasAlreadySolvedMap.find(key)) != FormulasAlreadySolvedMap.end()) {
      output = it->second;
      return true;
    }
    return false;
  } //CheckAlreadySolvedMap()

  void BVSolver::UpdateAlreadySolvedMap(const ASTNode& key, const ASTNode& value) {
    FormulasAlreadySolvedMap[key] = value;
  } //end of UpdateAlreadySolvedMap()

  //FIXME This is doing way more arithmetic than it needs to.
  //accepts an even number "in", and splits it into an odd number and
  //a power of 2. i.e " in = b.(2^k) ". returns the odd number, and
  //the power of two by reference
  ASTNode BVSolver::SplitEven_into_Oddnum_PowerOf2(const ASTNode& in, 
						unsigned int& number_shifts) {
    if(BVCONST != in.GetKind() || _bm->BVConstIsOdd(in)) {
      FatalError("BVSolver:SplitNum_Odd_PowerOf2: input must be a BVCONST and even\n",in);
    }
    
    unsigned int len = in.GetValueWidth();
    ASTNode zero = _bm->CreateZeroConst(len);
    ASTNode two = _bm->CreateTwoConst(len);
    ASTNode div_by_2 = in;
    ASTNode mod_by_2 = 
      _bm->BVConstEvaluator(_bm->CreateTerm(BVMOD,len,div_by_2,two)); 
    while(mod_by_2 == zero) {
      div_by_2 = 
	_bm->BVConstEvaluator(_bm->CreateTerm(BVDIV,len,div_by_2,two));
      number_shifts++;
      mod_by_2 = 
	_bm->BVConstEvaluator(_bm->CreateTerm(BVMOD,len,div_by_2,two));
    }
    return div_by_2;
  } //end of SplitEven_into_Oddnum_PowerOf2()

  //Checks if there are any ARRAYREADS in the term, after the
  //alreadyseenmap is cleared, i.e. traversing a new term altogether
  bool BVSolver::CheckForArrayReads_TopLevel(const ASTNode& term) {
    TermsAlreadySeenMap.clear();
    return CheckForArrayReads(term);
  }
  
  //Checks if there are any ARRAYREADS in the term
  bool BVSolver::CheckForArrayReads(const ASTNode& term) {
    ASTNode a = term;
    ASTNodeMap::iterator it;    
    if((it = TermsAlreadySeenMap.find(term)) != TermsAlreadySeenMap.end()) {
      //if the term has been seen, then simply return true, else
      //return false
      if(ASTTrue == (it->second)) {
	return true;
      }
      else {
	return false;
      }
    }

    switch(term.GetKind()) {
    case READ:
      //an array read has been seen. Make an entry in the map and
      //return true
      TermsAlreadySeenMap[term] = ASTTrue;
      return true;
    default: {
      ASTVec c = term.GetChildren();
      for(ASTVec::iterator it=c.begin(),itend=c.end();it!=itend;it++) {
	if(CheckForArrayReads(*it)) {
	  return true;
	}
      }
      break;
    }
    }

    //If control is here, then it means that no arrayread was seen for
    //the input 'term'. Make an entry in the map with the term as key
    //and FALSE as value.
    TermsAlreadySeenMap[term] = ASTFalse;
    return false;
  } //end of CheckForArrayReads()
  
  //check the solver map for 'key'. If key is present, then return the
  //value by reference in the argument 'output'
  bool BeevMgr::CheckSolverMap(const ASTNode& key, ASTNode& output) {
    ASTNodeMap::iterator it;
    if((it = SolverMap.find(key)) != SolverMap.end()) {
      output = it->second;
      return true;
    }
    return false;
  } //end of CheckSolverMap()

  bool BeevMgr::CheckSolverMap(const ASTNode& key) {
    if(SolverMap.find(key) != SolverMap.end())	
      return true;
    else
      return false;
  } //end of CheckSolverMap()
  
  //update solvermap with (key,value) pair
  bool BeevMgr::UpdateSolverMap(const ASTNode& key, const ASTNode& value) {
    ASTNode var = (BVEXTRACT == key.GetKind()) ? key[0] : key;
    if(!CheckSolverMap(var) && key != value) {
      SolverMap[key] = value;
      return true;
    }  
    return false;
  } //end of UpdateSolverMap()

  //collects the vars in the term 'lhs' into the multiset Vars
  void BVSolver::VarsInTheTerm_TopLevel(const ASTNode& lhs, ASTNodeMultiSet& Vars) {
    TermsAlreadySeenMap.clear();
    VarsInTheTerm(lhs,Vars);
  }

  //collects the vars in the term 'lhs' into the multiset Vars
  void BVSolver::VarsInTheTerm(const ASTNode& term, ASTNodeMultiSet& Vars) {
    ASTNode a = term;
    ASTNodeMap::iterator it;    
    if((it = TermsAlreadySeenMap.find(term)) != TermsAlreadySeenMap.end()) {
      //if the term has been seen, then simply return
      return;
    }

    switch(term.GetKind()) {
    case BVCONST:
      return;
    case SYMBOL:
      //cerr << "debugging: symbol added: " << term << endl;
      Vars.insert(term);
      break;
    case READ:
      //skip the arrayname, provided the arrayname is a SYMBOL
      if(SYMBOL == term[0].GetKind()) {
	VarsInTheTerm(term[1],Vars);
      }
      else {
	VarsInTheTerm(term[0],Vars);
	VarsInTheTerm(term[1],Vars);
      }
      break;
    default: {
      ASTVec c = term.GetChildren();
      for(ASTVec::iterator it=c.begin(),itend=c.end();it!=itend;it++) {
	  VarsInTheTerm(*it,Vars);	  
      }
      break;
    }
    }

    //ensures that you don't double count. if you have seen the term
    //once, then memoize
    TermsAlreadySeenMap[term] = ASTTrue;
    return;
  } //end of VarsInTheTerm()  

  bool BVSolver::DoNotSolveThis(const ASTNode& var) {
    if(DoNotSolve_TheseVars.find(var) != DoNotSolve_TheseVars.end()) {
      return true;
    }
    return false;
  }

  //chooses a variable in the lhs and returns the chosen variable
  ASTNode BVSolver::ChooseMonom(const ASTNode& eq, ASTNode& modifiedlhs) {
    if(!(EQ == eq.GetKind() && BVPLUS == eq[0].GetKind())) {
      FatalError("ChooseMonom: input must be a EQ",eq);
    }

    ASTNode lhs = eq[0];
    ASTNode rhs = eq[1];
    ASTNode zero = _bm->CreateZeroConst(32);

    //collect all the vars in the lhs and rhs
    ASTNodeMultiSet Vars;
    VarsInTheTerm_TopLevel(lhs,Vars);

    //handle BVPLUS case
    ASTVec c = lhs.GetChildren();
    ASTVec o;    
    ASTNode outmonom = _bm->CreateNode(UNDEFINED);
    bool chosen_symbol = false;
    bool chosen_odd = false;

    //choose variables with no coeffs
    for(ASTVec::iterator it=c.begin(),itend=c.end();it!=itend;it++) {
      ASTNode monom = *it;
      if(SYMBOL == monom.GetKind() &&
	 Vars.count(monom) == 1    &&	 
	 !_bm->VarSeenInTerm(monom,rhs) &&
	 !DoNotSolveThis(monom)   &&
	 !chosen_symbol) {
	outmonom = monom;
	chosen_symbol = true;
      }
      else if(BVUMINUS == monom.GetKind()  &&
	      SYMBOL == monom[0].GetKind() &&
	      Vars.count(monom[0]) == 1    &&
	      !DoNotSolveThis(monom[0])   &&
	      !_bm->VarSeenInTerm(monom[0],rhs) &&
	      !chosen_symbol) {
	//cerr << "Chosen Monom: " << monom << endl;
	outmonom = monom;
	chosen_symbol = true;
      }
      else {
	o.push_back(monom);
      }
    }

    //try to choose only odd coeffed variables first
    if(!chosen_symbol) {
      o.clear();
      for(ASTVec::iterator it=c.begin(),itend=c.end();it!=itend;it++) {
	ASTNode monom = *it;
	ASTNode var = (BVMULT == monom.GetKind()) ? monom[1] : _bm->CreateNode(UNDEFINED);

	if(BVMULT == monom.GetKind()     && 
	   BVCONST == monom[0].GetKind() &&
	   _bm->BVConstIsOdd(monom[0])   &&
	   ((SYMBOL == var.GetKind()  && 
	     Vars.count(var) == 1) 
	    || 
	    (BVEXTRACT == var.GetKind()  && 
	     SYMBOL == var[0].GetKind()  && 
	     BVCONST == var[1].GetKind() && 
	     zero == var[2] && 
	     !_bm->VarSeenInTerm(var[0],rhs) &&
	     !DoNotSolveThis(var[0]))	    
	    ) &&
	   !DoNotSolveThis(var)     &&
	   !_bm->VarSeenInTerm(var,rhs)  &&
	   !chosen_odd) {
	  //monom[0] is odd.
	  outmonom = monom;
	  chosen_odd = true;
	}
	else {
	o.push_back(monom);
	}
      }
    }

    modifiedlhs = (o.size() > 1) ? _bm->CreateTerm(BVPLUS,lhs.GetValueWidth(),o) : o[0];
    return outmonom;
  } //end of choosemonom()

  //solver function which solves for variables with odd coefficient
  ASTNode BVSolver::BVSolve_Odd(const ASTNode& input) {
    ASTNode eq = input;
    //cerr << "Input to BVSolve_Odd()" << eq << endl;
    if(!(wordlevel_solve && EQ == eq.GetKind())) {
      return input;
    }

    ASTNode output = input;
    if(CheckAlreadySolvedMap(input,output)) {
      return output;
    }

    //get the lhs and the rhs, and case-split on the lhs kind
    ASTNode lhs = eq[0];
    ASTNode rhs = eq[1];
    if(BVPLUS == lhs.GetKind()) {
      ASTNode chosen_monom = _bm->CreateNode(UNDEFINED);
      ASTNode leftover_lhs;

      //choose monom makes sure that it gets only those vars that
      //occur exactly once in lhs and rhs put together
      chosen_monom = ChooseMonom(eq, leftover_lhs);
      if(chosen_monom == _bm->CreateNode(UNDEFINED)) {
	//no monomial was chosen
	return eq;
      }
      
      //if control is here then it means that a monom was chosen
      //
      //construct:  rhs - (lhs without the chosen monom)
      unsigned int len = lhs.GetValueWidth();
      leftover_lhs = _bm->SimplifyTerm_TopLevel(_bm->CreateTerm(BVUMINUS,len,leftover_lhs));      
      ASTNode newrhs = _bm->SimplifyTerm(_bm->CreateTerm(BVPLUS,len,rhs,leftover_lhs));
      lhs = chosen_monom;
      rhs = newrhs;
    } //end of if(BVPLUS ...)

    if(BVUMINUS == lhs.GetKind()) {
      //equation is of the form (-lhs0) = rhs
      ASTNode lhs0 = lhs[0];
      rhs = _bm->SimplifyTerm(_bm->CreateTerm(BVUMINUS,rhs.GetValueWidth(),rhs));
      lhs = lhs0;      
    }

    switch(lhs.GetKind()) {
    case SYMBOL: {     
      //input is of the form x = rhs first make sure that the lhs
      //symbol does not occur on the rhs or that it has not been
      //solved for
      if(_bm->VarSeenInTerm(lhs,rhs)) {
	//found the lhs in the rhs. Abort!
	DoNotSolve_TheseVars.insert(lhs);
	return eq;
      }
      
      //rhs should not have arrayreads in it. it complicates matters
      //during transformation
      // if(CheckForArrayReads_TopLevel(rhs)) {
      //       	return eq;
      //       }

      DoNotSolve_TheseVars.insert(lhs);
      if(!_bm->UpdateSolverMap(lhs,rhs)) {
	return eq;
      }

      output = ASTTrue;
      break;
    }
    case BVEXTRACT: {
      ASTNode zero = _bm->CreateZeroConst(32);
      
      if(!(SYMBOL == lhs[0].GetKind()  && 
      	   BVCONST == lhs[1].GetKind() && 
      	   zero == lhs[2] && 
      	   !_bm->VarSeenInTerm(lhs[0],rhs) &&
      	   !DoNotSolveThis(lhs[0]))) {
      	return eq;
      }
      
      if(_bm->VarSeenInTerm(lhs[0],rhs)) {
      	DoNotSolve_TheseVars.insert(lhs[0]);
      	return eq;
      }
      
      DoNotSolve_TheseVars.insert(lhs[0]);
      if(!_bm->UpdateSolverMap(lhs,rhs)) {
      	return eq;
      }

      //if the extract of x[i:0] = t is entered into the solvermap,
      //then also add another entry for x = x1@t
      ASTNode var = lhs[0];
      ASTNode newvar = NewVar(var.GetValueWidth() - lhs.GetValueWidth());
      newvar = _bm->CreateTerm(BVCONCAT,var.GetValueWidth(),newvar,rhs);
      _bm->UpdateSolverMap(var,newvar);      
      output = ASTTrue;
      break;
    }
    case BVMULT: {
      //the input is of the form a*x = t. If 'a' is odd, then compute
      //its multiplicative inverse a^-1, multiply 't' with it, and
      //update the solver map
      if(BVCONST != lhs[0].GetKind()) {
	return eq;
      }
      
      if(!(SYMBOL == lhs[1].GetKind() ||
	   (BVEXTRACT == lhs[1].GetKind() &&
	   SYMBOL == lhs[1][0].GetKind()))) {
	return eq;
      }

      bool ChosenVar_Is_Extract = (BVEXTRACT == lhs[1].GetKind()) ? true : false;

      //if coeff is even, then we know that all the coeffs in the eqn
      //are even. Simply return the eqn
      if(!_bm->BVConstIsOdd(lhs[0])) {
	return eq;
      }

      ASTNode a = _bm->MultiplicativeInverse(lhs[0]);
      ASTNode chosenvar = (BVEXTRACT == lhs[1].GetKind()) ? lhs[1][0] : lhs[1];
      ASTNode chosenvar_value = 
	_bm->SimplifyTerm(_bm->CreateTerm(BVMULT,rhs.GetValueWidth(),a,rhs));
      
      //if chosenvar is seen in chosenvar_value then abort
      if(_bm->VarSeenInTerm(chosenvar,chosenvar_value)) {
	//abort solving
	DoNotSolve_TheseVars.insert(lhs);
	return eq;
      }

      //rhs should not have arrayreads in it. it complicates matters
      //during transformation
      // if(CheckForArrayReads_TopLevel(chosenvar_value)) {
      //       	return eq;
      //       }
            
      //found a variable to solve
      DoNotSolve_TheseVars.insert(chosenvar);
      chosenvar = lhs[1];
      if(!_bm->UpdateSolverMap(chosenvar,chosenvar_value)) {
	return eq;
      }

      if(ChosenVar_Is_Extract) {
	ASTNode var = lhs[1][0];
	ASTNode newvar = NewVar(var.GetValueWidth() - lhs[1].GetValueWidth());
	newvar = _bm->CreateTerm(BVCONCAT,var.GetValueWidth(),newvar,chosenvar_value);
	_bm->UpdateSolverMap(var,newvar);
      }
      output = ASTTrue;
      break;
    }    
    default:
      output = eq;
      break;
    }
    
    UpdateAlreadySolvedMap(input,output);
    return output;
  } //end of BVSolve_Odd()

  //Create a new variable of ValueWidth 'n'
  ASTNode BVSolver::NewVar(unsigned int n) {
    std:: string c("v");
    char d[32];
    sprintf(d,"%d",_symbol_count++);
    std::string ccc(d);
    c += "_solver_" + ccc;
    
    ASTNode CurrentSymbol = _bm->CreateSymbol(c.c_str());
    CurrentSymbol.SetValueWidth(n);
    CurrentSymbol.SetIndexWidth(0);
    return CurrentSymbol;
  } //end of NewVar()

  //The toplevel bvsolver(). Checks if the formula has already been
  //solved. If not, the solver() is invoked. If yes, then simply drop
  //the formula
  ASTNode BVSolver::TopLevelBVSolve(const ASTNode& input) {
    if(!wordlevel_solve) {
      return input;
    }
    
    Kind k = input.GetKind();
    if(!(EQ == k || AND == k)) {
      return input;
    }

    ASTNode output = input;
    if(CheckAlreadySolvedMap(input,output)) {
      //output is TRUE. The formula is thus dropped
      return output;
    }
    ASTVec o;
    ASTVec c;
    if(EQ == k) 
      c.push_back(input);
    else 
      c = input.GetChildren();
    ASTVec eveneqns;
    ASTNode solved = ASTFalse;
    for(ASTVec::iterator it = c.begin(), itend = c.end();it != itend;it++) { 
      //_bm->ASTNodeStats("Printing before calling simplifyformula inside the solver:", *it);
      ASTNode aaa = (ASTTrue == solved && EQ == it->GetKind()) ? _bm->SimplifyFormula(*it,false) : *it;
      //ASTNode aaa = *it;
      //_bm->ASTNodeStats("Printing after calling simplifyformula inside the solver:", aaa);
      aaa = BVSolve_Odd(aaa);
      //_bm->ASTNodeStats("Printing after oddsolver:", aaa);
      bool even = false;
      aaa = CheckEvenEqn(aaa, even);
      if(even) {
	eveneqns.push_back(aaa);
      }
      else {
	if(ASTTrue != aaa) {
	  o.push_back(aaa);
	}
      }
      solved = aaa;
    }

    ASTNode evens;
    if(eveneqns.size() > 0) {
      //if there is a system of even equations then solve them
      evens = (eveneqns.size() > 1) ? _bm->CreateNode(AND,eveneqns) : eveneqns[0];
      //evens = _bm->SimplifyFormula(evens,false);
      evens = BVSolve_Even(evens);
      _bm->ASTNodeStats("Printing after evensolver:", evens);
    }
    else {
      evens = ASTTrue;
    }
    output = (o.size() > 0) ? ((o.size() > 1) ? _bm->CreateNode(AND,o) : o[0]) : ASTTrue;
    output = _bm->CreateNode(AND,output,evens);

    UpdateAlreadySolvedMap(input,output);
    return output;
  } //end of TopLevelBVSolve()

  ASTNode BVSolver::CheckEvenEqn(const ASTNode& input, bool& evenflag) {
    ASTNode eq = input;
    //cerr << "Input to BVSolve_Odd()" << eq << endl;
    if(!(wordlevel_solve && EQ == eq.GetKind())) {
      evenflag = false;
      return eq;
    }

    ASTNode lhs = eq[0];
    ASTNode rhs = eq[1];
    ASTNode zero = _bm->CreateZeroConst(rhs.GetValueWidth());
    //lhs must be a BVPLUS, and rhs must be a BVCONST
    if(!(BVPLUS == lhs.GetKind() && zero == rhs)) {
      evenflag = false;
      return eq;
    }
    
    ASTVec lhs_c = lhs.GetChildren();
    ASTNode savetheconst = rhs;
    for(ASTVec::iterator it=lhs_c.begin(),itend=lhs_c.end();it!=itend;it++) {
      ASTNode aaa = *it;
      Kind itk = aaa.GetKind();

      if(BVCONST == itk){
	//check later if the constant is even or not
	savetheconst = aaa;
	continue;
      }
      
      if(!(BVMULT == itk &&
	   BVCONST == aaa[0].GetKind() &&
	   SYMBOL == aaa[1].GetKind() &&
	   !_bm->BVConstIsOdd(aaa[0]))) {
	//If the monomials of the lhs are NOT of the form 'a*x' where
	//'a' is even, then return the false
	evenflag = false;
	return eq;
      }  
    }//end of for loop

    //if control is here then it means that all coeffs are even. the
    //only remaining thing is to check if the constant is even or not
    if(_bm->BVConstIsOdd(savetheconst)) {
      //the constant turned out to be odd. we have UNSAT eqn
      evenflag = false;
      return ASTFalse;
    }
    
    //all is clear. the eqn in even, through and through
    evenflag = true;
    return eq;
  } //end of CheckEvenEqn

  //solve an eqn whose monomials have only even coefficients
  ASTNode BVSolver::BVSolve_Even(const ASTNode& input) {
    if(!wordlevel_solve) {
      return input;
    }

    if(!(EQ == input.GetKind() || AND == input.GetKind())) {
      return input;
    }

    ASTNode output;
    if(CheckAlreadySolvedMap(input,output)) {
      return output;
    }

    ASTVec input_c;
    if(EQ == input.GetKind()) {
      input_c.push_back(input);
    }
    else {
      input_c = input.GetChildren();
    }

    //power_of_2 holds the exponent of 2 in the coeff
    unsigned int power_of_2 = 0;
    //we need this additional variable to find the lowest power of 2
    unsigned int power_of_2_lowest = 0xffffffff;
    //the monom which has the least power of 2 in the coeff
    ASTNode monom_with_best_coeff;
    for(ASTVec::iterator jt=input_c.begin(),jtend=input_c.end();jt!=jtend;jt++) {
      ASTNode eq = *jt;
      ASTNode lhs = eq[0];
      ASTNode rhs = eq[1];
      ASTNode zero = _bm->CreateZeroConst(rhs.GetValueWidth());
      //lhs must be a BVPLUS, and rhs must be a BVCONST
      if(!(BVPLUS == lhs.GetKind() && zero == rhs)) {
	return input;
      }
    
      ASTVec lhs_c = lhs.GetChildren();
      ASTNode odd;
      for(ASTVec::iterator it=lhs_c.begin(),itend=lhs_c.end();it!=itend;it++) {
	ASTNode aaa = *it;
	Kind itk = aaa.GetKind();
	if(!(BVCONST == itk &&
	     !_bm->BVConstIsOdd(aaa)) &&
	   !(BVMULT == itk &&
	     BVCONST == aaa[0].GetKind() &&
	     SYMBOL == aaa[1].GetKind() &&
	     !_bm->BVConstIsOdd(aaa[0]))) {
	  //If the monomials of the lhs are NOT of the form 'a*x' or 'a'
	  //where 'a' is even, then return the eqn
	  return input;
	}
	
	//we are gauranteed that if control is here then the monomial is
	//of the form 'a*x' or 'a', where 'a' is even
	ASTNode coeff = (BVCONST == itk) ? aaa : aaa[0];
	odd = SplitEven_into_Oddnum_PowerOf2(coeff,power_of_2);
	if(power_of_2  < power_of_2_lowest) {
	  power_of_2_lowest = power_of_2;
	  monom_with_best_coeff = aaa;
	}
	power_of_2 = 0;
      }//end of inner for loop
    } //end of outer for loop    

    //get the exponent
    power_of_2 = power_of_2_lowest;
    
    //if control is here, we are gauranteed that we have chosen a
    //monomial with fewest powers of 2
    ASTVec formula_out;
    for(ASTVec::iterator jt=input_c.begin(),jtend=input_c.end();jt!=jtend;jt++) {
      ASTNode eq = *jt;      
      ASTNode lhs = eq[0];
      ASTNode rhs = eq[1];
      ASTNode zero = _bm->CreateZeroConst(rhs.GetValueWidth());
      //lhs must be a BVPLUS, and rhs must be a BVCONST
      if(!(BVPLUS == lhs.GetKind() && zero == rhs)) {
	return input;
      }
      
      unsigned len = lhs.GetValueWidth();
      ASTNode hi = _bm->CreateBVConst(32,len-1);
      ASTNode low = _bm->CreateBVConst(32,len - power_of_2);
      ASTNode low_minus_one = _bm->CreateBVConst(32,len - power_of_2 - 1);
      ASTNode low_zero = _bm->CreateZeroConst(32);
      unsigned newlen = len - power_of_2;
      ASTNode two_const = _bm->CreateTwoConst(len);

      unsigned count = power_of_2;
      ASTNode two = two_const;
      while(--count) {
	two = _bm->BVConstEvaluator(_bm->CreateTerm(BVMULT,len,two_const,two));
      }
      ASTVec lhs_c = lhs.GetChildren();
      ASTVec lhs_out;
      for(ASTVec::iterator it=lhs_c.begin(),itend=lhs_c.end();it!=itend;it++) {
	ASTNode aaa = *it;
	Kind itk = aaa.GetKind();
	if(BVCONST == itk) {
	  aaa = _bm->BVConstEvaluator(_bm->CreateTerm(BVDIV,len,aaa,two));
	  aaa = _bm->BVConstEvaluator(_bm->CreateTerm(BVEXTRACT,newlen,aaa,low_minus_one,low_zero));
	}
	else {
	  //it must be of the form a*x
	  ASTNode coeff = _bm->BVConstEvaluator(_bm->CreateTerm(BVDIV,len,aaa[0],two));
	  coeff = _bm->BVConstEvaluator(_bm->CreateTerm(BVEXTRACT,newlen,coeff,low_minus_one,low_zero));
	  ASTNode upper_x, lower_x;
	  //upper_x = _bm->SimplifyTerm(_bm->CreateTerm(BVEXTRACT, power_of_2, aaa[1], hi, low));
	  lower_x = _bm->SimplifyTerm(_bm->CreateTerm(BVEXTRACT, newlen,aaa[1],low_minus_one,low_zero));
	  aaa = _bm->CreateTerm(BVMULT,newlen,coeff,lower_x);
	}
	lhs_out.push_back(aaa);
      }//end of inner forloop()
      rhs = _bm->CreateZeroConst(newlen);
      lhs = _bm->CreateTerm(BVPLUS,newlen,lhs_out);     
      formula_out.push_back(_bm->CreateSimplifiedEQ(lhs,rhs));
    } //end of outer forloop()

    output = 
      (formula_out.size() > 0) ? (formula_out.size() > 1) ? _bm->CreateNode(AND,formula_out) : formula_out[0] : ASTTrue;

    UpdateAlreadySolvedMap(input,output);
    return output;
  } //end of BVSolve_Even()
};//end of namespace BEEV
