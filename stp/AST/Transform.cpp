/********************************************************************
 * AUTHORS: Vijay Ganesh, David L. Dill
 *
 * BEGIN DATE: November, 2005
 *
 * LICENSE: Please view LICENSE file in the home dir of this Program
 ********************************************************************/
// -*- c++ -*-

#include "AST.h"
#include <stdlib.h>
#include <stdio.h>
namespace BEEV {  
  
  //Translates signed BVDIV/BVMOD into unsigned variety
  ASTNode BeevMgr::TranslateSignedDivMod(const ASTNode& in) {
    if(!(SBVMOD == in.GetKind() || SBVDIV == in.GetKind())) {
      FatalError("TranslateSignedDivMod: input must be signed DIV/MOD\n",in);
    }

    ASTNode dividend = in[0];
    ASTNode divisor  = in[1];      
    unsigned len = in.GetValueWidth();
    if(SBVMOD == in.GetKind()) {
      //if(TopBit(dividend)==1) 
      //
      //then -BVMOD(-dividend,abs(divisor)) 
      //
      //else BVMOD(dividend,abs(divisor))

      //create the condition for the dividend
      ASTNode hi1  = CreateBVConst(32,len-1);
      ASTNode one  = CreateOneConst(1);
      ASTNode cond = CreateNode(EQ,one,CreateTerm(BVEXTRACT,1,dividend,hi1,hi1));

      //create the condition and conditional for the divisor
      ASTNode cond_divisor = CreateNode(EQ,one,CreateTerm(BVEXTRACT,1,divisor,hi1,hi1));
      ASTNode pos_divisor  = CreateTerm(ITE,len,cond_divisor,CreateTerm(BVUMINUS,len,divisor),divisor);

      //create the modulus term for each case
      ASTNode modnode = CreateTerm(BVMOD,len,dividend,pos_divisor);
      ASTNode minus_modnode = CreateTerm(BVMOD,len,CreateTerm(BVUMINUS,len,dividend),pos_divisor);
      minus_modnode = CreateTerm(BVUMINUS,len,minus_modnode);

      //put everything together, simplify, and return
      ASTNode n = CreateTerm(ITE,len,cond,minus_modnode,modnode);
      return SimplifyTerm_TopLevel(n);
    }
    
    //now handle the BVDIV case
    //if topBit(dividend) is 1 and topBit(divisor) is 0
    //
    //then output is -BVDIV(-dividend,divisor)
    //
    //elseif topBit(dividend) is 0 and topBit(divisor) is 1
    //
    //then output is -BVDIV(dividend,-divisor)
    //
    //elseif topBit(dividend) is 1 and topBit(divisor) is 1
    //
    // then output is BVDIV(-dividend,-divisor)
    //
    //else simply output BVDIV(dividend,divisor)
    ASTNode hi1 = CreateBVConst(32,len-1);
    ASTNode zero = CreateZeroConst(1);
    ASTNode one = CreateOneConst(1);
    ASTNode divnode = CreateTerm(BVDIV, len, dividend, divisor);

    ASTNode cond1 = CreateNode(AND,
			       CreateNode(EQ,zero,CreateTerm(BVEXTRACT,1,dividend,hi1,hi1)),
			       CreateNode(EQ,one, CreateTerm(BVEXTRACT,1,divisor,hi1,hi1)));
    ASTNode minus_divnode1 = CreateTerm(BVDIV,len,
					dividend,
					CreateTerm(BVUMINUS,len,divisor));
    minus_divnode1 = CreateTerm(BVUMINUS,len,minus_divnode1);

    ASTNode cond2 = CreateNode(AND,
			       CreateNode(EQ,one,CreateTerm(BVEXTRACT,1,dividend,hi1,hi1)),
			       CreateNode(EQ,zero,CreateTerm(BVEXTRACT,1,divisor,hi1,hi1)));
    ASTNode minus_divnode2 = CreateTerm(BVDIV,len,
					CreateTerm(BVUMINUS,len,dividend),
					divisor);
    minus_divnode2 = CreateTerm(BVUMINUS,len,minus_divnode2);

    ASTNode cond3 = CreateNode(AND,
			       CreateNode(EQ,one,CreateTerm(BVEXTRACT,1,dividend,hi1,hi1)),
			       CreateNode(EQ,one,CreateTerm(BVEXTRACT,1,divisor,hi1,hi1)));
    ASTNode minus_divnode3 = CreateTerm(BVDIV,len,
					CreateTerm(BVUMINUS,len,dividend),
					CreateTerm(BVUMINUS,len,divisor));
    ASTNode n = CreateTerm(ITE,len,
			   cond1,
			   minus_divnode1,
			   CreateTerm(ITE,len,
				      cond2,
				      minus_divnode2,
				      CreateTerm(ITE,len,
						 cond3,
						 minus_divnode3,
						 divnode)));
  return SimplifyTerm_TopLevel(n);
  }//end of TranslateSignedDivMod()

  ASTNode BeevMgr::TransformFormula(const ASTNode& form) {
    ASTNode result;

    ASTNode simpleForm = form;
    Kind k = simpleForm.GetKind();
    if(!(is_Form_kind(k) && BOOLEAN_TYPE == simpleForm.GetType())) {
      //FIXME: "You have inputted a NON-formula"?
      FatalError("TransformFormula: You have input a NON-formula",simpleForm);
    }

    ASTNodeMap::iterator iter;
    if((iter = TransformMap.find(simpleForm)) != TransformMap.end())
      return iter->second;

    switch(k) {
    case TRUE:
    case FALSE: {
      result = simpleForm;
      break;
    }
    case NOT: {
      ASTVec c;
      c.push_back(TransformFormula(simpleForm[0]));
      result = CreateNode(NOT,c);      
      break;
    }
    case BVLT:
    case BVLE:
    case BVGT:
    case BVGE:
    case BVSLT:
    case BVSLE:
    case BVSGT:
    case BVSGE:      
    case NEQ: {
      ASTVec c;
      c.push_back(TransformTerm(simpleForm[0]));      
      c.push_back(TransformTerm(simpleForm[1]));
      result = CreateNode(k,c);
      break;
    }
    case EQ: {
      ASTNode term1 = TransformTerm(simpleForm[0]);      
      ASTNode term2 = TransformTerm(simpleForm[1]);
      result = CreateSimplifiedEQ(term1,term2);     
      break;
    }
    case AND:
    case OR: 
    case NAND:
    case NOR:
    case IFF:
    case XOR:
    case ITE:
    case IMPLIES: {
      ASTVec vec;
      ASTNode o;
      for (ASTVec::const_iterator it = simpleForm.begin(),itend=simpleForm.end(); it != itend; it++){
	o = TransformFormula(*it);	
	vec.push_back(o);
      }

      result = CreateNode(k, vec);
      break;
    }
    default:
      if(k == SYMBOL && BOOLEAN_TYPE == simpleForm.GetType())
	result = simpleForm;      
      else {
	cerr << "The input is: " << simpleForm << endl;
	cerr << "The valuewidth of input is : " << simpleForm.GetValueWidth() << endl;
	FatalError("TransformFormula: Illegal kind: ",ASTUndefined, k);
      }
      break;    
    } 
    //BVTypeCheck(result);
    TransformMap[simpleForm] = result;
    return result;
  } //End of TransformFormula

  ASTNode BeevMgr::TransformTerm(const ASTNode& inputterm) {
    ASTNode result;
    ASTNode term = inputterm;

    Kind k = term.GetKind();
    if(!is_Term_kind(k))
      FatalError("TransformTerm: Illegal kind: You have input a nonterm:", inputterm, k);
    ASTNodeMap::iterator iter;
    if((iter = TransformMap.find(term)) != TransformMap.end())
      return iter->second;
    switch(k) {
    case SYMBOL: {
      // ASTNodeMap::iterator itsym;
//       if((itsym = CounterExampleMap.find(term)) != CounterExampleMap.end())	
//       	result = itsym->second;
//       else
	result = term;
      break;
    }
    case BVCONST:
      result = term;
      break;
    case WRITE:
      FatalError("TransformTerm: this kind is not supported",term);
      break;
    case READ:
      result = TransformArray(term);
      break;
    case ITE: {
      ASTNode cond = term[0];
      ASTNode thn  = term[1];
      ASTNode els  = term[2];
      cond = TransformFormula(cond);
      thn = TransformTerm(thn);
      els = TransformTerm(els);
      //result = CreateTerm(ITE,term.GetValueWidth(),cond,thn,els);
      result = CreateSimplifiedTermITE(cond,thn,els);
      result.SetIndexWidth(term.GetIndexWidth());
      break;
    }
    default: {
      ASTVec c = term.GetChildren();
      ASTVec::iterator it = c.begin();
      ASTVec::iterator itend = c.end();
      unsigned width = term.GetValueWidth();
      unsigned indexwidth = term.GetIndexWidth();
      ASTVec o;
      for(;it!=itend;it++) {
	o.push_back(TransformTerm(*it));
      }

      result = CreateTerm(k,width,o);
      result.SetIndexWidth(indexwidth);

      if(SBVDIV == result.GetKind() || SBVMOD == result.GetKind()) {
	result = TranslateSignedDivMod(result);
      }
      break;
    }
    }

    TransformMap[term] = result;
    if(term.GetValueWidth() != result.GetValueWidth())
      FatalError("TransformTerm: result and input terms are of different length", result);
    if(term.GetIndexWidth() != result.GetIndexWidth()) {
      cerr << "TransformTerm: input term is : " << term << endl;
      FatalError("TransformTerm: result and input terms have different index length", result);
    }
    return result;
  } //End of TransformTerm

  /* This function transforms Array Reads, Read over Writes, Read over
   * ITEs into flattened form.
   *
   * Transform1: Suppose there are two array reads in the input
   * Read(A,i) and Read(A,j) over the same array. Then Read(A,i) is
   * replaced with a symbolic constant, say v1, and Read(A,j) is
   * replaced with the following ITE:
   *
   * ITE(i=j,v1,v2)
   *
   * Transform2:
   * 
   * Transform3:
   */
  ASTNode BeevMgr::TransformArray(const ASTNode& term) {
    ASTNode result = term;

    unsigned int width = term.GetValueWidth();
    Kind k = term.GetKind();
    if (!is_Term_kind(k))
      FatalError("TransformArray: Illegal kind: You have input a nonterm:", ASTUndefined, k);
    ASTNodeMap::iterator iter;
    if((iter = TransformMap.find(term)) != TransformMap.end())
      return iter->second;

    switch(k) {
      //'term' is of the form READ(arrName, readIndex) 
    case READ: {
      ASTNode arrName = term[0];
      switch (arrName.GetKind()) {
      case SYMBOL: {
	/* input is of the form: READ(A, readIndex)
	 * 
	 * output is of the from: A1, if this is the first READ over A
         *                           
	 *                        ITE(previous_readIndex=readIndex,A1,A2)
	 *                        
         *                        .....
	 */

	//  Recursively transform read index, which may also contain reads.
	ASTNode readIndex = TransformTerm(term[1]);	
	ASTNode processedTerm = CreateTerm(READ,width,arrName,readIndex);
	
	//check if the 'processedTerm' has a corresponding ITE construct
	//already. if so, return it. else continue processing.
	ASTNodeMap::iterator it;
	if((it = _arrayread_ite.find(processedTerm)) != _arrayread_ite.end()) {
	  result = it->second;	
	  break;
	}
	//Constructing Symbolic variable corresponding to 'processedTerm'
	ASTNode CurrentSymbol;
	ASTNodeMap::iterator it1;
	// First, check if read index is constant and it has a constant value in the substitution map.
	if(CheckSubstitutionMap(processedTerm,CurrentSymbol)) {
	  _arrayread_symbol[processedTerm] = CurrentSymbol;
	}
	// Check if it already has an abstract variable.
	else if((it1 = _arrayread_symbol.find(processedTerm)) != _arrayread_symbol.end()) {
	  CurrentSymbol = it1->second;
	}
	else {
	  // Make up a new abstract variable.
	  // FIXME: Make this into a method (there already may BE a method) and
	  // get rid of the fixed-length buffer!
	  //build symbolic name corresponding to array read. The symbolic
	  //name has 2 components: stringname, and a count
	  const char * b = arrName.GetName();
	  std::string c(b);
	  char d[32];
	  sprintf(d,"%d",_symbol_count++);
	  std::string ccc(d);
	  c += "array_" + ccc;
	  
	  CurrentSymbol = CreateSymbol(c.c_str());
	  CurrentSymbol.SetValueWidth(processedTerm.GetValueWidth());
	  CurrentSymbol.SetIndexWidth(processedTerm.GetIndexWidth());
	  _arrayread_symbol[processedTerm] = CurrentSymbol;	  
	}
	
	//list of array-read indices corresponding to arrName, seen while
	//traversing the AST tree. we need this list to construct the ITEs
	// Dill: we hope to make this irrelevant.  Harmless for now.
	ASTVec readIndices = _arrayname_readindices[arrName];
	
	//construct the ITE structure for this array-read
	ASTNode ite = CurrentSymbol;
	_introduced_symbols.insert(CurrentSymbol);
	BVTypeCheck(ite);
	
	if(arrayread_refinement) {
	  // ite is really a variable here; it is an ite in the
	  // else-branch
	  result = ite;
	}
	else {
	  // Full Seshia transform if we're not doing read refinement.
	  //do not loop if the current readIndex is a BVCONST
	  // if(BVCONST == term[1].GetKind() && !SeenNonConstReadIndex && optimize) {
	  // 	    result = ite; 
	  // 	  }
	  // 	  else {	  
	    //else part: SET the SeenNonConstReadIndex var, and do the hard work
	    //SeenNonConstReadIndex = true;
	    ASTVec::reverse_iterator it2=readIndices.rbegin();
	    ASTVec::reverse_iterator it2end=readIndices.rend();
	    for(;it2!=it2end;it2++) {
	      ASTNode cond = CreateSimplifiedEQ(readIndex,*it2);
	      if(ASTFalse == cond)
		continue;
	      
	      ASTNode arrRead = CreateTerm(READ,width,arrName,*it2);
	      //Good idea to TypeCheck internally constructed nodes
	      BVTypeCheck(arrRead);
	      
	      ASTNode arrayreadSymbol = _arrayread_symbol[arrRead];
	      if(arrayreadSymbol.IsNull())
		FatalError("TransformArray:symbolic variable for processedTerm, p," 
			   "does not exist:p = ",arrRead);
	      ite = CreateSimplifiedTermITE(cond,arrayreadSymbol,ite);
	    }
	    result = ite;
	    //}
	}
	
	_arrayname_readindices[arrName].push_back(readIndex);	
	//save the ite corresponding to 'processedTerm'
	_arrayread_ite[processedTerm] = result;
	break;
      } //end of READ over a SYMBOL
      case WRITE:{	
	/* The input to this case is: READ((WRITE A i val) j)
	 *
	 * The output of this case is: ITE( (= i j) val (READ A i))
	 */
	
	/* 1. arrName or term[0] is infact a WRITE(A,i,val) expression
	 *
	 * 2. term[1] is the read-index j
	 *
	 * 3. arrName[0] is the new arrName i.e. A. A can be either a
              SYMBOL or a nested WRITE. no other possibility
	 *
	 * 4. arrName[1] is the WRITE index i.e. i
	 *
	 * 5. arrName[2] is the WRITE value i.e. val (val can inturn
	 *    be an array read)
	 */
	ASTNode readIndex = TransformTerm(term[1]);
	ASTNode writeIndex = TransformTerm(arrName[1]);
	ASTNode writeVal = TransformTerm(arrName[2]);
	
	if(!(SYMBOL == arrName[0].GetKind() || 
	     WRITE == arrName[0].GetKind())) 
	  FatalError("TransformArray: An array write is being attempted on a non-array:",term);
	if(ARRAY_TYPE != arrName[0].GetType())
	  FatalError("TransformArray: An array write is being attempted on a non-array:",term);
	
	ASTNode cond = CreateSimplifiedEQ(writeIndex,readIndex);
	//TypeCheck internally created node
	BVTypeCheck(cond);
	ASTNode readTerm = CreateTerm(READ,width,arrName[0],readIndex);
	//TypeCheck internally created node
	BVTypeCheck(readTerm);
	ASTNode readPushedIn = TransformArray(readTerm);
	//TypeCheck internally created node
	BVTypeCheck(readPushedIn);
	//result = CreateTerm(ITE, arrName[0].GetValueWidth(),cond,writeVal,readPushedIn);
	result = CreateSimplifiedTermITE(cond,writeVal,readPushedIn);

	//Good idea to typecheck terms created inside the system
	BVTypeCheck(result);
	break;
      } //end of READ over a WRITE
      case ITE: {
	/* READ((ITE cond thn els) j) 
	 *
	 * is transformed into
	 *
	 * (ITE cond (READ thn j) (READ els j))
	 */
	
	//(ITE cond thn els)
	ASTNode term0 = term[0];
	//READINDEX j
	ASTNode j = TransformTerm(term[1]);
	
	ASTNode cond = term0[0];
	//first array 
	ASTNode t01  = term0[1];
	//second array
	ASTNode t02  = term0[2];
	
	cond = TransformFormula(cond);
	ASTNode thn = TransformTerm(t01);
	ASTNode els = TransformTerm(t02);
	
	if(!(t01.GetValueWidth() == t02.GetValueWidth() &&
	     t01.GetValueWidth() == thn.GetValueWidth() &&
	     t01.GetValueWidth() == els.GetValueWidth()))
	  FatalError("TransformArray: length of THENbranch != length of ELSEbranch in the term t = \n",term);

	if(!(t01.GetIndexWidth() == t02.GetIndexWidth() &&
	     t01.GetIndexWidth() == thn.GetIndexWidth() &&
	     t01.GetIndexWidth() == els.GetIndexWidth()))
	  FatalError("TransformArray: length of THENbranch != length of ELSEbranch in the term t = \n",term);

	//(READ thn j)
	ASTNode thnRead = CreateTerm(READ,width,thn,j);
	BVTypeCheck(thnRead);
	thnRead = TransformArray(thnRead);
	
	//(READ els j)
	ASTNode elsRead = CreateTerm(READ,width,els,j);
	BVTypeCheck(elsRead);
	elsRead = TransformArray(elsRead);
	
	//(ITE cond (READ thn j) (READ els j))
	result = CreateSimplifiedTermITE(cond,thnRead,elsRead);
	BVTypeCheck(result);
      break;
      }
      default:
	FatalError("TransformArray: The READ is NOT over SYMBOL/WRITE/ITE",term);
	break;
      } 
      break;
    } //end of READ switch
    default:
      FatalError("TransformArray: input term is of wrong kind: ",ASTUndefined);
      break;
    }
    
    TransformMap[term] = result;
    return result;
  } //end of TransformArray()  
} //end of namespace BEEV
