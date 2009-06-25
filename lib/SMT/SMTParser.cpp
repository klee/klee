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
#include <cstring>
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
