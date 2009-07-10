//===-- SMTParser.cpp -----------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "SMTParser.h"
#include "expr/Parser.h"

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <cassert>
#include <stack>

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

SMTParser::SMTParser(const std::string filename) : fileName(filename), 
						   lineNum(1),
						   done(false),
						   query(NULL),
						   bvSize(0),
						   queryParsed(false) {
  is = new ifstream(filename.c_str());
  
  // Initial empty environments
  varEnvs.push(VarEnv());
  fvarEnvs.push(FVarEnv());
}

void SMTParser::Init() {
  SMTParser::parserTemp = this;

  void *buf = smtlib_createBuffer(smtlib_bufSize());
  smtlib_switchToBuffer(buf);
  smtlib_setInteractive(false);
  smtlibparse();
  cout << "Parsed successfully.\n";
}

Decl* SMTParser::ParseTopLevelDecl() {
  assert(done);
  return new QueryCommand(assumptions, query, 
			  std::vector<ExprHandle>(), 
			  std::vector<const Array*>());
}

// XXX: give more info
int SMTParser::Error(const string& s) {
  std::cerr << "error: " << s << "\n";
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

  ExprHandle r = AndExpr::create(kids[n_kids-2], kids[n_kids-1]);
  for (int i=n_kids-3; i>=0; i--)
    r = AndExpr::create(kids[i], r);
  return r;
}

ExprHandle SMTParser::CreateOr(std::vector<ExprHandle> kids) {
  unsigned n_kids = kids.size();
  assert(n_kids);
  if (n_kids == 1)
    return kids[0];

  ExprHandle r = OrExpr::create(kids[n_kids-2], kids[n_kids-1]);
  for (int i=n_kids-3; i>=0; i--)
    r = OrExpr::create(kids[i], r);
  return r;
}

ExprHandle SMTParser::CreateXor(std::vector<ExprHandle> kids) {
  unsigned n_kids = kids.size();
  assert(n_kids);
  if (n_kids == 1)
    return kids[0];

  ExprHandle r = XorExpr::create(kids[n_kids-2], kids[n_kids-1]);
  for (int i=n_kids-3; i>=0; i--)
    r = XorExpr::create(kids[i], r);
  return r;
}


void SMTParser::DeclareExpr(std::string name, Expr::Width w) {
  // for now, only allow variables which are multiples of 8
  if (w % 8 != 0) {
    cout << "BitVec not multiple of 8 (" << w << ").  Need to update code.\n";
    exit(1);
  }
  
  std::cout << "Declaring " << name << " of width " << w << "\n";
  
  Array *arr = new Array(name, w / 8);
  
  ref<Expr> *kids = new ref<Expr>[w/8];
  for (unsigned i=0; i < w/8; i++)
    kids[i] = ReadExpr::create(UpdateList(arr, NULL), 
			       ConstantExpr::create(i, 32));
  ref<Expr> var = ConcatExpr::createN(w/8, kids);
  delete [] kids;
  
  AddVar(name, var);
}


ExprHandle SMTParser::GetConstExpr(std::string val, uint8_t base, klee::Expr::Width w) {
  cerr << "In GetConstExpr(): val=" << val << ", base=" << (unsigned)base << ", width=" << w << "\n";
  assert(base == 2 || base == 10 || base == 16);
  llvm::APInt ap(w, val.c_str(), val.length(), base);
  
  return klee::ConstantExpr::alloc(ap);
}


void SMTParser::PushVarEnv() {
  cout << "Pushing new var env\n";
  varEnvs.push(VarEnv(varEnvs.top()));
}

void SMTParser::PopVarEnv() {
  cout << "Popping var env\n";
  varEnvs.pop();
}

void SMTParser::AddVar(std::string name, ExprHandle val) {
  cout << "Adding (" << name << ", " << val << ") to current var env.\n";
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
  cout << "Popping fvar env\n";
  fvarEnvs.pop();
}

void SMTParser::AddFVar(std::string name, ExprHandle val) {
  cout << "Adding (" << name << ", " << val << ") to current fvar env.\n";
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
