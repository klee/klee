//===-- SMTParser.cpp -----------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "SMTParser.h"

#include "klee/ExprBuilder.h"
#include "klee/Solver.h"
#include "klee/Constraints.h"
#include "expr/Parser.h"

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <cassert>
#include <stack>

//#define DEBUG

using namespace std;
using namespace klee;
using namespace klee::expr;

extern int smtlibparse(); 
extern void *smtlib_createBuffer(int);
extern void smtlib_deleteBuffer(void *);
extern void smtlib_switchToBuffer(void *);
extern int smtlib_bufSize(void);
extern void smtlib_setInteractive(bool);

SMTParser* SMTParser::parserTemp = NULL;

SMTParser::SMTParser(const std::string _filename, 
		     ExprBuilder* _builder) : fileName(_filename),
					      lineNum(1),
					      done(false),
					      satQuery(NULL),
					      bvSize(0),
					      queryParsed(false),
					      builder(_builder) {
  is = new ifstream(fileName.c_str());
  
  // Initial empty environments
  varEnvs.push(VarEnv());
  fvarEnvs.push(FVarEnv());
}

void SMTParser::Parse() {
  SMTParser::parserTemp = this;

  void *buf = smtlib_createBuffer(smtlib_bufSize());
  smtlib_switchToBuffer(buf);
  smtlib_setInteractive(false);
  smtlibparse();
  //xcout << "Parsed successfully.\n";
}

Decl* SMTParser::ParseTopLevelDecl() {
  return new QueryCommand(assumptions, builder->Not(satQuery),
			  std::vector<ExprHandle>(), 
			  std::vector<const Array*>());
}

bool SMTParser::Solve() {
  // FIXME: Support choice of solver.
  bool UseDummySolver = false, UseFastCexSolver = true, UseSTPQueryPCLog = true;
  Solver *S, *STP = S = 
    UseDummySolver ? createDummySolver() : new STPSolver(true);
  if (UseSTPQueryPCLog)
    S = createPCLoggingSolver(S, "stp-queries.pc");
  if (UseFastCexSolver)
    S = createFastCexSolver(S);
  S = createCexCachingSolver(S);
  S = createCachingSolver(S);
  S = createIndependentSolver(S);
  if (0)
    S = createValidatingSolver(S, STP);

  Decl *D = this->ParseTopLevelDecl();
  if (QueryCommand *QC = dyn_cast<QueryCommand>(D)) {
    //llvm::cout << "Query " << ":\t";

    assert("FIXME: Support counterexample query commands!");
    if (QC->Values.empty() && QC->Objects.empty()) {
      bool result;
      if (S->mustBeTrue(Query(ConstraintManager(QC->Constraints), QC->Query),
			result)) {
	//std::cout << (result ? "VALID" : "INVALID") << "\n";
	return result;	
      }
    }
  }
  llvm::cout << "FAIL";
  exit(1);
}


// XXX: give more info
int SMTParser::Error(const string& msg) {
  std::cerr << SMTParser::parserTemp->fileName << ":" 
	    << SMTParser::parserTemp->lineNum
	    << ": " << msg << "\n";
  exit(1);
  return 0;
}


int SMTParser::StringToInt(const std::string& s) {
  std::stringstream str(s);
  int x;
  str >> x;
  assert(str);
  return x;
}


ExprHandle SMTParser::CreateAnd(std::vector<ExprHandle> kids) {
  unsigned n_kids = kids.size();
  assert(n_kids);
  if (n_kids == 1)
    return kids[0];

  ExprHandle r = builder->And(kids[n_kids-2], kids[n_kids-1]);
  for (int i=n_kids-3; i>=0; i--)
    r = builder->And(kids[i], r);
  return r;
}

ExprHandle SMTParser::CreateOr(std::vector<ExprHandle> kids) {
  unsigned n_kids = kids.size();
  assert(n_kids);
  if (n_kids == 1)
    return kids[0];

  ExprHandle r = builder->Or(kids[n_kids-2], kids[n_kids-1]);
  for (int i=n_kids-3; i>=0; i--)
    r = builder->Or(kids[i], r);
  return r;
}

ExprHandle SMTParser::CreateXor(std::vector<ExprHandle> kids) {
  unsigned n_kids = kids.size();
  assert(n_kids);
  if (n_kids == 1)
    return kids[0];

  ExprHandle r = builder->Xor(kids[n_kids-2], kids[n_kids-1]);
  for (int i=n_kids-3; i>=0; i--)
    r = builder->Xor(kids[i], r);
  return r;
}


void SMTParser::DeclareExpr(std::string name, Expr::Width w) {
  // for now, only allow variables which are multiples of 8
  if (w % 8 != 0) {
    cout << "BitVec not multiple of 8 (" << w << ").  Need to update code.\n";
    exit(1);
  }
  
#ifdef DEBUG
  std::cout << "Declaring " << name << " of width " << w << "\n";
#endif
  
  Array *arr = new Array(name, w / 8);
  
  ref<Expr> *kids = new ref<Expr>[w/8];
  for (unsigned i=0; i < w/8; i++)
    kids[i] = builder->Read(UpdateList(arr, NULL), 
			    builder->Constant(i, 32));
  ref<Expr> var = ConcatExpr::createN(w/8, kids); // XXX: move to builder?
  delete [] kids;
  
  AddVar(name, var);
}


ExprHandle SMTParser::GetConstExpr(std::string val, uint8_t base, klee::Expr::Width w) {
  assert(base == 2 || base == 10 || base == 16);
  llvm::APInt ap(w, val.c_str(), val.length(), base);
  
  return klee::ConstantExpr::alloc(ap);
}


void SMTParser::PushVarEnv() {
#ifdef DEBUG
  cout << "Pushing new var env\n";
#endif
  varEnvs.push(VarEnv(varEnvs.top()));
}

void SMTParser::PopVarEnv() {
#ifdef DEBUG
  cout << "Popping var env\n";
#endif
  varEnvs.pop();
}

void SMTParser::AddVar(std::string name, ExprHandle val) {
#ifdef DEBUG
  cout << "Adding (" << name << ", " << val << ") to current var env.\n";
#endif
  varEnvs.top()[name] = val;
}

ExprHandle SMTParser::GetVar(std::string name) {
  VarEnv top = varEnvs.top();
  if (top.find(name) == top.end()) {
    std::cerr << "Cannot find variable ?" << name << "\n";
    exit(1);
  }
  return top[name];
}


void SMTParser::PushFVarEnv() {
  fvarEnvs.push(FVarEnv(fvarEnvs.top()));
}

void SMTParser::PopFVarEnv(void) {
#ifdef DEBUG
  cout << "Popping fvar env\n";
#endif
  fvarEnvs.pop();
}

void SMTParser::AddFVar(std::string name, ExprHandle val) {
#ifdef DEBUG
  cout << "Adding (" << name << ", " << val << ") to current fvar env.\n";
#endif
  fvarEnvs.top()[name] = val;
}

ExprHandle SMTParser::GetFVar(std::string name) {
  FVarEnv top = fvarEnvs.top();
  if (top.find(name) == top.end()) {
    std::cerr << "Cannot find fvar $" << name << "\n";
    exit(1);
  }
  return top[name];
}
