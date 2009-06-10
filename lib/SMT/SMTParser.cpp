//===-- SMTParser.cpp -----------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "parser_temp.h"
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

namespace CVC3 {
  ParserTemp* parserTemp;
}

void SMTParser::Init() {
  cout << "Initializing parser\n";
  void *buf = smtlib_createBuffer(smtlib_bufSize());

  CVC3::parserTemp = new CVC3::ParserTemp();
  CVC3::parserTemp->fileName = fname;
  CVC3::parserTemp->is = new ifstream(fname.c_str());
  CVC3::parserTemp->interactive = false;
  
  smtlib_switchToBuffer(buf);
  smtlib_setInteractive(false);
  smtlibparse();
}

Decl* SMTParser::ParseTopLevelDecl() {
  return NULL;
}
