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

#include "expr/Parser.h"

#include <map>
#include <stack>
#include <string>

namespace klee {
  class ExprBuilder;
  
namespace expr {

class SMTParser : public klee::expr::Parser {
  private:
    void *buf;
    
 public:
  /* For interacting w/ the actual parser, should make this nicer */
  static SMTParser* parserTemp;
  std::string fileName;
  std::istream* is;
  int lineNum;
  bool done;
  bool arraysEnabled;
  
  std::vector<ExprHandle> assumptions;
  klee::expr::ExprHandle satQuery;

  int bvSize;
  bool queryParsed;
  
  klee::ExprBuilder *builder;
    
  SMTParser(const std::string filename, ExprBuilder *builder);
  
  virtual klee::expr::Decl *ParseTopLevelDecl();
  bool Solve();
  
  virtual void SetMaxErrors(unsigned N) { }
  
  virtual unsigned GetNumErrors() const {  return 1; }
  
  virtual ~SMTParser() {}
  
  void Parse(void);
  
  int Error(const std::string& s);
  
  int StringToInt(const std::string& s);
  ExprHandle GetConstExpr(std::string val, uint8_t base, klee::Expr::Width w);
  
  void DeclareExpr(std::string name, Expr::Width w);
  
  ExprHandle CreateAnd(std::vector<ExprHandle>);
  ExprHandle CreateOr(std::vector<ExprHandle>);
  ExprHandle CreateXor(std::vector<ExprHandle>);
  

  typedef std::map<const std::string, ExprHandle> VarEnv;
  typedef std::map<const std::string, ExprHandle> FVarEnv;

  std::stack<VarEnv> varEnvs;
  std::stack<FVarEnv> fvarEnvs;

  void PushVarEnv(void);
  void PopVarEnv(void);
  void AddVar(std::string name, ExprHandle val); // to current var env
  ExprHandle GetVar(std::string name); // from current var env

  void PushFVarEnv(void);
  void PopFVarEnv(void);
  void AddFVar(std::string name, ExprHandle val); // to current fvar env
  ExprHandle GetFVar(std::string name); // from current fvar env
};

}
}

#endif

