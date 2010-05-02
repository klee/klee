/********************************************************************
 * AUTHORS: Vijay Ganesh, David L. Dill
 *
 * BEGIN DATE: November, 2005
 *
 * LICENSE: Please view LICENSE file in the home dir of this Program
 ********************************************************************/
// -*- c++ -*-
#include "AST.h"
#include "ASTUtil.h"
#include "../simplifier/bvsolver.h"
#include <math.h>


namespace BEEV {
  /* FUNCTION: lookup or create a new MINISAT literal
   * lookup or create new MINISAT Vars from the global MAP
   * _ASTNode_to_SATVar.
   */
  MINISAT::Var BeevMgr::LookupOrCreateSATVar(MINISAT::Solver& newS, const ASTNode& n) {  
    ASTtoSATMap::iterator it;  
    MINISAT::Var v;
    
    //look for the symbol in the global map from ASTNodes to ints. if
    //not found, create a S.newVar(), else use the existing one.
    if((it = _ASTNode_to_SATVar.find(n)) == _ASTNode_to_SATVar.end()) {
      v = newS.newVar();
      _ASTNode_to_SATVar[n] = v;	
      
      //ASSUMPTION: I am assuming that the newS.newVar() call increments v
      //by 1 each time it is called, and the initial value of a
      //MINISAT::Var is 0.
      _SATVar_to_AST.push_back(n);
    }
    else
      v = it->second;
    return v;
  }
  
  /* FUNCTION: convert ASTClauses to MINISAT clauses and solve.
   * Accepts ASTClauses and converts them to MINISAT clauses. Then adds
   * the newly minted MINISAT clauses to the local SAT instance, and
   * calls solve(). If solve returns unsat, then stop and return
   * unsat. else continue.
   */  
  // FIXME: Still need to deal with TRUE/FALSE in clauses!
 bool BeevMgr::toSATandSolve(MINISAT::Solver& newS, BeevMgr::ClauseList& cll)
 {
    CountersAndStats("SAT Solver");

    //iterate through the list (conjunction) of ASTclauses cll
    BeevMgr::ClauseList::const_iterator i = cll.begin(), iend = cll.end();
    
    if(i == iend)
      FatalError("toSATandSolve: Nothing to Solve",ASTUndefined);
    
    //turnOffSubsumption
    newS.turnOffSubsumption();

    // (*i) is an ASTVec-ptr which denotes an ASTclause
    for(; i!=iend; i++) {    
      //Clause for the SATSolver
      MINISAT::vec<MINISAT::Lit> satSolverClause;
      
      //now iterate through the internals of the ASTclause itself
      ASTVec::const_iterator j = (*i)->begin(), jend = (*i)->end();
      //j is a disjunct in the ASTclause (*i)
      for(;j!=jend;j++) {

	bool negate = (NOT == j->GetKind()) ? true : false;		
	ASTNode n = negate ? (*j)[0] : *j;
	
	//Lookup or create the MINISAT::Var corresponding to the Booelan
	//ASTNode Variable, and push into sat Solver clause
	MINISAT::Var v = LookupOrCreateSATVar(newS,n);
	MINISAT::Lit l(v, negate);
	satSolverClause.push(l);
      }
      newS.addClause(satSolverClause);
      // clause printing.
      // (printClause<MINISAT::vec<MINISAT::Lit> >)(satSolverClause);
      // cout << " 0 ";
      // cout << endl;
      
      if(newS.okay()) {
	continue;
      }
      else {
	PrintStats(newS.stats);
	return false;
      }
      
      if(!newS.simplifyDB(false)) {
      	PrintStats(newS.stats);
      	return false;
      }
    }

    // if input is UNSAT return false, else return true    
    if(!newS.simplifyDB(false)) {
      PrintStats(newS.stats);
      return false;
    }
    
    //PrintActivityLevels_Of_SATVars("Before SAT:",newS);
    //ChangeActivityLevels_Of_SATVars(newS);
    //PrintActivityLevels_Of_SATVars("Before SAT and after initial bias:",newS); 
    newS.solve();
    //PrintActivityLevels_Of_SATVars("After SAT",newS);

    PrintStats(newS.stats);
    if (newS.okay())
      return true;
    else
      return false;
  }

  // GLOBAL FUNCTION: Prints statistics from the MINISAT Solver   
  void BeevMgr::PrintStats(MINISAT::SolverStats& s) {
    if(!stats)
      return;
    double  cpu_time = MINISAT::cpuTime();
    MINISAT::int64   mem_used = MINISAT::memUsed();
    printf("restarts              : %"I64_fmt"\n", s.starts);
    printf("conflicts             : %-12"I64_fmt"   (%.0f /sec)\n", s.conflicts   , s.conflicts   /cpu_time);
    printf("decisions             : %-12"I64_fmt"   (%.0f /sec)\n", s.decisions   , s.decisions   /cpu_time);
    printf("propagations          : %-12"I64_fmt"   (%.0f /sec)\n", s.propagations, s.propagations/cpu_time);
    printf("conflict literals     : %-12"I64_fmt"   (%4.2f %% deleted)\n", 
           s.tot_literals, 
           (s.max_literals - s.tot_literals)*100 / (double)s.max_literals);
    if (mem_used != 0) printf("Memory used           : %.2f MB\n", mem_used / 1048576.0);
    printf("CPU time              : %g s\n", cpu_time);
    fflush(stdout);
  }
  
  // Prints Satisfying assignment directly, for debugging.
  void BeevMgr::PrintSATModel(MINISAT::Solver& newS) {
    if(!newS.okay())
      FatalError("PrintSATModel: NO COUNTEREXAMPLE TO PRINT",ASTUndefined);
    // FIXME: Don't put tests like this in the print functions.  The print functions
    // should print unconditionally.  Put a conditional around the call if you don't 
    // want them to print
    if(!(stats && print_nodes))
      return;

    int num_vars = newS.nVars();
    cout << "Satisfying assignment: " << endl;
    for (int i = 0; i < num_vars; i++) {
      if (newS.model[i] == MINISAT::l_True) {
	ASTNode s = _SATVar_to_AST[i];
	cout << s << endl;
      }
      else if (newS.model[i] == MINISAT::l_False) {
	ASTNode s = _SATVar_to_AST[i];
	cout << CreateNode(NOT, s) << endl;
      }
    }
  }


  // Looks up truth value of ASTNode SYMBOL in MINISAT satisfying assignment.
  // Returns ASTTrue if true, ASTFalse if false or undefined.
  ASTNode BeevMgr::SymbolTruthValue(MINISAT::Solver &newS, ASTNode form) 
  {
    MINISAT::Var satvar = _ASTNode_to_SATVar[form];
    if (newS.model[satvar] == MINISAT::l_True) {
      return ASTTrue;
    }
    else if (newS.model[satvar] == MINISAT::l_False){
      // False
      return ASTFalse;
    }
    else {
      return (rand() > 4096) ? ASTTrue : ASTFalse; 
    }
  }


  // This function is for debugging problems with BitBlast and especially
  // ToCNF. It evaluates the bit-blasted formula in the satisfying
  // assignment.  While doing that, it checks that every subformula has
  // the same truth value as its representative literal, if it has one.
  // If this condition is violated, it halts immediately (on the leftmost
  // lowest term).
  // Use CreateSimpForm to evaluate, even though it's expensive, so that
  // we can use the partial truth assignment.
  ASTNode BeevMgr::CheckBBandCNF(MINISAT::Solver& newS, ASTNode form)
  {
    // Clear memo table (in case newS has changed).
    CheckBBandCNFMemo.clear();
    // Call recursive version that does the work.
    return CheckBBandCNF_int(newS, form);
  }

  // Recursive body CheckBBandCNF
  // FIXME:  Modify this to just check if result is true, and print mismatch 
  // if not.   Might have a trace flag for the other stuff.
  ASTNode BeevMgr::CheckBBandCNF_int(MINISAT::Solver& newS, ASTNode form)
  {

    //    cout << "++++++++++++++++" << endl << "CheckBBandCNF_int form = " <<
    //      form << endl;
  
    ASTNodeMap::iterator memoit = CheckBBandCNFMemo.find(form);
    if (memoit != CheckBBandCNFMemo.end()) {
      // found it.  Return memoized value.
      return memoit->second;
    }

    ASTNode result;		// return value, to memoize.

    Kind k = form.GetKind();
    switch (k) {
    case TRUE:
    case FALSE: {
      return form;
      break;
    }
    case SYMBOL: 
    case BVGETBIT:  {
      // Look up the truth value
      // ASTNode -> Sat -> Truthvalue -> ASTTrue or ASTFalse;
      // FIXME: Could make up a fresh var in undefined case.

      result = SymbolTruthValue(newS, form);

      cout << "================" << endl << "Checking BB formula:" << form << endl;
      cout << "----------------" << endl << "Result:" << result << endl;

      break;
    }
    default: {
      // Evaluate the children recursively.
      ASTVec eval_children;
      ASTVec ch = form.GetChildren();
      ASTVec::iterator itend = ch.end();
      for(ASTVec::iterator it = ch.begin(); it < itend; it++) {
	eval_children.push_back(CheckBBandCNF_int(newS, *it));
      }
      result = CreateSimpForm(k, eval_children);

      cout << "================" << endl << "Checking BB formula:" << form << endl;
      cout << "----------------" << endl << "Result:" << result << endl;

      ASTNode replit_eval;
      // Compare with replit, if there is one.
      ASTNodeMap::iterator replit_it = RepLitMap.find(form);
      if (replit_it != RepLitMap.end()) {
	ASTNode replit = RepLitMap[form];
	// Replit is symbol or not symbol.
	if (SYMBOL == replit.GetKind()) {
	  replit_eval = SymbolTruthValue(newS, replit);
	}
	else {
	  // It's (NOT sym).  Get value of sym and complement.
	  replit_eval = CreateSimpNot(SymbolTruthValue(newS, replit[0]));
	}

	cout << "----------------" << endl << "Rep lit: " << replit << endl;
	cout << "----------------" << endl << "Rep lit value: " << replit_eval << endl;

	if (result != replit_eval) {
	  // Hit the panic button.
	  FatalError("Truth value of BitBlasted formula disagrees with representative literal in CNF.");
	}
      }
      else {
	cout << "----------------" << endl << "No rep lit" << endl;
      }

    }
    }

    return (CheckBBandCNFMemo[form] = result);
  }

  /*FUNCTION: constructs counterexample from MINISAT counterexample
   * step1 : iterate through MINISAT counterexample and assemble the
   * bits for each AST term. Store it in a map from ASTNode to vector
   * of bools (bits).
   *
   * step2: Iterate over the map from ASTNodes->Vector-of-Bools and
   * populate the CounterExampleMap data structure (ASTNode -> BVConst)
   */
  void BeevMgr::ConstructCounterExample(MINISAT::Solver& newS) {
    //iterate over MINISAT counterexample and construct a map from AST
    //terms to vector of bools. We need this iteration step because
    //MINISAT might return the various bits of a term out of
    //order. Therfore, we need to collect all the bits and assemble
    //them properly
    
    if(!newS.okay())
      return;
    if(!construct_counterexample)
      return;    

    CopySolverMap_To_CounterExample();
    for (int i = 0; i < newS.nVars(); i++) {
      //Make sure that the MINISAT::Var is defined
      if (newS.model[i] != MINISAT::l_Undef) {
	
	//mapping from MINISAT::Vars to ASTNodes. We do not need to
	//print MINISAT vars or CNF vars.
	ASTNode s = _SATVar_to_AST[i];
	
	//assemble the counterexample here
	if(s.GetKind() == BVGETBIT && s[0].GetKind() == SYMBOL) {
	  ASTNode symbol = s[0];
	  unsigned int symbolWidth = symbol.GetValueWidth();
	  
	  //'v' is the map from bit-index to bit-value
	  hash_map<unsigned,bool> * v;	
	  if(_ASTNode_to_Bitvector.find(symbol) == _ASTNode_to_Bitvector.end())
	    _ASTNode_to_Bitvector[symbol] = new hash_map<unsigned,bool>(symbolWidth);	
	  
	  //v holds the map from bit-index to bit-value
	  v = _ASTNode_to_Bitvector[symbol];
	  
	  //kk is the index of BVGETBIT
	  unsigned int kk = GetUnsignedConst(s[1]); 	
	  
	  //Collect the bits of 'symbol' and store in v. Store in reverse order.
	  if(newS.model[i]==MINISAT::l_True)
	    (*v)[(symbolWidth-1) - kk] = true;
	  else
	    (*v)[(symbolWidth-1) - kk] = false;
	}
	else {	 
	  if(s.GetKind() == SYMBOL && s.GetType() == BOOLEAN_TYPE) {
	    const char * zz = s.GetName();
	    //if the variables are not cnf variables then add them to the counterexample
	    if(0 != strncmp("cnf",zz,3) && 0 != strcmp("*TrueDummy*",zz)) {
	      if(newS.model[i]==MINISAT::l_True)
		CounterExampleMap[s] = ASTTrue;
	      else
		CounterExampleMap[s] = ASTFalse;	    
	    }
	  }
	}
      }
    }

    //iterate over the ASTNode_to_Bitvector data-struct and construct
    //the the aggregate value of the bitvector, and populate the
    //CounterExampleMap datastructure
    for(ASTtoBitvectorMap::iterator it=_ASTNode_to_Bitvector.begin(),itend=_ASTNode_to_Bitvector.end();
	it!=itend;it++) {
      ASTNode var = it->first;      
      //debugging
      //cerr << var;
      if(SYMBOL != var.GetKind())
	FatalError("ConstructCounterExample: error while constructing counterexample: not a variable: ",var);

      //construct the bitvector value
      hash_map<unsigned,bool> * w = it->second;
      ASTNode value = BoolVectoBVConst(w, var.GetValueWidth());      
      //debugging
      //cerr << value;

      //populate the counterexample datastructure. add only scalars
      //variables which were declared in the input and newly
      //introduced variables for array reads
      CounterExampleMap[var] = value;
    }
    
    //In this loop, we compute the value of each array read, the
    //corresponding ITE against the counterexample generated above.
    for(ASTNodeMap::iterator it=_arrayread_ite.begin(),itend=_arrayread_ite.end();
	it!=itend;it++){
      //the array read
      ASTNode arrayread = it->first;
      ASTNode value_ite = _arrayread_ite[arrayread];
      
      //convert it to a constant array-read and store it in the
      //counter-example. First convert the index into a constant. then
      //construct the appropriate array-read and store it in the
      //counterexample
      ASTNode arrayread_index = TermToConstTermUsingModel(arrayread[1]);
      ASTNode key = CreateTerm(READ,arrayread.GetValueWidth(),arrayread[0],arrayread_index);

      //Get the ITE corresponding to the array-read and convert it
      //to a constant against the model
      ASTNode value = TermToConstTermUsingModel(value_ite);
      //save the result in the counter_example
      if(!CheckSubstitutionMap(key))
	CounterExampleMap[key] = value;      
    }
  } //End of ConstructCounterExample

  // FUNCTION: accepts a non-constant term, and returns the
  // corresponding constant term with respect to a model. 
  //
  // term READ(A,i) is treated as follows:
  //
  //1. If (the boolean variable 'ArrayReadFlag' is true && ArrayRead
  //1. has value in counterexample), then return the value of the
  //1. arrayread.
  //
  //2. If (the boolean variable 'ArrayReadFlag' is true && ArrayRead
  //2. doesn't have value in counterexample), then return the
  //2. arrayread itself (normalized such that arrayread has a constant
  //2. index)
  //
  //3. If (the boolean variable 'ArrayReadFlag' is false) && ArrayRead
  //3. has a value in the counterexample then return the value of the
  //3. arrayread.
  //
  //4. If (the boolean variable 'ArrayReadFlag' is false) && ArrayRead
  //4. doesn't have a value in the counterexample then return 0 as the
  //4. value of the arrayread.
  ASTNode BeevMgr::TermToConstTermUsingModel(const ASTNode& t, bool ArrayReadFlag) {
    Begin_RemoveWrites = false;
    SimplifyWrites_InPlace_Flag = false;
    //ASTNode term = SimplifyTerm(t);
    ASTNode term = t;
    Kind k = term.GetKind();
    

    //cerr << "Input to TermToConstTermUsingModel: " << term << endl;
    if(!is_Term_kind(k)) {
      FatalError("TermToConstTermUsingModel: The input is not a term: ",term);
    }
    if(k == WRITE) {
      FatalError("TermToConstTermUsingModel: The input has wrong kind: WRITE : ",term);
    }
    if(k == SYMBOL && BOOLEAN_TYPE == term.GetType()) {
      FatalError("TermToConstTermUsingModel: The input has wrong kind: Propositional variable : ",term);
    }

    ASTNodeMap::iterator it1;
    if((it1 = CounterExampleMap.find(term)) != CounterExampleMap.end()) {
      ASTNode val = it1->second;
      if(BVCONST != val.GetKind()) {
	//CounterExampleMap has two maps rolled into
	//one. SubstitutionMap and SolverMap.
	//
	//recursion is fine here. There are two maps that are checked
	//here. One is the substitutionmap. We garuntee that the value
	//of a key in the substitutionmap is always a constant.
	//
	//in the SolverMap we garuntee that "term" does not occur in
	//the value part of the map
	if(term == val) {
	  FatalError("TermToConstTermUsingModel: The input term is stored as-is "
		     "in the CounterExample: Not ok: ",term);    
	}
	return TermToConstTermUsingModel(val,ArrayReadFlag);
      }
      else {
	return val;
      }
    }

    ASTNode output;
    switch(k) {
    case BVCONST:
      output = term;
      break;
    case SYMBOL: {
      if(term.GetType() == ARRAY_TYPE) {
	return term;
      }

      //when all else fails set symbol values to some constant by
      //default. if the variable is queried the second time then add 1
      //to and return the new value.
      ASTNode zero = CreateZeroConst(term.GetValueWidth());
      output = zero;
      break;    
    }
    case READ: {      
      ASTNode arrName = term[0];
      ASTNode index = term[1];
      if(0 == arrName.GetIndexWidth()) {
	FatalError("TermToConstTermUsingModel: array has 0 index width: ",arrName);
      }
      
      //READ over a WRITE
      if(WRITE == arrName.GetKind()) {
	ASTNode wrtterm = Expand_ReadOverWrite_UsingModel(term, ArrayReadFlag);
	if(wrtterm == term) {
	  FatalError("TermToConstTermUsingModel: Read_Over_Write term must be expanded into an ITE", term);
	}
	ASTNode rtterm = TermToConstTermUsingModel(wrtterm,ArrayReadFlag);	
	return rtterm;
      } 
      //READ over an ITE
      if(ITE == arrName.GetKind()) {
	arrName = TermToConstTermUsingModel(arrName,ArrayReadFlag);
      }

      ASTNode modelentry;
      if(CounterExampleMap.find(index) != CounterExampleMap.end()) {	
	//index has a const value in the CounterExampleMap
	ASTNode indexVal = CounterExampleMap[index];
	modelentry = CreateTerm(READ, arrName.GetValueWidth(), arrName, indexVal);
      }
      else { 
	//index does not have a const value in the CounterExampleMap. compute it.
	ASTNode indexconstval = TermToConstTermUsingModel(index,ArrayReadFlag);
	//update model with value of the index
	//CounterExampleMap[index] = indexconstval;
	modelentry = CreateTerm(READ,arrName.GetValueWidth(), arrName,indexconstval);	
      }
      //modelentry is now an arrayread over a constant index
      BVTypeCheck(modelentry);
      
      //if a value exists in the CounterExampleMap then return it
      if(CounterExampleMap.find(modelentry) != CounterExampleMap.end()) {
	output = TermToConstTermUsingModel(CounterExampleMap[modelentry],ArrayReadFlag);
      }
      else if(ArrayReadFlag) {
	//return the array read over a constantindex
	output = modelentry;
      }
      else {
	//when all else fails set symbol values to some constant by
	//default. if the variable is queried the second time then add 1
	//to and return the new value.
	ASTNode zero = CreateZeroConst(modelentry.GetValueWidth());
	output = zero;
      }
      break;
    }
    case ITE: {
      ASTNode condcompute = ComputeFormulaUsingModel(term[0]);
      if(ASTTrue == condcompute) {
	output = TermToConstTermUsingModel(term[1],ArrayReadFlag);
      }
      else if(ASTFalse == condcompute) {
	output = TermToConstTermUsingModel(term[2],ArrayReadFlag);
      } 
      else {
	cerr << "TermToConstTermUsingModel: termITE: value of conditional is wrong: " << condcompute << endl; 
	FatalError(" TermToConstTermUsingModel: termITE: cannot compute ITE conditional against model: ",term);
      }
      break;
    }
    default: {
      ASTVec c = term.GetChildren();
      ASTVec o;
      for(ASTVec::iterator it=c.begin(),itend=c.end();it!=itend;it++) {
	ASTNode ff = TermToConstTermUsingModel(*it,ArrayReadFlag);
	o.push_back(ff);
      }
      output = CreateTerm(k,term.GetValueWidth(),o);
      //output is a CONST expression. compute its value and store it
      //in the CounterExampleMap
      ASTNode oo = BVConstEvaluator(output);
      //the return value
      output = oo;
      break;
    }
    }

    //when this flag is false, we should compute the arrayread to a
    //constant. this constant is stored in the counter_example
    //datastructure
    if(!ArrayReadFlag) {
      CounterExampleMap[term] = output;
    }
    
    //cerr << "Output to TermToConstTermUsingModel: " << output << endl;
    return output;
  } //End of TermToConstTermUsingModel

  //Expands read-over-write by evaluating (readIndex=writeIndex) for
  //every writeindex until, either it evaluates to TRUE or all
  //(readIndex=writeIndex) evaluate to FALSE
  ASTNode BeevMgr::Expand_ReadOverWrite_UsingModel(const ASTNode& term, bool arrayread_flag) {
    if(READ != term.GetKind() && 
       WRITE != term[0].GetKind()) {
      FatalError("RemovesWrites: Input must be a READ over a WRITE",term);
    }

    ASTNode output;
    ASTNodeMap::iterator it1;    
    if((it1 = CounterExampleMap.find(term)) != CounterExampleMap.end()) {
      ASTNode val = it1->second;
      if(BVCONST != val.GetKind()) {
	//recursion is fine here. There are two maps that are checked
	//here. One is the substitutionmap. We garuntee that the value
	//of a key in the substitutionmap is always a constant.
	if(term == val) {
	  FatalError("TermToConstTermUsingModel: The input term is stored as-is "
		     "in the CounterExample: Not ok: ",term);    
	}
	return TermToConstTermUsingModel(val,arrayread_flag);
      }
      else {
	return val;
      }
    }

    unsigned int width = term.GetValueWidth();
    ASTNode writeA = ASTTrue; 
    ASTNode newRead = term;
    ASTNode readIndex = TermToConstTermUsingModel(newRead[1],false);
    //iteratively expand read-over-write, and evaluate against the
    //model at every iteration
    do {
      ASTNode write = newRead[0];
      writeA = write[0];
      ASTNode writeIndex = TermToConstTermUsingModel(write[1],false);
      ASTNode writeVal = TermToConstTermUsingModel(write[2],false);
      
      ASTNode cond = ComputeFormulaUsingModel(CreateSimplifiedEQ(writeIndex,readIndex));
      if(ASTTrue == cond) {
	//found the write-value. return it
	output = writeVal;
	CounterExampleMap[term] = output;
	return output;
      }

      newRead = CreateTerm(READ,width,writeA,readIndex);
    } while(READ == newRead.GetKind() && WRITE == newRead[0].GetKind());
    
    output = TermToConstTermUsingModel(newRead,arrayread_flag);
        
    //memoize
    CounterExampleMap[term] = output;
    return output;  
  } //Exand_ReadOverWrite_To_ITE_UsingModel()

  /* FUNCTION: accepts a non-constant formula, and checks if the
   * formula is ASTTrue or ASTFalse w.r.t to a model
   */
  ASTNode BeevMgr::ComputeFormulaUsingModel(const ASTNode& form) {
    ASTNode in = form;
    Kind k = form.GetKind();
    if(!(is_Form_kind(k) && BOOLEAN_TYPE == form.GetType())) {
      FatalError(" ComputeConstFormUsingModel: The input is a non-formula: ", form);
    }

    //cerr << "Input to ComputeFormulaUsingModel:" << form << endl;
    ASTNodeMap::iterator it1;
    if((it1 = ComputeFormulaMap.find(form)) != ComputeFormulaMap.end()) {
      ASTNode res = it1->second;
      if(ASTTrue == res || ASTFalse == res) {
	return res;
      }
      else {
	FatalError("ComputeFormulaUsingModel: The value of a formula must be TRUE or FALSE:", form);
      }
    }
    
    ASTNode t0,t1;
    ASTNode output = ASTFalse;
    switch(k) {
    case TRUE:
    case FALSE:
      output = form;      
      break;
    case SYMBOL:
      if(BOOLEAN_TYPE != form.GetType())
	FatalError(" ComputeFormulaUsingModel: Non-Boolean variables are not formulas",form);
      if(CounterExampleMap.find(form) != CounterExampleMap.end()) {
	ASTNode counterexample_val = CounterExampleMap[form];
	if(!VarSeenInTerm(form,counterexample_val)) {
	  output = ComputeFormulaUsingModel(counterexample_val);
	}
	else {
	  output = counterexample_val;
	}
      }
      else
	output = ASTFalse;
      break;  
    case EQ:
    case NEQ:
    case BVLT:
    case BVLE:
    case BVGT:
    case BVGE:
    case BVSLT:
    case BVSLE:
    case BVSGT:
    case BVSGE:
      //convert form[0] into a constant term
      t0 = TermToConstTermUsingModel(form[0],false);
      //convert form[0] into a constant term
      t1 = TermToConstTermUsingModel(form[1],false);
      output = BVConstEvaluator(CreateNode(k,t0,t1));
      
      //evaluate formula to false if bvdiv execption occurs while
      //counterexample is being checked during refinement.
      if(bvdiv_exception_occured && 
	 counterexample_checking_during_refinement) {
	output = ASTFalse;
      }
      break;   
    case NAND: {
      ASTNode o = ASTTrue;
      for(ASTVec::const_iterator it=form.begin(),itend=form.end();it!=itend;it++)
	if(ASTFalse == ComputeFormulaUsingModel(*it)) {
	  o = ASTFalse;
	  break;
	}      
      if(o == ASTTrue) 
	output = ASTFalse;
      else 
	output = ASTTrue;
      break;
    }
    case NOR: {
      ASTNode o = ASTFalse;
      for(ASTVec::const_iterator it=form.begin(),itend=form.end();it!=itend;it++)
	if(ASTTrue == ComputeFormulaUsingModel(*it)) {
	  o = ASTTrue;
	  break;
	}
      if(o == ASTTrue) 
	output = ASTFalse;
      else 
	output = ASTTrue;
      break;
    }
    case NOT:
      if(ASTTrue == ComputeFormulaUsingModel(form[0]))
	output = ASTFalse;
      else
	output = ASTTrue;
      break;
    case OR:
      for(ASTVec::const_iterator it=form.begin(),itend=form.end();it!=itend;it++) 
	if(ASTTrue == ComputeFormulaUsingModel(*it))
	  output = ASTTrue;
      break;
    case AND:
      output = ASTTrue;
      for(ASTVec::const_iterator it=form.begin(),itend=form.end();it!=itend;it++) {
	if(ASTFalse == ComputeFormulaUsingModel(*it)) {	    
	  output = ASTFalse;
	  break;	  
	}
      }
      break;
    case XOR:
      t0 = ComputeFormulaUsingModel(form[0]);
      t1 = ComputeFormulaUsingModel(form[1]);
      if((ASTTrue == t0 && ASTTrue == t1) || (ASTFalse == t0 && ASTFalse == t1))
	output = ASTFalse;
      else
	output = ASTTrue;
      break;
    case IFF:
      t0 = ComputeFormulaUsingModel(form[0]);
      t1 = ComputeFormulaUsingModel(form[1]);
      if((ASTTrue == t0 && ASTTrue == t1) || (ASTFalse == t0 && ASTFalse == t1))
	output = ASTTrue;
      else
	output = ASTFalse;
      break;
    case IMPLIES:
      t0 = ComputeFormulaUsingModel(form[0]);
      t1 = ComputeFormulaUsingModel(form[1]);
      if((ASTFalse == t0) || (ASTTrue == t0 && ASTTrue == t1))
	output = ASTTrue;
      else
	output = ASTFalse;
      break;    
    case ITE:
      t0 = ComputeFormulaUsingModel(form[0]);
      if(ASTTrue == t0)
	output = ComputeFormulaUsingModel(form[1]);
      else if(ASTFalse == t0)
	output = ComputeFormulaUsingModel(form[2]);
      else 
	FatalError("ComputeFormulaUsingModel: ITE: something is wrong with the formula: ",form);
      break;
    default:
      FatalError(" ComputeFormulaUsingModel: the kind has not been implemented", ASTUndefined);
      break;
    }

    //cout << "ComputeFormulaUsingModel output is:" << output << endl;
    ComputeFormulaMap[form] = output;
    return output;
  }

  void BeevMgr::CheckCounterExample(bool t) {
    // FIXME:  Code is more useful if enable flags are check OUTSIDE the method.
    // If I want to check a counterexample somewhere, I don't want to have to set
    // the flag in order to make it actualy happen!

    if(!check_counterexample) {
      return;
    }

    //input is valid, no counterexample to check
    if(ValidFlag)
      return;

    //t is true if SAT solver generated a counterexample, else it is false
    if(!t)
      FatalError("CheckCounterExample: No CounterExample to check", ASTUndefined);
    const ASTVec c = GetAsserts();    
    for(ASTVec::const_iterator it=c.begin(),itend=c.end();it!=itend;it++)
      if(ASTFalse == ComputeFormulaUsingModel(*it))
	FatalError("CheckCounterExample:counterexample bogus:"\
		   "assert evaluates to FALSE under counterexample: NOT OK",*it);
        
    if(ASTTrue == ComputeFormulaUsingModel(_current_query))
      FatalError("CheckCounterExample:counterexample bogus:"\
		 "query evaluates to TRUE under counterexample: NOT OK",_current_query);
  }

  /* FUNCTION: prints a counterexample for INVALID inputs.  iterate
   * through the CounterExampleMap data structure and print it to
   * stdout
   */
  void BeevMgr::PrintCounterExample(bool t, std::ostream& os) {
    //global command-line option
    // FIXME: This should always print the counterexample.  If you want
    // to turn it off, check the switch at the point of call.
    if(!print_counterexample)
      return;

    //input is valid, no counterexample to print
    if(ValidFlag)
      return;

    //if this option is true then print the way dawson wants using a
    //different printer. do not use this printer.
    if(print_arrayval_declaredorder)
      return;

    //t is true if SAT solver generated a counterexample, else it is
    //false
    if(!t) {
      cerr << "PrintCounterExample: No CounterExample to print: " << endl;
      return;
    }

    //os << "\nCOUNTEREXAMPLE: \n" << endl;
    ASTNodeMap::iterator it  = CounterExampleMap.begin();
    ASTNodeMap::iterator itend = CounterExampleMap.end();
    for(;it!=itend;it++) {
      ASTNode f = it->first;
      ASTNode se = it->second;
      
      if(ARRAY_TYPE == se.GetType()) {
	FatalError("TermToConstTermUsingModel: entry in counterexample is an arraytype. bogus:",se);
      }

      //skip over introduced variables
      if(f.GetKind() == SYMBOL && (_introduced_symbols.find(f) != _introduced_symbols.end())) 
	continue;
      if(f.GetKind() == SYMBOL || 
	 (f.GetKind() == READ && f[0].GetKind() == SYMBOL && f[1].GetKind() == BVCONST)) {
	os << "ASSERT( ";
	f.PL_Print(os,0);
	os << " = ";	
	if(BITVECTOR_TYPE == se.GetType()) {
	  TermToConstTermUsingModel(se,false).PL_Print(os,0);
	}
	else {
	  se.PL_Print(os,0);
	}
	os << " );" << endl;
      }
    }	      
    //os << "\nEND OF COUNTEREXAMPLE" << endl;
  } //End of PrintCounterExample

  /* iterate through the CounterExampleMap data structure and print it
   * to stdout. this function prints only the declared array variables
   * IN the ORDER in which they were declared. It also assumes that
   * the variables are of the form 'varname_number'. otherwise it will
   * not print anything. This function was specifically written for
   * Dawson Engler's group (bug finding research group at Stanford)
   */
  void BeevMgr::PrintCounterExample_InOrder(bool t) {
    //global command-line option to print counterexample. we do not
    //want both counterexample printers to print at the sametime.
    // FIXME: This should always print the counterexample.  If you want
    // to turn it off, check the switch at the point of call.
    if(print_counterexample)
      return;

    //input is valid, no counterexample to print
    if(ValidFlag)
      return;
    
    //print if the commandline option is '-q'. allows printing the
    //counterexample in order.
    if(!print_arrayval_declaredorder)
      return;

    //t is true if SAT solver generated a counterexample, else it is
    //false
    if(!t) {
      cerr << "PrintCounterExample: No CounterExample to print: " << endl;
      return;
    }
    
    //vector to store the integer values
    std::vector<int> out_int;	
    cout << "% ";
    for(ASTVec::iterator it=_special_print_set.begin(),itend=_special_print_set.end();
	it!=itend;it++) {
      if(ARRAY_TYPE == it->GetType()) {
	//get the name of the variable
	const char * c = it->GetName();
	std::string ss(c);
	if(!(0 == strncmp(ss.c_str(),"ini_",4)))
	  continue;
	reverse(ss.begin(),ss.end());

	//cout << "debugging: " << ss;
	size_t pos = ss.find('_',0);
	if(!(0 < pos && pos < ss.size()))
	  continue;

	//get the associated length
	std::string sss = ss.substr(0,pos);
	reverse(sss.begin(),sss.end());
	int n = atoi(sss.c_str());

	it->PL_Print(cout,2);
	for(int j=0;j < n; j++) {
	  ASTNode index = CreateBVConst(it->GetIndexWidth(),j);
	  ASTNode readexpr = CreateTerm(READ,it->GetValueWidth(),*it,index);
	  ASTNode val = GetCounterExample(t, readexpr);
	  //cout << "ASSERT( ";
	  //cout << " = ";	  
	  out_int.push_back(GetUnsignedConst(val));
	  //cout << "\n";
	}
      }
    }
    cout << endl;
    for(unsigned int jj=0; jj < out_int.size();jj++)
      cout << out_int[jj] << endl;
    cout << endl;
  } //End of PrintCounterExample_InOrder

  /* FUNCTION: queries the CounterExampleMap object with 'expr' and
   * returns the corresponding counterexample value.
   */
  ASTNode BeevMgr::GetCounterExample(bool t, const ASTNode& expr) {    
    //input is valid, no counterexample to get
    if(ValidFlag)
      return ASTUndefined;
    
    if(BOOLEAN_TYPE == expr.GetType()) {
      return ComputeFormulaUsingModel(expr);
    }

    if(BVCONST == expr.GetKind()) {
      return expr;
    }

    ASTNodeMap::iterator it;
    ASTNode output;
    if((it = CounterExampleMap.find(expr)) != CounterExampleMap.end())
      output =  TermToConstTermUsingModel(CounterExampleMap[expr],false);
    else
      output = CreateZeroConst(expr.GetValueWidth());
    return output;
  } //End of GetCounterExample

  // FIXME:  Don't use numeric codes.  Use an enum type!
  //Acceps a query, calls the SAT solver and generates Valid/InValid.
  //if returned 0 then input is INVALID
  //if returned 1 then input is VALID
  //if returned 2 then ERROR
  int BeevMgr::TopLevelSAT( const ASTNode& inputasserts, const ASTNode& query) {  
    /******start solving**********/
    ASTNode q = CreateNode(AND, inputasserts, CreateNode(NOT,query));
    ASTNode orig_input = q;
    ASTNodeStats("input asserts and query: ", q);
 
    ASTNode newq = q;
    //round of substitution, solving, and simplification. ensures that
    //DAG is minimized as much as possibly, and ideally should
    //garuntee that all liketerms in BVPLUSes have been combined.
    BVSolver bvsolver(this);
    SimplifyWrites_InPlace_Flag = false;
    Begin_RemoveWrites = false;
    start_abstracting = false;    
    TermsAlreadySeenMap.clear();
    do {
      q = newq;
      newq = CreateSubstitutionMap(newq);
      //ASTNodeStats("after pure substitution: ", newq);
      newq = SimplifyFormula_TopLevel(newq,false);
      //ASTNodeStats("after simplification: ", newq);
      //newq = bvsolver.TopLevelBVSolve(newq);
      //ASTNodeStats("after solving: ", newq);      
    }while(q!=newq);

    ASTNodeStats("Before SimplifyWrites_Inplace begins: ", newq);
    SimplifyWrites_InPlace_Flag = true;
    Begin_RemoveWrites = false;
    start_abstracting = false;
    TermsAlreadySeenMap.clear();
    do {
      q = newq;
      //newq = CreateSubstitutionMap(newq);
      //ASTNodeStats("after pure substitution: ", newq);
      newq = SimplifyFormula_TopLevel(newq,false);
      //ASTNodeStats("after simplification: ", newq);
      newq = bvsolver.TopLevelBVSolve(newq);
      //ASTNodeStats("after solving: ", newq);      
    }while(q!=newq);
    ASTNodeStats("After SimplifyWrites_Inplace: ", newq);        

    start_abstracting = (arraywrite_refinement) ? true : false;
    SimplifyWrites_InPlace_Flag = false;
    Begin_RemoveWrites = (start_abstracting) ? false : true;    
    if(start_abstracting) {
      ASTNodeStats("before abstraction round begins: ", newq);
    }

    TermsAlreadySeenMap.clear();
    do {
      q = newq;
      //newq = CreateSubstitutionMap(newq);
      //Begin_RemoveWrites = true;
      //ASTNodeStats("after pure substitution: ", newq);
      newq = SimplifyFormula_TopLevel(newq,false);
      //ASTNodeStats("after simplification: ", newq);
      //newq = bvsolver.TopLevelBVSolve(newq);
      //ASTNodeStats("after solving: ", newq);
    }while(q!=newq);

    if(start_abstracting) {
      ASTNodeStats("After abstraction: ", newq);
    }
    start_abstracting = false;
    SimplifyWrites_InPlace_Flag = false;
    Begin_RemoveWrites = false;    
    
    newq = TransformFormula(newq);
    ASTNodeStats("after transformation: ", newq);
    TermsAlreadySeenMap.clear();

    int res;    
    //solver instantiated here
    MINISAT::Solver newS;
    if(arrayread_refinement) {
      counterexample_checking_during_refinement = true;
    }
    
    //call SAT and check the result
    res = CallSAT_ResultCheck(newS,newq,orig_input);
    if(2 != res) {
      CountersAndStats("print_func_stats");
      return res;
    }

    res = SATBased_ArrayReadRefinement(newS,newq,orig_input);
    if(2 != res) {
      CountersAndStats("print_func_stats");
      return res;
    }
 
    res = SATBased_ArrayWriteRefinement(newS,orig_input);
    if(2 != res) {
      CountersAndStats("print_func_stats");
      return res;
    }          

    res = SATBased_ArrayReadRefinement(newS,newq,orig_input);
    if(2 != res) {
      CountersAndStats("print_func_stats");
      return res;
    }

    FatalError("TopLevelSAT: reached the end without proper conclusion:" 
	       "either a divide by zero in the input or a bug in STP");
    //bogus return to make the compiler shut up
    return 2;
  } //End of TopLevelSAT

  //go over the list of indices for each array, and generate Leibnitz
  //axioms. Then assert these axioms into the SAT solver. Check if the
  //addition of the new constraints has made the bogus counterexample
  //go away. if yes, return the correct answer. if no, continue adding
  //Leibnitz axioms systematically.
  // FIXME:  What it really does is, for each array, loop over each index i.
  // inside that loop, it finds all the true and false axioms with i as first
  // index.  When it's got them all, it adds the false axioms to the formula
  // and re-solves, and returns if the result is correct.  Otherwise, it
  // goes on to the next index.
  // If it gets through all the indices without a correct result (which I think
  // is impossible, but this is pretty confusing), it then solves with all
  // the true axioms, too.
  // This is not the most obvious way to do it, and I don't know how it 
  // compares with other approaches (e.g., one false axiom at a time or
  // all the false axioms each time).
  int BeevMgr::SATBased_ArrayReadRefinement(MINISAT::Solver& newS, 
					    const ASTNode& q, const ASTNode& orig_input) {
    if(!arrayread_refinement)
      FatalError("SATBased_ArrayReadRefinement: Control should not reach here");

    ASTVec FalseAxiomsVec, RemainingAxiomsVec;
    RemainingAxiomsVec.push_back(ASTTrue);   
    FalseAxiomsVec.push_back(ASTTrue);

    //in these loops we try to construct Leibnitz axioms and add it to
    //the solve(). We add only those axioms that are false in the
    //current counterexample. we keep adding the axioms until there
    //are no more axioms to add
    //
    //for each array, fetch its list of indices seen so far
    for(ASTNodeToVecMap::iterator iset = _arrayname_readindices.begin(), iset_end = _arrayname_readindices.end();
	iset!=iset_end;iset++) {
      ASTVec listOfIndices = iset->second;
      //loop over the list of indices for the array and create LA, and add to q
      for(ASTVec::iterator it=listOfIndices.begin(),itend=listOfIndices.end();it!=itend;it++) {
	if(BVCONST == it->GetKind()) {
	  continue;
	}	

	ASTNode the_index = *it;
	//get the arrayname
	ASTNode ArrName = iset->first;
	// if(SYMBOL != ArrName.GetKind())
	// 	  FatalError("SATBased_ArrayReadRefinement: arrname is not a SYMBOL",ArrName);
	ASTNode arr_read1 = CreateTerm(READ, ArrName.GetValueWidth(), ArrName, the_index);
	//get the variable corresponding to the array_read1
	ASTNode arrsym1 = _arrayread_symbol[arr_read1];
	if(!(SYMBOL == arrsym1.GetKind() || BVCONST == arrsym1.GetKind()))
	  FatalError("TopLevelSAT: refinementloop:term arrsym1 corresponding to READ must be a var", arrsym1);

	//we have nonconst index here. create Leibnitz axiom for it
	//w.r.t every index in listOfIndices
	for(ASTVec::iterator it1=listOfIndices.begin(),itend1=listOfIndices.end();
	    it1!=itend1;it1++) {
	  ASTNode compare_index = *it1;
	  //do not compare with yourself
	  if(the_index == compare_index)
	    continue;
	  
	  //prepare for SAT LOOP 
	  //first construct the antecedent for the LA axiom
	  ASTNode eqOfIndices = 
	    (exprless(the_index,compare_index)) ? 
	    CreateSimplifiedEQ(the_index,compare_index) : CreateSimplifiedEQ(compare_index,the_index);
	  	  
	  ASTNode arr_read2 = CreateTerm(READ, ArrName.GetValueWidth(), ArrName, compare_index);
	  //get the variable corresponding to the array_read2
	  ASTNode arrsym2 = _arrayread_symbol[arr_read2];
	  if(!(SYMBOL == arrsym2.GetKind() || BVCONST == arrsym2.GetKind()))
	    FatalError("TopLevelSAT: refinement loop:"
		       "term arrsym2 corresponding to READ must be a var", arrsym2);
	  
	  ASTNode eqOfReads = CreateSimplifiedEQ(arrsym1,arrsym2);
	  //construct appropriate Leibnitz axiom
	  ASTNode LeibnitzAxiom = CreateNode(IMPLIES, eqOfIndices, eqOfReads);
	  if(ASTFalse == ComputeFormulaUsingModel(LeibnitzAxiom))
	    //FalseAxioms = CreateNode(AND,FalseAxioms,LeibnitzAxiom);
	    FalseAxiomsVec.push_back(LeibnitzAxiom);
	  else
	    //RemainingAxioms = CreateNode(AND,RemainingAxioms,LeibnitzAxiom);
	    RemainingAxiomsVec.push_back(LeibnitzAxiom);
	}
	ASTNode FalseAxioms = (FalseAxiomsVec.size()>1) ? CreateNode(AND,FalseAxiomsVec) : FalseAxiomsVec[0];
	ASTNodeStats("adding false readaxioms to SAT: ", FalseAxioms);  
	int res2 = CallSAT_ResultCheck(newS,FalseAxioms,orig_input);
	if(2!=res2) {
	  return res2;
	}	
      }
    }
    ASTNode RemainingAxioms = (RemainingAxiomsVec.size()>1) ? CreateNode(AND,RemainingAxiomsVec):RemainingAxiomsVec[0];
    ASTNodeStats("adding remaining readaxioms to SAT: ", RemainingAxioms);  
    return CallSAT_ResultCheck(newS,RemainingAxioms,orig_input);
  } //end of SATBased_ArrayReadRefinement

  ASTNode BeevMgr::Create_ArrayWriteAxioms(const ASTNode& term, const ASTNode& newvar) {
    if(READ != term.GetKind() && WRITE != term[0].GetKind()) {
      FatalError("Create_ArrayWriteAxioms: Input must be a READ over a WRITE",term);
    }
    
    ASTNode lhs = newvar;
    ASTNode rhs = term;
    ASTNode arraywrite_axiom = CreateSimplifiedEQ(lhs,rhs);
    return arraywrite_axiom;
  }//end of Create_ArrayWriteAxioms()

  int BeevMgr::SATBased_ArrayWriteRefinement(MINISAT::Solver& newS, const ASTNode& orig_input) {
    ASTNode writeAxiom;
    ASTNodeMap::iterator it = ReadOverWrite_NewName_Map.begin();
    ASTNodeMap::iterator itend = ReadOverWrite_NewName_Map.end();
    //int count = 0;
    //int num_write_axioms = ReadOverWrite_NewName_Map.size();

    ASTVec FalseAxioms, RemainingAxioms;
    FalseAxioms.push_back(ASTTrue);
    RemainingAxioms.push_back(ASTTrue);
    for(;it!=itend;it++) {
      //Guided refinement starts here
      ComputeFormulaMap.clear();      
      writeAxiom = Create_ArrayWriteAxioms(it->first,it->second);
      if(ASTFalse == ComputeFormulaUsingModel(writeAxiom)) {
	writeAxiom = TransformFormula(writeAxiom);
	FalseAxioms.push_back(writeAxiom);
      }
      else {
	writeAxiom = TransformFormula(writeAxiom);
	RemainingAxioms.push_back(writeAxiom);
      }
    }
      
    writeAxiom = (FalseAxioms.size() != 1) ? CreateNode(AND,FalseAxioms) : FalseAxioms[0];
    ASTNodeStats("adding false writeaxiom to SAT: ", writeAxiom);
    int res2 = CallSAT_ResultCheck(newS,writeAxiom,orig_input);
    if(2!=res2) {
      return res2;
    }
    
    writeAxiom = (RemainingAxioms.size() != 1) ? CreateNode(AND,RemainingAxioms) : RemainingAxioms[0];
    ASTNodeStats("adding remaining writeaxiom to SAT: ", writeAxiom);
    res2 = CallSAT_ResultCheck(newS,writeAxiom,orig_input);
    if(2!=res2) {
	return res2;
    }
    
    return 2;
  } //end of SATBased_ArrayWriteRefinement

  //Check result after calling SAT FIXME: Document arguments in
  //comments, and give them meaningful names.  How is anyone supposed
  //to know what "q" is?
  int BeevMgr::CallSAT_ResultCheck(MINISAT::Solver& newS, 
				   const ASTNode& q, const ASTNode& orig_input) {
    //Bitblast, CNF, call SAT now
    ASTNode BBFormula = BBForm(q);
    //ASTNodeStats("after bitblasting", BBFormula);    
    ClauseList *cllp = ToCNF(BBFormula);
    // if(stats && print_nodes) {
    //       cout << "\nClause list" << endl;
    //       PrintClauseList(cout, *cllp);
    //       cerr << "\n finished printing clauselist\n";
    //     }

    bool sat = toSATandSolve(newS,*cllp);    
    // Temporary debugging call.
    // CheckBBandCNF(newS, BBFormula);

    DeleteClauseList(cllp);
    if(!sat) {
      PrintOutput(true);
      return 1;
    }
    else if(newS.okay()) {
      CounterExampleMap.clear();
      ConstructCounterExample(newS);
      if (stats && print_nodes) {
	PrintSATModel(newS);
      }
      //check if the counterexample is good or not
      ComputeFormulaMap.clear();
      if(counterexample_checking_during_refinement)
	bvdiv_exception_occured = false;
      ASTNode orig_result = ComputeFormulaUsingModel(orig_input);
      if(!(ASTTrue == orig_result || ASTFalse == orig_result))
      	FatalError("TopLevelSat: Original input must compute to true or false against model");
      
//       if(!arrayread_refinement && !(ASTTrue == orig_result)) {
// 	print_counterexample = true;
// 	PrintCounterExample(true);
//       	FatalError("counterexample bogus : arrayread_refinement is switched off: " 
//       		   "EITHER all LA axioms have not been added OR bitblaster() or ToCNF()"
// 		   "or satsolver() or counterexamplechecker() have a bug");
//       }

      // if the counterexample is indeed a good one, then return
      // invalid
      if(ASTTrue == orig_result) {
	CheckCounterExample(newS.okay());
	PrintOutput(false);
	PrintCounterExample(newS.okay());
	PrintCounterExample_InOrder(newS.okay());
	return 0;	
      }
      // counterexample is bogus: flag it
      else {
	if(stats && print_nodes) {
	  cout << "Supposedly bogus one: \n";
	  bool tmp = print_counterexample;
	  print_counterexample = true;
	  PrintCounterExample(true);
	  print_counterexample = tmp;
	}

	return 2;
      }
    }
    else {
      PrintOutput(true);
      return -100;
    }
  } //end of CALLSAT_ResultCheck


  //FUNCTION: this function accepts a boolvector and returns a BVConst   
  ASTNode BeevMgr::BoolVectoBVConst(hash_map<unsigned,bool> * w, unsigned int l) {    
    unsigned len = w->size();
    if(l < len)
      FatalError("BoolVectorBVConst : length of bitvector does not match hash_map size:",ASTUndefined,l);
    std::string cc;
    for(unsigned int jj = 0; jj < l; jj++) {
      if((*w)[jj] == true)
	cc += '1';
      else if((*w)[jj] == false)
	cc += '0';
      else 
	cc += '0';
    }
    return CreateBVConst(cc.c_str(),2);
  }

  void BeevMgr::PrintActivityLevels_Of_SATVars(char * init_msg, MINISAT::Solver& newS) {
    if(!print_sat_varorder)
      return;

    ASTtoSATMap::iterator itbegin = _ASTNode_to_SATVar.begin();   
    ASTtoSATMap::iterator itend = _ASTNode_to_SATVar.end();
   
    cout << init_msg;
    cout << ": Printing activity levels of variables\n";
    for(ASTtoSATMap::iterator it=itbegin;it!=itend;it++){
      cout << (it->second) << "  :  ";
      (it->first).PL_Print(cout,0);
      cout << "   :   ";
      cout << newS.returnActivity(it->second) << endl;
    }
  }

  //this function biases the activity levels of MINISAT variables.
  void BeevMgr::ChangeActivityLevels_Of_SATVars(MINISAT::Solver& newS) {
    if(!variable_activity_optimize)
      return;

    ASTtoSATMap::iterator itbegin = _ASTNode_to_SATVar.begin();   
    ASTtoSATMap::iterator itend = _ASTNode_to_SATVar.end();
   
    unsigned int index=1;
    double base = 2;
    for(ASTtoSATMap::iterator it=itbegin;it!=itend;it++){
      ASTNode n = it->first;

      if(BVGETBIT == n.GetKind() || NOT == n.GetKind()) {
      	if(BVGETBIT == n.GetKind())
      	  index = GetUnsignedConst(n[1]);
      	else if (NOT == n.GetKind() && BVGETBIT == n[0].GetKind())
      	  index = GetUnsignedConst(n[0][1]);
      	else 
      	  index = 0;
	double initial_activity = pow(base,(double)index);
	newS.updateInitialActivity(it->second,initial_activity);
      } 
      else {
	double initial_activity = pow(base,pow(base,(double)index));
	newS.updateInitialActivity(it->second,initial_activity);	
      }    
    }
  }

  //This function prints the output of the STP solver
  void BeevMgr::PrintOutput(bool true_iff_valid) {
    //self-explanatory
    if(true_iff_valid) {
      ValidFlag = true;
      if(print_output) {
	if(smtlib_parser_enable)
	  cout << "unsat\n";
	else
	  cout << "Valid.\n";
      }
    }
    else {
      ValidFlag = false;
      if(print_output) {
	if(smtlib_parser_enable)
	  cout << "sat\n";
	else
	  cout << "Invalid.\n";
      }
    }
  }
} //end of namespace BEEV
