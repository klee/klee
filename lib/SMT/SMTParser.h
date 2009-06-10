//===-- SMTParser.h -------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//


#ifndef SMT_PARSER_H
#define SMT_PARSER_H

#include "parser_temp.h"
#include "expr/Parser.h"

#include <cassert>
#include <iostream>
#include <map>
#include <cstring>

namespace klee {
namespace expr {

class SMTParser : public klee::expr::Parser {
 private:
  std::string fname;
  void *buf;

 public:
  SMTParser(const std::string filename) : fname(filename) {}
  
  virtual klee::expr::Decl *ParseTopLevelDecl();
  
  virtual void SetMaxErrors(unsigned N) { }
  
  virtual unsigned GetNumErrors() const {  return 1; }
  
  virtual ~SMTParser() {}
  
  void Init(void);
};

}
}

#endif

