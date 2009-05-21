/********************************************************************
 * AUTHORS: Vijay Ganesh, David L. Dill
 *
 * BEGIN DATE: November, 2005
 *
 * LICENSE: Please view LICENSE file in the home dir of this Program
 ********************************************************************/
// -*- c++ -*-
#include "c_interface.h"

#include <cstdlib>
#include <cassert>
#include <ostream>
#include <iostream>
#include "fdstream.h"
#include "../AST/AST.h"

//These typedefs lower the effort of using the keyboard to type (too
//many overloaded meanings of the word type)
typedef BEEV::ASTNode  node;
typedef BEEV::ASTNode* nodestar;
typedef BEEV::BeevMgr* bmstar;
typedef BEEV::ASTVec   nodelist;
typedef BEEV::CompleteCounterExample* CompleteCEStar;
BEEV::ASTVec *decls = NULL;
//vector<BEEV::ASTNode *> created_exprs;
bool cinterface_exprdelete_on = false;

void vc_setFlags(char c) {
  std::string helpstring = "Usage: stp [-option] [infile]\n\n";
  helpstring +=  "-r  : switch refinement off (optimizations are ON by default)\n";
  helpstring +=  "-w  : switch wordlevel solver off (optimizations are ON by default)\n";
  helpstring +=  "-a  : switch optimizations off (optimizations are ON by default)\n";
  helpstring +=  "-s  : print function statistics\n";
  helpstring +=  "-v  : print nodes \n";
  helpstring +=  "-c  : construct counterexample\n";
  helpstring +=  "-d  : check counterexample\n";
  helpstring +=  "-p  : print counterexample\n";
  helpstring +=  "-h  : help\n";
  
  switch(c) {
  case 'a' :
    BEEV::optimize = false;
    BEEV::wordlevel_solve = false;
    break;
  case 'b':
    BEEV::print_STPinput_back = true;
    break;
  case 'c':
    BEEV::construct_counterexample = true;
    break;
  case 'd':
    BEEV::construct_counterexample = true;
    BEEV::check_counterexample = true;
    break;
  case 'e':
    BEEV::variable_activity_optimize = true;
    break;
  case 'f':
    BEEV::smtlib_parser_enable = true;
    break;
  case 'h':
    cout << helpstring;
    BEEV::FatalError("");
    break;
  case 'l' :
    BEEV::linear_search = true;
    break;
  case 'n':
    BEEV::print_output = true;
    break;
  case 'p':
    BEEV::print_counterexample = true;
    break;
  case 'q':
    BEEV::print_arrayval_declaredorder = true;
    break;
  case 'r':
    BEEV::arrayread_refinement = false;
    break;
  case 's' :
    BEEV::stats = true;
    break;
  case 'u':
    BEEV::arraywrite_refinement = true;
    break;  
  case 'v' :
    BEEV::print_nodes = true;
    break;
  case 'w':
    BEEV::wordlevel_solve = false;
    break;
  case 'x':
    cinterface_exprdelete_on = true;
    break;
  case 'z':
    BEEV::print_sat_varorder = true;
    break;
  default:
    std::string s = "C_interface: vc_setFlags: Unrecognized commandline flag:\n";
    s += helpstring;
    BEEV::FatalError(s.c_str());
    break;
  }
}

//Create a validity Checker. This is the global BeevMgr
VC vc_createValidityChecker(void) {
  vc_setFlags('d');
#ifdef NATIVE_C_ARITH
#else
  CONSTANTBV::ErrCode c = CONSTANTBV::BitVector_Boot(); 
  if(0 != c) {
    cout << CONSTANTBV::BitVector_Error(c) << endl;
    return 0;
  }
#endif
  bmstar bm = new BEEV::BeevMgr();
  decls = new BEEV::ASTVec();
  //created_exprs.clear();
  return (VC)bm;
}

// Expr I/O
void vc_printExpr(VC vc, Expr e) {
  //do not print in lisp mode
  //bmstar b = (bmstar)vc;
  BEEV::ASTNode q = (*(nodestar)e);
  //   b->Begin_RemoveWrites = true;
  //   BEEV::ASTNode q = b->SimplifyFormula_TopLevel(*((nodestar)e),false);
  //   b->Begin_RemoveWrites = false;    
  q.PL_Print(cout);
}

void vc_printExprFile(VC vc, Expr e, int fd) {
  fdostream os(fd);
  ((nodestar)e)->PL_Print(os);
  //os.flush();
}

static void vc_printVarDeclsToStream(VC vc, ostream &os) {
  for(BEEV::ASTVec::iterator i = decls->begin(),iend=decls->end();i!=iend;i++) {
    node a = *i;
    switch(a.GetType()) {
    case BEEV::BITVECTOR_TYPE:
      a.PL_Print(os);
      os << " : BITVECTOR(" << a.GetValueWidth() << ");" << endl;
      break;
    case BEEV::ARRAY_TYPE:
      a.PL_Print(os);
      os << " : ARRAY " << "BITVECTOR(" << a.GetIndexWidth() << ") OF ";
      os << "BITVECTOR(" << a.GetValueWidth() << ");" << endl;
      break;
    case BEEV::BOOLEAN_TYPE:
      a.PL_Print(os);
      os << " : BOOLEAN;" << endl;
      break;
    default:
      BEEV::FatalError("vc_printDeclsToStream: Unsupported type",a);
      break;
    }
  }
}

void vc_printVarDecls(VC vc) {
  vc_printVarDeclsToStream(vc, cout);
}

static void vc_printAssertsToStream(VC vc, ostream &os, int simplify_print) {
  bmstar b = (bmstar)vc;
  BEEV::ASTVec v = b->GetAsserts();
  for(BEEV::ASTVec::iterator i=v.begin(),iend=v.end();i!=iend;i++) {
    b->Begin_RemoveWrites = true;
    BEEV::ASTNode q = (simplify_print == 1) ? b->SimplifyFormula_TopLevel(*i,false) : *i;
    q = (simplify_print == 1) ? b->SimplifyFormula_TopLevel(q,false) : q;
    b->Begin_RemoveWrites = false;
    os << "ASSERT( ";
    q.PL_Print(os);
    os << ");" << endl;
  }
}

void vc_printAsserts(VC vc, int simplify_print) {
  vc_printAssertsToStream(vc, cout, simplify_print);
}

void vc_printQueryStateToBuffer(VC vc, Expr e, char **buf, unsigned long *len, int simplify_print){
  assert(vc);
  assert(e);
  assert(buf);
  assert(len);
  bmstar b = (bmstar)vc;

  // formate the state of the query
  stringstream os;
  vc_printVarDeclsToStream(vc, os);
  os << "%----------------------------------------------------" << endl;
  vc_printAssertsToStream(vc, os, simplify_print);
  os << "%----------------------------------------------------" << endl;
  os << "QUERY( ";
  b->Begin_RemoveWrites = true;
  BEEV::ASTNode q = (simplify_print == 1) ? b->SimplifyFormula_TopLevel(*((nodestar)e),false) : *(nodestar)e;
  b->Begin_RemoveWrites = false;    
  q.PL_Print(os);
  os << " );" << endl;

  // convert to a c buffer
  string s = os.str();
  const char *cstr = s.c_str();
  unsigned long size = s.size() + 1; // number of chars + terminating null
  *buf = (char *)malloc(size);
  if (!(*buf)) {
    fprintf(stderr, "malloc(%lu) failed.", size);
    assert(*buf);
  }
  *len = size;
  memcpy(*buf, cstr, size);
}

void vc_printCounterExampleToBuffer(VC vc, char **buf, unsigned long *len) {
  assert(vc);
  assert(buf);
  assert(len);
  bmstar b = (bmstar)vc;

  // formate the state of the query
  std::ostringstream os;
  BEEV::print_counterexample = true;
  os << "COUNTEREXAMPLE BEGIN: \n";
  b->PrintCounterExample(true,os);
  os << "COUNTEREXAMPLE END: \n";

  // convert to a c buffer
  string s = os.str();
  const char *cstr = s.c_str();
  unsigned long size = s.size() + 1; // number of chars + terminating null
  *buf = (char *)malloc(size);
  if (!(*buf)) {
    fprintf(stderr, "malloc(%lu) failed.", size);
    assert(*buf);
  }
  *len = size;
  memcpy(*buf, cstr, size);
}

void vc_printExprToBuffer(VC vc, Expr e, char **buf, unsigned long * len) {
  stringstream os;
  //bmstar b = (bmstar)vc;
  BEEV::ASTNode q = *((nodestar)e);
  // b->Begin_RemoveWrites = true;
  //   BEEV::ASTNode q = b->SimplifyFormula_TopLevel(*((nodestar)e),false);
  //   b->Begin_RemoveWrites = false;    
  q.PL_Print(os);
  //((nodestar)e)->PL_Print(os);
  string s = os.str();
  const char * cstr = s.c_str();
  unsigned long size = s.size() + 1; // number of chars + terminating null
  *buf = (char *)malloc(size);
  *len = size;
  memcpy(*buf, cstr, size);
}

void vc_printQuery(VC vc){
  ostream& os = std::cout;
  bmstar b = (bmstar)vc;
  os << "QUERY(";
  //b->Begin_RemoveWrites = true;
  //BEEV::ASTNode q = b->SimplifyFormula_TopLevel(b->GetQuery(),false);
  BEEV::ASTNode q = b->GetQuery();
  //b->Begin_RemoveWrites = false;    
  q.PL_Print(os);
  // b->GetQuery().PL_Print(os);
  os << ");" << endl;
}

/////////////////////////////////////////////////////////////////////////////
// Array-related methods                                                   //
/////////////////////////////////////////////////////////////////////////////
//! Create an array type
Type vc_arrayType(VC vc, Type typeIndex, Type typeData) {
  bmstar b = (bmstar)vc;
  nodestar ti = (nodestar)typeIndex;
  nodestar td = (nodestar)typeData;

  if(!(ti->GetKind() == BEEV::BITVECTOR && (*ti)[0].GetKind() == BEEV::BVCONST))
    BEEV::FatalError("Tyring to build array whose indextype i is not a BITVECTOR, where i = ",*ti);
  if(!(td->GetKind()  == BEEV::BITVECTOR && (*td)[0].GetKind() == BEEV::BVCONST))
    BEEV::FatalError("Trying to build an array whose valuetype v is not a BITVECTOR. where a = ",*td);
  nodestar output = new node(b->CreateNode(BEEV::ARRAY,(*ti)[0],(*td)[0]));
  //if(cinterface_exprdelete_on) created_exprs.push_back(output);
  return (Type)output;
}

//! Create an expression for the value of array at the given index
Expr vc_readExpr(VC vc, Expr array, Expr index) {
  bmstar b = (bmstar)vc;
  nodestar a = (nodestar)array;
  nodestar i = (nodestar)index;
  
  b->BVTypeCheck(*a);
  b->BVTypeCheck(*i);
  node o = b->CreateTerm(BEEV::READ,a->GetValueWidth(),*a,*i);
  b->BVTypeCheck(o);

  nodestar output = new node(o);
  //if(cinterface_exprdelete_on) created_exprs.push_back(output);
  return output;
}

// //! Array update; equivalent to "array WITH [index] := newValue"
Expr vc_writeExpr(VC vc, Expr array, Expr index, Expr newValue) {
  bmstar b = (bmstar)vc;
  nodestar a = (nodestar)array;
  nodestar i = (nodestar)index;
  nodestar n = (nodestar)newValue;

  b->BVTypeCheck(*a);
  b->BVTypeCheck(*i);
  b->BVTypeCheck(*n);
  node o = b->CreateTerm(BEEV::WRITE,a->GetValueWidth(),*a,*i,*n);
  o.SetIndexWidth(a->GetIndexWidth());
  b->BVTypeCheck(o);

  nodestar output = new node(o);
  //if(cinterface_exprdelete_on) created_exprs.push_back(output);
  return output;
}

/////////////////////////////////////////////////////////////////////////////
// Context-related methods                                                 //
/////////////////////////////////////////////////////////////////////////////
//! Assert a new formula in the current context.  
/*! The formula must have Boolean type. */
void vc_assertFormula(VC vc, Expr e) {
  nodestar a = (nodestar)e;
  bmstar b = (bmstar)vc;

  if(!BEEV::is_Form_kind(a->GetKind()))
    BEEV::FatalError("Trying to assert a NON formula: ",*a);

  b->BVTypeCheck(*a);
  b->AddAssert(*a);
}

//! Check validity of e in the current context.
/*!  If the result is true, then the resulting context is the same as
 * the starting context.  If the result is false, then the resulting
 * context is a context in which e is false.  e must have Boolean
 * type. */
int vc_query(VC vc, Expr e) {
  nodestar a = (nodestar)e;
  bmstar b = (bmstar)vc;

 if(!BEEV::is_Form_kind(a->GetKind()))
    BEEV::FatalError("CInterface: Trying to QUERY a NON formula: ",*a);

  b->BVTypeCheck(*a);
  b->AddQuery(*a);

  const BEEV::ASTVec v = b->GetAsserts();
  node o;
  if(!v.empty()) {
    if(v.size()==1)
      return b->TopLevelSAT(v[0],*a);
    else 
      return b->TopLevelSAT(b->CreateNode(BEEV::AND,v),*a);
  }
  else
    return b->TopLevelSAT(b->CreateNode(BEEV::TRUE),*a);
}

void vc_push(VC vc) {
  bmstar b = (bmstar)vc;
  b->ClearAllCaches();
  b->Push();
}

void vc_pop(VC vc) {
  bmstar b = (bmstar)vc;
  b->Pop();
}

void vc_printCounterExample(VC vc) {
  bmstar b = (bmstar)vc;
  BEEV::print_counterexample = true;    
  cout << "COUNTEREXAMPLE BEGIN: \n";
  b->PrintCounterExample(true);
  cout << "COUNTEREXAMPLE END: \n";
}

// //! Return the counterexample after a failed query.
// /*! This method should only be called after a query which returns
//  * false.  It will try to return the simplest possible set of
//  * assertions which are sufficient to make the queried expression
//  * false.  The caller is responsible for freeing the array when
//  * finished with it.
//  */

Expr vc_getCounterExample(VC vc, Expr e) {
  nodestar a = (nodestar)e;
  bmstar b = (bmstar)vc;    

  bool t = false;
  if(b->CounterExampleSize())
    t = true;
  nodestar output = new node(b->GetCounterExample(t, *a));  
  //if(cinterface_exprdelete_on) created_exprs.push_back(output);
  return output;
}

int vc_counterexample_size(VC vc) {
  bmstar b = (bmstar)vc;
  return b->CounterExampleSize();
}

WholeCounterExample vc_getWholeCounterExample(VC vc) {
  bmstar b = (bmstar)vc;
  CompleteCEStar c = 
    new BEEV::CompleteCounterExample(b->GetCompleteCounterExample(), b);
  return c;
}

Expr vc_getTermFromCounterExample(VC vc, Expr e, CompleteCEStar cc) {
  //bmstar b = (bmstar)vc;
  nodestar n = (nodestar)e;
  CompleteCEStar c = (CompleteCEStar)cc;

  nodestar output = new node(c->GetCounterExample(*n));
  return output;
}

int vc_getBVLength(VC vc, Expr ex) {
  nodestar e = (nodestar)ex;

  if(BEEV::BITVECTOR_TYPE != e->GetType()) {
    BEEV::FatalError("c_interface: vc_GetBVLength: Input expression must be a bit-vector");
  }

  return e->GetValueWidth();
} // end of vc_getBVLength

/////////////////////////////////////////////////////////////////////////////
// Expr Creation methods                                                   //
/////////////////////////////////////////////////////////////////////////////
//! Create a variable with a given name and type 
/*! The type cannot be a function type. */
Expr vc_varExpr1(VC vc, char* name, 
		int indexwidth, int valuewidth) {
  bmstar b = (bmstar)vc;

  node o = b->CreateSymbol(name);
  o.SetIndexWidth(indexwidth);
  o.SetValueWidth(valuewidth);
  
  nodestar output = new node(o);
  ////if(cinterface_exprdelete_on) created_exprs.push_back(output);
  b->BVTypeCheck(*output);

  //store the decls in a vector for printing purposes
  decls->push_back(o);
  return output;
}

Expr vc_varExpr(VC vc, char * name, Type type) {
  bmstar b = (bmstar)vc;
  nodestar a = (nodestar)type;

  node o = b->CreateSymbol(name);
  switch(a->GetKind()) {
  case BEEV::BITVECTOR:
    o.SetIndexWidth(0);
    o.SetValueWidth(GetUnsignedConst((*a)[0]));
    break;
  case BEEV::ARRAY:
    o.SetIndexWidth(GetUnsignedConst((*a)[0]));
    o.SetValueWidth(GetUnsignedConst((*a)[1]));
    break;
  case BEEV::BOOLEAN:
    o.SetIndexWidth(0);
    o.SetValueWidth(0);
    break;
  default:
    BEEV::FatalError("CInterface: vc_varExpr: Unsupported type",*a);
    break;
  }
  nodestar output = new node(o);
  ////if(cinterface_exprdelete_on) created_exprs.push_back(output);
  b->BVTypeCheck(*output);

  //store the decls in a vector for printing purposes
  decls->push_back(o);
  return output;
}

//! Create an equality expression.  The two children must have the
//same type.
Expr vc_eqExpr(VC vc, Expr ccc0, Expr ccc1) {
  bmstar b = (bmstar)vc;

  nodestar a = (nodestar)ccc0;
  nodestar aa = (nodestar)ccc1;
  b->BVTypeCheck(*a);
  b->BVTypeCheck(*aa);
  node o = b->CreateNode(BEEV::EQ,*a,*aa);

  nodestar output = new node(o);
  //if(cinterface_exprdelete_on) created_exprs.push_back(output);
  return output;
}

Expr vc_boolType(VC vc) {
  bmstar b = (bmstar)vc;

  node o = b->CreateNode(BEEV::BOOLEAN);
  nodestar output = new node(o);
  //if(cinterface_exprdelete_on) created_exprs.push_back(output);
  return output;
}

/////////////////////////////////////////////////////////////////////////////
// BOOLEAN EXPR Creation methods                                           //
/////////////////////////////////////////////////////////////////////////////
// The following functions create Boolean expressions.  The children
// provided as arguments must be of type Boolean.
Expr vc_trueExpr(VC vc) {
  bmstar b = (bmstar)vc;
  node c = b->CreateNode(BEEV::TRUE);
  
  nodestar d = new node(c);
  //if(cinterface_exprdelete_on) created_exprs.push_back(d);
  return d;
}

Expr vc_falseExpr(VC vc) {
  bmstar b = (bmstar)vc;
  node c = b->CreateNode(BEEV::FALSE);
  
  nodestar d = new node(c);
  //if(cinterface_exprdelete_on) created_exprs.push_back(d);
  return d;
}

Expr vc_notExpr(VC vc, Expr ccc) {
  bmstar b = (bmstar)vc;
  nodestar a = (nodestar)ccc;
  
  node o = b->CreateNode(BEEV::NOT,*a);
  b->BVTypeCheck(o);

  nodestar output = new node(o);
  //if(cinterface_exprdelete_on) created_exprs.push_back(output);
  return output;
}

Expr vc_andExpr(VC vc, Expr left, Expr right) {
  bmstar b = (bmstar)vc;
  nodestar l = (nodestar)left;
  nodestar r = (nodestar)right;
  
  node o = b->CreateNode(BEEV::AND,*l,*r);
  b->BVTypeCheck(o);

  nodestar output = new node(o);
  //if(cinterface_exprdelete_on) created_exprs.push_back(output);
  return output;
}

Expr vc_orExpr(VC vc, Expr left, Expr right) {
  bmstar b = (bmstar)vc;
  nodestar l = (nodestar)left;
  nodestar r = (nodestar)right;

  node o = b->CreateNode(BEEV::OR,*l,*r);
  b->BVTypeCheck(o);
  nodestar output = new node(o);
  //if(cinterface_exprdelete_on) created_exprs.push_back(output);
  return output;
}

Expr vc_andExprN(VC vc, Expr* cc, int n) {
  bmstar b = (bmstar)vc;
  nodestar * c = (nodestar *)cc;
  nodelist d;
  
  for(int i =0; i < n; i++)
    d.push_back(*c[i]);
  
  node o = b->CreateNode(BEEV::AND,d);
  b->BVTypeCheck(o);

  nodestar output = new node(o);
  //if(cinterface_exprdelete_on) created_exprs.push_back(output);
  return output;
}


Expr vc_orExprN(VC vc, Expr* cc, int n) {
  bmstar b = (bmstar)vc;
  nodestar * c = (nodestar *)cc;
  nodelist d;
  
  for(int i =0; i < n; i++)
    d.push_back(*c[i]);
  
  node o = b->CreateNode(BEEV::OR,d);
  b->BVTypeCheck(o);

  nodestar output = new node(o);
  //if(cinterface_exprdelete_on) created_exprs.push_back(output);
  return output;
}

Expr vc_iteExpr(VC vc, Expr cond, Expr thenpart, Expr elsepart){
  bmstar b = (bmstar)vc;
  nodestar c = (nodestar)cond;
  nodestar t = (nodestar)thenpart;
  nodestar e = (nodestar)elsepart;
  
  b->BVTypeCheck(*c);
  b->BVTypeCheck(*t);
  b->BVTypeCheck(*e);
  node o;
  //if the user asks for a formula then produce a formula, else
  //prodcue a term
  if(BEEV::BOOLEAN_TYPE == t->GetType())
    o = b->CreateNode(BEEV::ITE,*c,*t,*e);
  else {
    o = b->CreateTerm(BEEV::ITE,t->GetValueWidth(),*c,*t,*e);
    o.SetIndexWidth(t->GetIndexWidth());
  }
  b->BVTypeCheck(o);
  nodestar output = new node(o);
  //if(cinterface_exprdelete_on) created_exprs.push_back(output);
  return output;
}

Expr vc_impliesExpr(VC vc, Expr antecedent, Expr consequent){
  bmstar b = (bmstar)vc;
  nodestar c = (nodestar)antecedent;
  nodestar t = (nodestar)consequent;
  
  b->BVTypeCheck(*c);
  b->BVTypeCheck(*t);
  node o;

  o = b->CreateNode(BEEV::IMPLIES,*c,*t);
  b->BVTypeCheck(o);
  nodestar output = new node(o);
  //if(cinterface_exprdelete_on) created_exprs.push_back(output);
  return output;
}

Expr vc_iffExpr(VC vc, Expr e0, Expr e1){
  bmstar b = (bmstar)vc;
  nodestar c = (nodestar)e0;
  nodestar t = (nodestar)e1;
  
  b->BVTypeCheck(*c);
  b->BVTypeCheck(*t);
  node o;

  o = b->CreateNode(BEEV::IFF,*c,*t);
  b->BVTypeCheck(o);
  nodestar output = new node(o);
  //if(cinterface_exprdelete_on) created_exprs.push_back(output);
  return output;
}

Expr vc_boolToBVExpr(VC vc, Expr form) {
  bmstar b = (bmstar)vc;
  nodestar c = (nodestar)form;
  
  b->BVTypeCheck(*c);
  if(!is_Form_kind(c->GetKind()))
    BEEV::FatalError("CInterface: vc_BoolToBVExpr: You have input a NON formula:",*c);
  
  node o;
  node one = b->CreateOneConst(1); 
  node zero = b->CreateZeroConst(1);  
  o = b->CreateTerm(BEEV::ITE,1,*c,one,zero);

  b->BVTypeCheck(o);
  nodestar output = new node(o);
  //if(cinterface_exprdelete_on) created_exprs.push_back(output);
  return output;
}

/////////////////////////////////////////////////////////////////////////////
// BITVECTOR EXPR Creation methods                                         //
/////////////////////////////////////////////////////////////////////////////
Type vc_bvType(VC vc, int num_bits) {
  bmstar b = (bmstar)vc;
  
  if(!(0 < num_bits))
    BEEV::FatalError("CInterface: number of bits in a bvtype must be a positive integer:", 
		     b->CreateNode(BEEV::UNDEFINED));

  node e = b->CreateBVConst(32, num_bits);
  nodestar output = new node(b->CreateNode(BEEV::BITVECTOR,e));
  //if(cinterface_exprdelete_on) created_exprs.push_back(output);
  return output;
}

Type vc_bv32Type(VC vc) {
  return vc_bvType(vc,32);
}


Expr vc_bvConstExprFromStr(VC vc, char* binary_repr) {
  bmstar b = (bmstar)vc;

  node n = b->CreateBVConst(binary_repr,2);
  b->BVTypeCheck(n);
  nodestar output = new node(n);
  //if(cinterface_exprdelete_on) created_exprs.push_back(output);
  return output;
}

Expr vc_bvConstExprFromInt(VC vc,
			   int n_bits, 
			   unsigned int value) {
  bmstar b = (bmstar)vc;

  unsigned long long int v = (unsigned long long int)value;
  node n = b->CreateBVConst(n_bits, v);
  b->BVTypeCheck(n);
  nodestar output = new node(n);
  //if(cinterface_exprdelete_on) created_exprs.push_back(output);
  return output;
}

Expr vc_bvConstExprFromLL(VC vc,
			  int n_bits, 
			  unsigned long long value) {
  bmstar b = (bmstar)vc;
  
  node n = b->CreateBVConst(n_bits, value);
  b->BVTypeCheck(n);
  nodestar output = new node(n);
  //if(cinterface_exprdelete_on) created_exprs.push_back(output);
  return output;
}

Expr vc_bvConcatExpr(VC vc, Expr left, Expr right) {
  bmstar b = (bmstar)vc;
  nodestar l = (nodestar)left;
  nodestar r = (nodestar)right;

  b->BVTypeCheck(*l);
  b->BVTypeCheck(*r);
  node o =
    b->CreateTerm(BEEV::BVCONCAT,
		  l->GetValueWidth()+ r->GetValueWidth(),*l,*r);
  b->BVTypeCheck(o);
  nodestar output = new node(o);
  //if(cinterface_exprdelete_on) created_exprs.push_back(output);
  return output;
}

Expr vc_bvPlusExpr(VC vc, int n_bits, Expr left, Expr right){
  bmstar b = (bmstar)vc;
  nodestar l = (nodestar)left;
  nodestar r = (nodestar)right;

  b->BVTypeCheck(*l);
  b->BVTypeCheck(*r);
  node o = b->CreateTerm(BEEV::BVPLUS,n_bits, *l, *r);
  b->BVTypeCheck(o);
  nodestar output = new node(o);
  //if(cinterface_exprdelete_on) created_exprs.push_back(output);
  return output;
}


Expr vc_bv32PlusExpr(VC vc, Expr left, Expr right) {
  return vc_bvPlusExpr(vc, 32, left, right);
}


Expr vc_bvMinusExpr(VC vc, int n_bits, Expr left, Expr right) {
  bmstar b = (bmstar)vc;
  nodestar l = (nodestar)left;
  nodestar r = (nodestar)right;

  b->BVTypeCheck(*l);
  b->BVTypeCheck(*r);
  node o = b->CreateTerm(BEEV::BVSUB,n_bits, *l, *r);
  b->BVTypeCheck(o);
  nodestar output = new node(o);
  //if(cinterface_exprdelete_on) created_exprs.push_back(output);
  return output;
}


Expr vc_bv32MinusExpr(VC vc, Expr left, Expr right) {
  return vc_bvMinusExpr(vc, 32, left, right);
}


Expr vc_bvMultExpr(VC vc, int n_bits, Expr left, Expr right) {
  bmstar b = (bmstar)vc;
  nodestar l = (nodestar)left;
  nodestar r = (nodestar)right;

  b->BVTypeCheck(*l);
  b->BVTypeCheck(*r);
  node o = b->CreateTerm(BEEV::BVMULT,n_bits, *l, *r);
  b->BVTypeCheck(o);
  nodestar output = new node(o);
  //if(cinterface_exprdelete_on) created_exprs.push_back(output);
  return output;
}

Expr vc_bvDivExpr(VC vc, int n_bits, Expr left, Expr right) {
  bmstar b = (bmstar)vc;
  nodestar l = (nodestar)left;
  nodestar r = (nodestar)right;

  b->BVTypeCheck(*l);
  b->BVTypeCheck(*r);
  node o = b->CreateTerm(BEEV::BVDIV,n_bits, *l, *r);
  b->BVTypeCheck(o);
  nodestar output = new node(o);
  //if(cinterface_exprdelete_on) created_exprs.push_back(output);
  return output;
}

Expr vc_bvModExpr(VC vc, int n_bits, Expr left, Expr right) {
  bmstar b = (bmstar)vc;
  nodestar l = (nodestar)left;
  nodestar r = (nodestar)right;

  b->BVTypeCheck(*l);
  b->BVTypeCheck(*r);
  node o = b->CreateTerm(BEEV::BVMOD,n_bits, *l, *r);
  b->BVTypeCheck(o);
  nodestar output = new node(o);
  //if(cinterface_exprdelete_on) created_exprs.push_back(output);
  return output;
}

Expr vc_sbvDivExpr(VC vc, int n_bits, Expr left, Expr right) {
  bmstar b = (bmstar)vc;
  nodestar l = (nodestar)left;
  nodestar r = (nodestar)right;

  b->BVTypeCheck(*l);
  b->BVTypeCheck(*r);
  node o = b->CreateTerm(BEEV::SBVDIV,n_bits, *l, *r);
  b->BVTypeCheck(o);
  nodestar output = new node(o);
  //if(cinterface_exprdelete_on) created_exprs.push_back(output);
  return output;
}

Expr vc_sbvModExpr(VC vc, int n_bits, Expr left, Expr right) {
  bmstar b = (bmstar)vc;
  nodestar l = (nodestar)left;
  nodestar r = (nodestar)right;

  b->BVTypeCheck(*l);
  b->BVTypeCheck(*r);
  node o = b->CreateTerm(BEEV::SBVMOD,n_bits, *l, *r);
  b->BVTypeCheck(o);
  nodestar output = new node(o);
  //if(cinterface_exprdelete_on) created_exprs.push_back(output);
  return output;
}

Expr vc_bv32MultExpr(VC vc, Expr left, Expr right) {
  return vc_bvMultExpr(vc, 32, left, right);
}


// unsigned comparators
Expr vc_bvLtExpr(VC vc, Expr left, Expr right) {
  bmstar b = (bmstar)vc;
  nodestar l = (nodestar)left;
  nodestar r = (nodestar)right;

  b->BVTypeCheck(*l);
  b->BVTypeCheck(*r);
  node o = b->CreateNode(BEEV::BVLT, *l, *r);
  b->BVTypeCheck(o);
  nodestar output = new node(o);
  //if(cinterface_exprdelete_on) created_exprs.push_back(output);
  return output;
}

Expr vc_bvLeExpr(VC vc, Expr left, Expr right) {
  bmstar b = (bmstar)vc;
  nodestar l = (nodestar)left;
  nodestar r = (nodestar)right;

  b->BVTypeCheck(*l);
  b->BVTypeCheck(*r);
  node o = b->CreateNode(BEEV::BVLE, *l, *r);
  b->BVTypeCheck(o);
  nodestar output = new node(o);
  //if(cinterface_exprdelete_on) created_exprs.push_back(output);
  return output;
}

Expr vc_bvGtExpr(VC vc, Expr left, Expr right) {
  bmstar b = (bmstar)vc;
  nodestar l = (nodestar)left;
  nodestar r = (nodestar)right;

  b->BVTypeCheck(*l);
  b->BVTypeCheck(*r);
  node o = b->CreateNode(BEEV::BVGT, *l, *r);
  b->BVTypeCheck(o);
  nodestar output = new node(o);
  //if(cinterface_exprdelete_on) created_exprs.push_back(output);
  return output;
}

Expr vc_bvGeExpr(VC vc, Expr left, Expr right) {
  bmstar b = (bmstar)vc;
  nodestar l = (nodestar)left;
  nodestar r = (nodestar)right;

  b->BVTypeCheck(*l);
  b->BVTypeCheck(*r);
  node o = b->CreateNode(BEEV::BVGE, *l, *r);
  b->BVTypeCheck(o);
  nodestar output = new node(o);
  //if(cinterface_exprdelete_on) created_exprs.push_back(output);
  return output;
}

// signed comparators
Expr vc_sbvLtExpr(VC vc, Expr left, Expr right) {
  bmstar b = (bmstar)vc;
  nodestar l = (nodestar)left;
  nodestar r = (nodestar)right;

  b->BVTypeCheck(*l);
  b->BVTypeCheck(*r);
  node o = b->CreateNode(BEEV::BVSLT, *l, *r);
  b->BVTypeCheck(o);
  nodestar output = new node(o);
  //if(cinterface_exprdelete_on) created_exprs.push_back(output);
  return output;
}

Expr vc_sbvLeExpr(VC vc, Expr left, Expr right) {
  bmstar b = (bmstar)vc;
  nodestar l = (nodestar)left;
  nodestar r = (nodestar)right;

  b->BVTypeCheck(*l);
  b->BVTypeCheck(*r);
  node o = b->CreateNode(BEEV::BVSLE, *l, *r);
  b->BVTypeCheck(o);
  nodestar output = new node(o);
  //if(cinterface_exprdelete_on) created_exprs.push_back(output);
  return output;
}

Expr vc_sbvGtExpr(VC vc, Expr left, Expr right) {
  bmstar b = (bmstar)vc;
  nodestar l = (nodestar)left;
  nodestar r = (nodestar)right;

  b->BVTypeCheck(*l);
  b->BVTypeCheck(*r);
  node o = b->CreateNode(BEEV::BVSGT, *l, *r);
  b->BVTypeCheck(o);
  nodestar output = new node(o);
  //if(cinterface_exprdelete_on) created_exprs.push_back(output);
  return output;
}

Expr vc_sbvGeExpr(VC vc, Expr left, Expr right) {
  bmstar b = (bmstar)vc;
  nodestar l = (nodestar)left;
  nodestar r = (nodestar)right;

  b->BVTypeCheck(*l);
  b->BVTypeCheck(*r);
  node o = b->CreateNode(BEEV::BVSGE, *l, *r);
  b->BVTypeCheck(o);
  nodestar output = new node(o);
  //if(cinterface_exprdelete_on) created_exprs.push_back(output);
  return output;
}

Expr vc_bvUMinusExpr(VC vc, Expr ccc) {
  bmstar b = (bmstar)vc;
  nodestar a = (nodestar)ccc;
  b->BVTypeCheck(*a);

  node o = b->CreateTerm(BEEV::BVUMINUS, a->GetValueWidth(), *a);
  b->BVTypeCheck(o);
  nodestar output = new node(o);
  //if(cinterface_exprdelete_on) created_exprs.push_back(output);
  return output;
}

// bitwise operations: these are terms not formulas
Expr vc_bvAndExpr(VC vc, Expr left, Expr right) {
  bmstar b = (bmstar)vc;
  nodestar l = (nodestar)left;
  nodestar r = (nodestar)right;

  b->BVTypeCheck(*l);
  b->BVTypeCheck(*r);
  node o = b->CreateTerm(BEEV::BVAND, (*l).GetValueWidth(), *l, *r);
  b->BVTypeCheck(o);
  nodestar output = new node(o);
  //if(cinterface_exprdelete_on) created_exprs.push_back(output);
  return output;
}

Expr vc_bvOrExpr(VC vc, Expr left, Expr right) {
  bmstar b = (bmstar)vc;
  nodestar l = (nodestar)left;
  nodestar r = (nodestar)right;

  b->BVTypeCheck(*l);
  b->BVTypeCheck(*r);
  node o = b->CreateTerm(BEEV::BVOR, (*l).GetValueWidth(), *l, *r);
  b->BVTypeCheck(o);
  nodestar output = new node(o);
  //if(cinterface_exprdelete_on) created_exprs.push_back(output);
  return output;
}

Expr vc_bvXorExpr(VC vc, Expr left, Expr right) {
  bmstar b = (bmstar)vc;
  nodestar l = (nodestar)left;
  nodestar r = (nodestar)right;

  b->BVTypeCheck(*l);
  b->BVTypeCheck(*r);
  node o = b->CreateTerm(BEEV::BVXOR, (*l).GetValueWidth(), *l, *r);
  b->BVTypeCheck(o);
  nodestar output = new node(o);
  //if(cinterface_exprdelete_on) created_exprs.push_back(output);
  return output;
}

Expr vc_bvNotExpr(VC vc, Expr ccc) {
  bmstar b = (bmstar)vc;
  nodestar a = (nodestar)ccc;

  b->BVTypeCheck(*a);
  node o = b->CreateTerm(BEEV::BVNEG, a->GetValueWidth(), *a);
  b->BVTypeCheck(o);
  nodestar output = new node(o);
  //if(cinterface_exprdelete_on) created_exprs.push_back(output);
  return output;
}

Expr vc_bvLeftShiftExpr(VC vc, int sh_amt, Expr ccc) {
  bmstar b = (bmstar)vc;
  nodestar a = (nodestar)ccc;
  b->BVTypeCheck(*a);

  //convert leftshift to bvconcat
  if(0 != sh_amt) {
    node len = b->CreateBVConst(sh_amt, 0);
    node o = b->CreateTerm(BEEV::BVCONCAT, a->GetValueWidth() + sh_amt, *a, len);
    b->BVTypeCheck(o);
    nodestar output = new node(o);
    //if(cinterface_exprdelete_on) created_exprs.push_back(output);
    return output;
  }
  else
    return a;
}

Expr vc_bvRightShiftExpr(VC vc, int sh_amt, Expr ccc) {
  bmstar b = (bmstar)vc;
  nodestar a = (nodestar)ccc;
  b->BVTypeCheck(*a);
  
  unsigned int w = a->GetValueWidth();  
  //the amount by which you are rightshifting
  //is less-than/equal-to the length of input
  //bitvector  
  if(0 < (unsigned)sh_amt && (unsigned)sh_amt <= w) {
    node len = b->CreateBVConst(sh_amt, 0);
    node hi = b->CreateBVConst(32,w-1);
    node low = b->CreateBVConst(32,sh_amt);
    node extract = b->CreateTerm(BEEV::BVEXTRACT,w-sh_amt,*a,hi,low);

    node n = b->CreateTerm(BEEV::BVCONCAT, w,len, extract);
    b->BVTypeCheck(n);
    nodestar output = new node(n);
    //if(cinterface_exprdelete_on) created_exprs.push_back(output);
    return output;
  }
  else if(sh_amt == 0)
    return a;
  else {
    if(0== w)
      BEEV::FatalError("CInterface: vc_bvRightShiftExpr: cannot have a bitvector of length 0:",*a);
    nodestar output = new node(b->CreateBVConst(w,0));
    //if(cinterface_exprdelete_on) created_exprs.push_back(output);
    return output;
  }
}

/* Same as vc_bvLeftShift only that the answer in 32 bits long */
Expr vc_bv32LeftShiftExpr(VC vc, int sh_amt, Expr child) {
  return vc_bvExtract(vc, vc_bvLeftShiftExpr(vc, sh_amt, child), 31, 0);
}

/* Same as vc_bvRightShift only that the answer in 32 bits long */
Expr vc_bv32RightShiftExpr(VC vc, int sh_amt, Expr child) {
  return vc_bvExtract(vc, vc_bvRightShiftExpr(vc, sh_amt, child), 31, 0);
}


Expr vc_bvVar32LeftShiftExpr(VC vc, Expr sh_amt, Expr child) {
  Expr ifpart;
  Expr thenpart;
  Expr elsepart = vc_trueExpr(vc);
  Expr ite = vc_trueExpr(vc);

  for(int count=32; count >= 0; count--){
    if(count != 32) {
      ifpart = vc_eqExpr(vc, sh_amt, 
			 vc_bvConstExprFromInt(vc, 32, count));
      thenpart = vc_bvExtract(vc,
			      vc_bvLeftShiftExpr(vc, count, child),
			      31, 0);

      ite = vc_iteExpr(vc,ifpart,thenpart,elsepart);
      elsepart = ite;
    } 
    else
      elsepart = vc_bvConstExprFromInt(vc,32, 0);    
  }  
  return ite;  
}

Expr vc_bvVar32DivByPowOfTwoExpr(VC vc, Expr child, Expr rhs) {
  Expr ifpart;
  Expr thenpart;
  Expr elsepart = vc_trueExpr(vc);
  Expr ite = vc_trueExpr(vc);

  for(int count=32; count >= 0; count--){
    if(count != 32) {
      ifpart = vc_eqExpr(vc, rhs, 
			 vc_bvConstExprFromInt(vc, 32, 1 << count));      
      thenpart = vc_bvRightShiftExpr(vc, count, child);      
      ite = vc_iteExpr(vc,ifpart,thenpart,elsepart);
      elsepart = ite;
    } else {
      elsepart = vc_bvConstExprFromInt(vc,32, 0);
    }    
  }  
  return ite;  
}

Expr vc_bvVar32RightShiftExpr(VC vc, Expr sh_amt, Expr child) {
  Expr ifpart;
  Expr thenpart;
  Expr elsepart = vc_trueExpr(vc);
  Expr ite = vc_trueExpr(vc);

  for(int count=32; count >= 0; count--){
    if(count != 32) {
      ifpart = vc_eqExpr(vc, sh_amt, 
			 vc_bvConstExprFromInt(vc, 32, count));      
      thenpart = vc_bvRightShiftExpr(vc, count, child);      
      ite = vc_iteExpr(vc,ifpart,thenpart,elsepart);
      elsepart = ite;
    } else {
      elsepart = vc_bvConstExprFromInt(vc,32, 0);
    }    
  }  
  return ite;  
}

Expr vc_bvExtract(VC vc, Expr ccc, int hi_num, int low_num) {
  bmstar b = (bmstar)vc;
  nodestar a = (nodestar)ccc;
  b->BVTypeCheck(*a);

  node hi = b->CreateBVConst(32,hi_num);
  node low = b->CreateBVConst(32,low_num);
  node o = b->CreateTerm(BEEV::BVEXTRACT,hi_num-low_num+1,*a,hi,low);
  b->BVTypeCheck(o);
  nodestar output = new node(o);
  //if(cinterface_exprdelete_on) created_exprs.push_back(output);
  return output;
}

Expr vc_bvBoolExtract(VC vc, Expr ccc, int bit_num) {
  bmstar b = (bmstar)vc;
  nodestar a = (nodestar)ccc;
  b->BVTypeCheck(*a);

  node bit = b->CreateBVConst(32,bit_num);
  //node o = b->CreateNode(BEEV::BVGETBIT,*a,bit);  
  node zero = b->CreateBVConst(1,0);
  node oo = b->CreateTerm(BEEV::BVEXTRACT,1,*a,bit,bit);
  node o = b->CreateNode(BEEV::EQ,oo,zero);
  b->BVTypeCheck(o);
  nodestar output = new node(o);
  //if(cinterface_exprdelete_on) created_exprs.push_back(output);
  return output; 
}

Expr vc_bvSignExtend(VC vc, Expr ccc, int nbits) {
  bmstar b = (bmstar)vc;
  nodestar a = (nodestar)ccc;
  
  //width of the expr which is being sign extended. nbits is the
  //resulting length of the signextended expr
  b->BVTypeCheck(*a);
  
  unsigned exprlen = a->GetValueWidth();
  unsigned outputlen = nbits;
  node n;
  if(exprlen >= outputlen) {
    //extract
    node hi = b->CreateBVConst(32,outputlen-1);
    node low = b->CreateBVConst(32,0);
    n = b->CreateTerm(BEEV::BVEXTRACT,nbits,*a,hi,low);
    b->BVTypeCheck(n);
  }
  else {
    //sign extend
    BEEV::ASTNode width = b->CreateBVConst(32,nbits);
    n = b->CreateTerm(BEEV::BVSX,nbits,*a, width);
  }

  b->BVTypeCheck(n);
  nodestar output = new node(n);
  //if(cinterface_exprdelete_on) created_exprs.push_back(output);
  return output;
}

//! Return an int from a constant bitvector expression
int getBVInt(Expr e) {
  //bmstar b = (bmstar)vc;
  nodestar a = (nodestar)e;

  if(BEEV::BVCONST != a->GetKind())
    BEEV::FatalError("CInterface: getBVInt: Attempting to extract int value from a NON-constant BITVECTOR: ",*a);
  return (int)GetUnsignedConst(*a);
}

//! Return an unsigned int from a constant bitvector expression
unsigned int getBVUnsigned(Expr e) {
  //bmstar b = (bmstar)vc;
  nodestar a = (nodestar)e;

  if(BEEV::BVCONST != a->GetKind())
    BEEV::FatalError("getBVUnsigned: Attempting to extract int value from a NON-constant BITVECTOR: ",*a);
  return (unsigned int)GetUnsignedConst(*a);
}

//! Return an unsigned long long int from a constant bitvector expression
unsigned long long int getBVUnsignedLongLong(Expr e) {
  //bmstar b = (bmstar)vc;
  nodestar a = (nodestar)e;

  if(BEEV::BVCONST != a->GetKind())
    BEEV::FatalError("getBVUnsigned: Attempting to extract int value from a NON-constant BITVECTOR: ",*a);
#ifdef NATIVE_C_ARITH
  return (unsigned long long int)a->GetBVConst();
#else
  unsigned* bv = a->GetBVConst();

  char * str_bv  = (char *)CONSTANTBV::BitVector_to_Bin(bv);
  unsigned long long int tmp = strtoull(str_bv,NULL,2);
  CONSTANTBV::BitVector_Dispose((unsigned char *)str_bv);
  return tmp;
#endif
}


Expr vc_simplify(VC vc, Expr e) {
  bmstar b = (bmstar)vc;
  nodestar a = (nodestar)e;

  if(BEEV::BOOLEAN_TYPE == a->GetType()) {
    nodestar output = new node(b->SimplifyFormula_TopLevel(*a,false));
    //if(cinterface_exprdelete_on) created_exprs.push_back(output);
    b->Begin_RemoveWrites = true;
    output = new node(b->SimplifyFormula_TopLevel(*output,false));
    b->Begin_RemoveWrites = false;
    return output;    
  }
  else {
    nodestar output = new node(b->SimplifyTerm(*a));
    //if(cinterface_exprdelete_on) created_exprs.push_back(output);
    b->Begin_RemoveWrites = true;
    output = new node(b->SimplifyTerm(*output));
    b->Begin_RemoveWrites = false;
    return output;
  }
}

/* C pointer support: C interface to support C memory arrays in CVCL */
Expr vc_bvCreateMemoryArray(VC vc, char * arrayName) {
  Type bv8  = vc_bvType(vc,8);
  Type bv32 = vc_bvType(vc,32);
  
  Type malloced_mem0 = vc_arrayType(vc,bv32,bv8);
  return vc_varExpr(vc, arrayName, malloced_mem0);
}

Expr vc_bvReadMemoryArray(VC vc, 
			  Expr array, 
			  Expr byteIndex, int numOfBytes) {
  if(!(numOfBytes > 0))
    BEEV::FatalError("numOfBytes must be greater than 0");

  if(numOfBytes == 1)
    return vc_readExpr(vc,array,byteIndex);
  else {
    int count = 1;
    Expr a = vc_readExpr(vc,array,byteIndex);
    while(--numOfBytes > 0) {
      Expr b = vc_readExpr(vc,array,
			   /*vc_simplify(vc, */
				       vc_bvPlusExpr(vc, 32, 
						     byteIndex,
						     vc_bvConstExprFromInt(vc,32,count)))/*)*/;
      a = vc_bvConcatExpr(vc,b,a);
      count++;
    }
    return a;
  }    
}

Expr vc_bvWriteToMemoryArray(VC vc, 
			     Expr array, Expr byteIndex, 
			     Expr element, int numOfBytes) {
  if(!(numOfBytes > 0))
    BEEV::FatalError("numOfBytes must be greater than 0");
	    
  int newBitsPerElem = numOfBytes*8;
  if(numOfBytes == 1)
    return vc_writeExpr(vc, array, byteIndex, element);
  else {
    int count = 1;
    int hi = newBitsPerElem - 1;
    int low = newBitsPerElem - 8;
    int low_elem = 0;
    int hi_elem = low_elem + 7;
    Expr c = vc_bvExtract(vc, element, hi_elem, low_elem);
    Expr newarray = vc_writeExpr(vc, array, byteIndex, c);
    while(--numOfBytes > 0) {
      hi = low-1;
      low = low-8;      

      low_elem = low_elem + 8;
      hi_elem = low_elem + 7;

      c = vc_bvExtract(vc, element, hi_elem, low_elem);
      newarray = 
	vc_writeExpr(vc, newarray,
		     vc_bvPlusExpr(vc, 32, byteIndex, vc_bvConstExprFromInt(vc,32,count)),
		     c);
      count++;
    }
    return newarray;
  }    
}

Expr vc_bv32ConstExprFromInt(VC vc, unsigned int value){
  return vc_bvConstExprFromInt(vc, 32, value);
}


#if 0
static char *val_to_binary_str(unsigned nbits, unsigned long long val) {
        char s[65];

	assert(nbits < sizeof s);
        strcpy(s, "");
        while(nbits-- > 0) {
                if((val >> nbits) & 1)
                        strcat(s, "1");
                else
                        strcat(s, "0");
        }
        return strdup(s);
}
#endif

char* exprString(Expr e){
  stringstream ss;
  ((nodestar)e)->PL_Print(ss,0);
  string s = ss.str();
  char *copy = strdup(s.c_str());
  return copy;
}

char* typeString(Type t){
  stringstream ss;
  ((nodestar)t)->PL_Print(ss,0);

  string s = ss.str();
  char *copy = strdup(s.c_str());
  return copy;
}

Expr getChild(Expr e, int i){
  nodestar a = (nodestar)e;

  BEEV::ASTVec c = a->GetChildren();
  if(0 <=  (unsigned)i && (unsigned)i < c.size()) {
    BEEV::ASTNode o = c[i];
    nodestar output = new node(o);
    //if(cinterface_exprdelete_on) created_exprs.push_back(output);
    return output;
  }
  else 
    BEEV::FatalError("getChild: Error accessing childNode in expression: ",*a);
  return a;
}

void vc_registerErrorHandler(void (*error_hdlr)(const char* err_msg)) {
  BEEV::vc_error_hdlr = error_hdlr;
}


int vc_getHashQueryStateToBuffer(VC vc, Expr query) {
  assert(vc);
  assert(query);
  bmstar b = (bmstar)vc;
  nodestar qry = (nodestar)query;
  BEEV::ASTVec v = b->GetAsserts(); 
  BEEV::ASTNode out = b->CreateNode(BEEV::AND,b->CreateNode(BEEV::NOT,*qry),v);
  return out.Hash();
}

Type vc_getType(VC vc, Expr ex) {
  nodestar e = (nodestar)ex;

  switch(e->GetType()) {
  case BEEV::BOOLEAN_TYPE:
    return vc_boolType(vc);
    break;      
  case BEEV::BITVECTOR_TYPE:
    return vc_bvType(vc,e->GetValueWidth());
    break;
  case BEEV::ARRAY_TYPE: {
    Type typeindex = vc_bvType(vc,e->GetIndexWidth());
    Type typedata = vc_bvType(vc,e->GetValueWidth());
    return vc_arrayType(vc,typeindex,typedata);
    break;
  }
  default:
    BEEV::FatalError("c_interface: vc_GetType: expression with bad typing: please check your expression construction");
    return vc_boolType(vc);
    break;
  }
}// end of vc_gettype()

//!if e is TRUE then return 1; if e is FALSE then return 0; otherwise
//return -1
int vc_isBool(Expr e) {
  nodestar input = (nodestar)e;
  if(BEEV::TRUE == input->GetKind()) {
    return 1;
  }

  if(BEEV::FALSE == input->GetKind()) {
    return 0;
  }

  return -1;
}

void vc_Destroy(VC vc) {
  bmstar b = (bmstar)vc;
  // for(std::vector<BEEV::ASTNode *>::iterator it=created_exprs.begin(),
  // 	itend=created_exprs.end();it!=itend;it++) {
  //     BEEV::ASTNode * aaa = *it;
  //     delete aaa;
  //   }
  delete decls;
  delete b;
}

void vc_DeleteExpr(Expr e) {
  nodestar input = (nodestar)e;
  //bmstar b = (bmstar)vc;
  delete input;
}

exprkind_t getExprKind(Expr e) {
  nodestar input = (nodestar)e;
  return (exprkind_t)(input->GetKind());  
}

int getDegree (Expr e) {
  nodestar input = (nodestar)e;
  return input->Degree();
}

int getBVLength(Expr ex) {
  nodestar e = (nodestar)ex;

  if(BEEV::BITVECTOR_TYPE != e->GetType()) {
    BEEV::FatalError("c_interface: vc_GetBVLength: Input expression must be a bit-vector");
  }

  return e->GetValueWidth();
} 

type_t getType (Expr ex) {
  nodestar e = (nodestar)ex;
    
  return (type_t)(e->GetType());
}

int getVWidth (Expr ex) {
  nodestar e = (nodestar)ex;

  return e->GetValueWidth();
}

int getIWidth (Expr ex) {
  nodestar e = (nodestar)ex;

  return e->GetIndexWidth();
}

void vc_printCounterExampleFile(VC vc, int fd) {
  fdostream os(fd);
  bmstar b = (bmstar)vc;
  BEEV::print_counterexample = true;    
  os << "COUNTEREXAMPLE BEGIN: \n";
  b->PrintCounterExample(true, os);
  os << "COUNTEREXAMPLE END: \n";
}

const char* exprName(Expr e){
    return ((nodestar)e)->GetName();
}

int getExprID (Expr ex) {
    BEEV::ASTNode q = (*(nodestar)ex);

    return q.GetNodeNum();
}
