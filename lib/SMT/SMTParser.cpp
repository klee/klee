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
