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
						   lineNum(0),
						   done(false),
						   expr(NULL),
						   bvSize(0),
						   queryParsed(false) {
  is = new ifstream(filename.c_str());
}

void SMTParser::Init() {
  cout << "Initializing parser\n";
  SMTParser::parserTemp = this;

  void *buf = smtlib_createBuffer(smtlib_bufSize());
  smtlib_switchToBuffer(buf);
  smtlib_setInteractive(false);
  smtlibparse();
}

Decl* SMTParser::ParseTopLevelDecl() {
  return NULL;
}

// XXX: give more info
int SMTParser::Error(const string& s) {
  std::cerr << "error: " << s << "\n";
  exit(1);
  return 0;
}
