/********************************************************************
 * AUTHORS: Vijay Ganesh, David L. Dill
 *
 * BEGIN DATE: November, 2005
 *
 * License to use, copy, modify, sell and/or distribute this software
 * and its documentation for any purpose is hereby granted without
 * royalty, subject to the terms and conditions defined in the \ref
 * LICENSE file provided with this distribution.  In particular:
 *
 * - The above copyright notice and this permission notice must appear
 * in all copies of the software and related documentation.
 *
 * - THE SOFTWARE IS PROVIDED "AS-IS", WITHOUT ANY WARRANTIES,
 * EXPRESSED OR IMPLIED.  USE IT AT YOUR OWN RISK.
 ********************************************************************/
// -*- c++ -*-
#ifndef _cvcl__include__c_interface_h_
#define _cvcl__include__c_interface_h_

#ifdef __cplusplus
extern "C" {
#endif
  
#ifdef STP_STRONG_TYPING
#else
  //This gives absolutely no pointer typing at compile-time. Most C
  //users prefer this over stronger typing. User is the king. A
  //stronger typed interface is in the works.
  typedef void* VC;
  typedef void* Expr;
  typedef void* Type;
  typedef void* WholeCounterExample;
#endif

  // o  : optimizations
  // c  : check counterexample
  // p  : print counterexample
  // h  : help
  // s  : stats
  // v  : print nodes
  void vc_setFlags(char c);
  
  //! Flags can be NULL
  VC vc_createValidityChecker(void);
  
  // Basic types
  Type vc_boolType(VC vc);
  
  //! Create an array type
  Type vc_arrayType(VC vc, Type typeIndex, Type typeData);

  /////////////////////////////////////////////////////////////////////////////
  // Expr manipulation methods                                               //
  /////////////////////////////////////////////////////////////////////////////

  //! Create a variable with a given name and type 
  /*! The type cannot be a function type. The var name can contain
    only variables, numerals and underscore. If you use any other
    symbol, you will get a segfault. */  
  Expr vc_varExpr(VC vc, char * name, Type type);

  //The var name can contain only variables, numerals and
  //underscore. If you use any other symbol, you will get a segfault.
  Expr vc_varExpr1(VC vc, char* name, 
		  int indexwidth, int valuewidth);

  //! Get the expression and type associated with a name.
  /*!  If there is no such Expr, a NULL Expr is returned. */
  //Expr vc_lookupVar(VC vc, char* name, Type* type);
  
  //! Get the type of the Expr.
  Type vc_getType(VC vc, Expr e);
  
  int vc_getBVLength(VC vc, Expr e);

  //! Create an equality expression.  The two children must have the same type.
  Expr vc_eqExpr(VC vc, Expr child0, Expr child1);
  
  // Boolean expressions
  
  // The following functions create Boolean expressions.  The children
  // provided as arguments must be of type Boolean (except for the
  // function vc_iteExpr(). In the case of vc_iteExpr() the
  // conditional must always be Boolean, but the ifthenpart
  // (resp. elsepart) can be bit-vector or Boolean type. But, the
  // ifthenpart and elsepart must be both of the same type)
  Expr vc_trueExpr(VC vc);
  Expr vc_falseExpr(VC vc);
  Expr vc_notExpr(VC vc, Expr child);
  Expr vc_andExpr(VC vc, Expr left, Expr right);
  Expr vc_andExprN(VC vc, Expr* children, int numOfChildNodes);
  Expr vc_orExpr(VC vc, Expr left, Expr right);
  Expr vc_orExprN(VC vc, Expr* children, int numOfChildNodes);
  Expr vc_impliesExpr(VC vc, Expr hyp, Expr conc);
  Expr vc_iffExpr(VC vc, Expr left, Expr right);
  //The output type of vc_iteExpr can be Boolean (formula-level ite)
  //or bit-vector (word-level ite)
  Expr vc_iteExpr(VC vc, Expr conditional, Expr ifthenpart, Expr elsepart);
  
  //Boolean to single bit BV Expression
  Expr vc_boolToBVExpr(VC vc, Expr form);

  // Arrays
  
  //! Create an expression for the value of array at the given index
  Expr vc_readExpr(VC vc, Expr array, Expr index);

  //! Array update; equivalent to "array WITH [index] := newValue"
  Expr vc_writeExpr(VC vc, Expr array, Expr index, Expr newValue);
  
  // Expr I/O
  //! Expr vc_parseExpr(VC vc, char* s);

  //! Prints 'e' to stdout.
  void vc_printExpr(VC vc, Expr e);

  //! Prints 'e' into an open file descriptor 'fd'
  void vc_printExprFile(VC vc, Expr e, int fd);

  //! Prints state of 'vc' into malloc'd buffer '*buf' and stores the 
  //  length into '*len'.  It is the responsibility of the caller to 
  //  free the buffer.
  //void vc_printStateToBuffer(VC vc, char **buf, unsigned long *len);

  //! Prints 'e' to malloc'd buffer '*buf'.  Sets '*len' to the length of 
  //  the buffer. It is the responsibility of the caller to free the buffer.
  void vc_printExprToBuffer(VC vc, Expr e, char **buf, unsigned long * len);

  //! Prints counterexample to stdout.
  void vc_printCounterExample(VC vc);

  //! Prints variable declarations to stdout.
  void vc_printVarDecls(VC vc);

  //! Prints asserts to stdout. The flag simplify_print must be set to
  //"1" if you wish simplification to occur dring printing. It must be
  //set to "0" otherwise
  void vc_printAsserts(VC vc, int simplify_print);

  //! Prints the state of the query to malloc'd buffer '*buf' and
  //stores ! the length of the buffer to '*len'.  It is the
  //responsibility of the caller to free the buffer. The flag
  //simplify_print must be set to "1" if you wish simplification to
  //occur dring printing. It must be set to "0" otherwise
  void vc_printQueryStateToBuffer(VC vc, Expr e, 
				  char **buf, unsigned long *len, int simplify_print);

  //! Similar to vc_printQueryStateToBuffer()
  void vc_printCounterExampleToBuffer(VC vc, char **buf,unsigned long *len);

  //! Prints query to stdout.
  void vc_printQuery(VC vc);

  /////////////////////////////////////////////////////////////////////////////
  // Context-related methods                                                 //
  /////////////////////////////////////////////////////////////////////////////
  
  //! Assert a new formula in the current context.  
  /*! The formula must have Boolean type. */
  void vc_assertFormula(VC vc, Expr e);
  
  //! Simplify e with respect to the current context
  Expr vc_simplify(VC vc, Expr e);

  //! Check validity of e in the current context. e must be a FORMULA
  //
  //if returned 0 then input is INVALID. 
  //
  //if returned 1 then input is VALID
  //
  //if returned 2 then ERROR
  int vc_query(VC vc, Expr e);
  
  //! Return the counterexample after a failed query.
  Expr vc_getCounterExample(VC vc, Expr e);

  //! get size of counterexample, i.e. the number of variables/array
  //locations in the counterexample.
  int vc_counterexample_size(VC vc);
  
  //! Checkpoint the current context and increase the scope level
  void vc_push(VC vc);
  
  //! Restore the current context to its state at the last checkpoint
  void vc_pop(VC vc);
  
  //! Return an int from a constant bitvector expression
  int getBVInt(Expr e);
  //! Return an unsigned int from a constant bitvector expression
  unsigned int getBVUnsigned(Expr e);
  //! Return an unsigned long long int from a constant bitvector expressions
  unsigned long long int getBVUnsignedLongLong(Expr e);
  
  /**************************/
  /* BIT VECTOR OPERATIONS  */
  /**************************/
  Type vc_bvType(VC vc, int no_bits);
  Type vc_bv32Type(VC vc);
  
  Expr vc_bvConstExprFromStr(VC vc, char* binary_repr);
  Expr vc_bvConstExprFromInt(VC vc, int n_bits, unsigned int value);
  Expr vc_bvConstExprFromLL(VC vc, int n_bits, unsigned long long value);
  Expr vc_bv32ConstExprFromInt(VC vc, unsigned int value);
  
  Expr vc_bvConcatExpr(VC vc, Expr left, Expr right);
  Expr vc_bvPlusExpr(VC vc, int n_bits, Expr left, Expr right);
  Expr vc_bv32PlusExpr(VC vc, Expr left, Expr right);
  Expr vc_bvMinusExpr(VC vc, int n_bits, Expr left, Expr right);
  Expr vc_bv32MinusExpr(VC vc, Expr left, Expr right);
  Expr vc_bvMultExpr(VC vc, int n_bits, Expr left, Expr right);
  Expr vc_bv32MultExpr(VC vc, Expr left, Expr right);
  // left divided by right i.e. left/right
  Expr vc_bvDivExpr(VC vc, int n_bits, Expr left, Expr right);
  // left modulo right i.e. left%right
  Expr vc_bvModExpr(VC vc, int n_bits, Expr left, Expr right);
  // signed left divided by right i.e. left/right
  Expr vc_sbvDivExpr(VC vc, int n_bits, Expr left, Expr right);
  // signed left modulo right i.e. left%right
  Expr vc_sbvModExpr(VC vc, int n_bits, Expr left, Expr right);
  
  Expr vc_bvLtExpr(VC vc, Expr left, Expr right);
  Expr vc_bvLeExpr(VC vc, Expr left, Expr right);
  Expr vc_bvGtExpr(VC vc, Expr left, Expr right);
  Expr vc_bvGeExpr(VC vc, Expr left, Expr right);
  
  Expr vc_sbvLtExpr(VC vc, Expr left, Expr right);
  Expr vc_sbvLeExpr(VC vc, Expr left, Expr right);
  Expr vc_sbvGtExpr(VC vc, Expr left, Expr right);
  Expr vc_sbvGeExpr(VC vc, Expr left, Expr right);
  
  Expr vc_bvUMinusExpr(VC vc, Expr child);

  // bitwise operations: these are terms not formulas  
  Expr vc_bvAndExpr(VC vc, Expr left, Expr right);
  Expr vc_bvOrExpr(VC vc, Expr left, Expr right);
  Expr vc_bvXorExpr(VC vc, Expr left, Expr right);
  Expr vc_bvNotExpr(VC vc, Expr child);
  
  Expr vc_bvLeftShiftExpr(VC vc, int sh_amt, Expr child);
  Expr vc_bvRightShiftExpr(VC vc, int sh_amt, Expr child);
  /* Same as vc_bvLeftShift only that the answer in 32 bits long */
  Expr vc_bv32LeftShiftExpr(VC vc, int sh_amt, Expr child);
  /* Same as vc_bvRightShift only that the answer in 32 bits long */
  Expr vc_bv32RightShiftExpr(VC vc, int sh_amt, Expr child);
  Expr vc_bvVar32LeftShiftExpr(VC vc, Expr sh_amt, Expr child);
  Expr vc_bvVar32RightShiftExpr(VC vc, Expr sh_amt, Expr child);
  Expr vc_bvVar32DivByPowOfTwoExpr(VC vc, Expr child, Expr rhs);

  Expr vc_bvExtract(VC vc, Expr child, int high_bit_no, int low_bit_no);
  
  //accepts a bitvector and position, and returns a boolean
  //corresponding to that position. More precisely, it return the
  //equation (x[bit_no:bit_no] = 0)
  //FIXME  = 1 ?
  Expr vc_bvBoolExtract(VC vc, Expr x, int bit_no);  
  Expr vc_bvSignExtend(VC vc, Expr child, int nbits);
  
  /*C pointer support:  C interface to support C memory arrays in CVCL */
  Expr vc_bvCreateMemoryArray(VC vc, char * arrayName);
  Expr vc_bvReadMemoryArray(VC vc, 
			  Expr array, Expr byteIndex, int numOfBytes);
  Expr vc_bvWriteToMemoryArray(VC vc, 
			       Expr array, Expr  byteIndex, 
			       Expr element, int numOfBytes);
  Expr vc_bv32ConstExprFromInt(VC vc, unsigned int value);
  
  // return a string representation of the Expr e. The caller is responsible
  // for deallocating the string with free()
  char* exprString(Expr e);
  
  // return a string representation of the Type t. The caller is responsible
  // for deallocating the string with free()
  char* typeString(Type t);

  Expr getChild(Expr e, int i);

  //1.if input expr is TRUE then the function returns 1;
  //
  //2.if input expr is FALSE then function returns 0;
  //
  //3.otherwise the function returns -1
  int vc_isBool(Expr e);

  /* Register the given error handler to be called for each fatal error.*/
  void vc_registerErrorHandler(void (*error_hdlr)(const char* err_msg));

  int vc_getHashQueryStateToBuffer(VC vc, Expr query);

  //destroys the STP instance, and removes all the created expressions
  void vc_Destroy(VC vc);

  //deletes the expression e
  void vc_DeleteExpr(Expr e);

  //Get the whole counterexample from the current context
  WholeCounterExample vc_getWholeCounterExample(VC vc);

  //Get the value of a term expression from the CounterExample
  Expr vc_getTermFromCounterExample(VC vc, Expr e, WholeCounterExample c);

  //Kinds of Expr
  enum exprkind_t{
      UNDEFINED,
      SYMBOL,
      BVCONST,
      BVNEG,
      BVCONCAT,
      BVOR,
      BVAND,
      BVXOR,
      BVNAND,
      BVNOR,
      BVXNOR,
      BVEXTRACT,
      BVLEFTSHIFT,
      BVRIGHTSHIFT,
      BVSRSHIFT,
      BVVARSHIFT,
      BVPLUS,
      BVSUB,
      BVUMINUS,
      BVMULTINVERSE,
      BVMULT,
      BVDIV,
      BVMOD,
      SBVDIV,
      SBVMOD,
      BVSX,
      BOOLVEC,
      ITE,
      BVGETBIT,
      BVLT,
      BVLE,
      BVGT,
      BVGE,
      BVSLT,
      BVSLE,
      BVSGT,
      BVSGE,
      EQ,
      NEQ,
      FALSE,
      TRUE,
      NOT,
      AND,
      OR,
      NAND,
      NOR,
      XOR,
      IFF,
      IMPLIES,
      READ,
      WRITE,
      ARRAY,
      BITVECTOR,
      BOOLEAN
  };

  // type of expression
  enum type_t {
      BOOLEAN_TYPE = 0,
      BITVECTOR_TYPE,
      ARRAY_TYPE,
      UNKNOWN_TYPE
  };

  // get the kind of the expression
  exprkind_t getExprKind (Expr e);

  // get the number of children nodes
  int getDegree (Expr e);

  // get the bv length
  int getBVLength(Expr e);

  // get expression type
  type_t getType (Expr e);

  // get value bit width
  int getVWidth (Expr e);

  // get index bit width
  int getIWidth (Expr e);

  // Prints counterexample to an open file descriptor 'fd'
  void vc_printCounterExampleFile(VC vc, int fd);

  // get name of expression. must be a variable.
  const char* exprName(Expr e);
  
  // get the node ID of an Expr.
  int getExprID (Expr ex);

#ifdef __cplusplus
}
#endif

#endif


