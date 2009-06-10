/*****************************************************************************/
/*!
 * \file parser.cpp
 * \brief The top-level API to the parser.  See parser.h for details.
 * 
 * Author: Sergey Berezin
 * 
 * Created: Wed Feb  5 11:46:57 2003
 *
 * <hr>
 *
 * License to use, copy, modify, sell and/or distribute this software
 * and its documentation for any purpose is hereby granted without
 * royalty, subject to the terms and conditions defined in the \ref
 * LICENSE file provided with this distribution.
 * 
 * <hr>
 * 
 */
/*****************************************************************************/

#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdlib.h>
#include "parser_temp.h"
#include "parser.h"

using namespace std;

// The communication entry points of the actual parsers

// for smtlib language (smtlib.y and smtlib.lex)
extern int smtlibparse(); 
extern void *smtlib_createBuffer(int);
extern void smtlib_deleteBuffer(void *);
extern void smtlib_switchToBuffer(void *);
extern int smtlib_bufSize();
extern void *smtlibBufState();
void smtlib_setInteractive(bool);

namespace CVC3 {

  // The global pointer to ParserTemp.  Each instance of class Parser
  // sets this pointer before any calls to the lexer.  We do it this
  // way because flex and bison use global vars, and we want each
  // Parser object to appear independent.  

  // FIXME: This should probably go away eventually, when I figure out
  // flex++   -- Sergey.
  ParserTemp* parserTemp;

  int ParserTemp::error(const string& s) {
    // FIXME: Fail better?
    std::cerr << "error: " << s << "\n";
    exit(1);
    return 0;
  }

  // Internal storage class; I'll use member names without 'd_'s here
  class ParserData {
  public:
    // Is the input given by the file name or as istream?
    bool useName;
    ParserTemp temp;
    // Internal buffer used by the parser
    void* buffer;
  };

  // Constructors
  Parser::Parser(bool interactive, const std::string& fileName)
    : d_data(new ParserData) {
    if(fileName == "") {
      // Use std::cin
      d_data->useName = false;
      d_data->temp.is = &cin;
      d_data->temp.fileName = "stdin";
      d_data->temp.interactive = interactive;
    } else {
      // Open the file by name
      d_data->useName = true;
      d_data->temp.fileName = fileName;
      d_data->temp.is = new ifstream(fileName.c_str());
      if (!(*d_data->temp.is)) {
        // FIXME: Fail better?
        std::cerr << "error: File not found: " << fileName << "\n";
        exit(1);
      }
      d_data->temp.interactive = false;
    }
    initParser();
  }

  Parser::Parser(std::istream& is, bool interactive)
    : d_data(new ParserData) {
    d_data->useName = false;
    d_data->temp.is = &is;
    d_data->temp.fileName = "stdin";
    d_data->temp.interactive = interactive;
    initParser();
  }
    
  // Destructor
  Parser::~Parser() {
    if(d_data->useName) // Stream is ours; delete it
      delete d_data->temp.is;
    deleteParser();
    delete d_data;
  }

  // Initialize the actual parser
  void Parser::initParser() {
    d_data->buffer = smtlib_createBuffer(smtlib_bufSize());
    d_data->temp.lineNum = 1;
  }

  // Clean up the parser's internal data structures
  void Parser::deleteParser() {
    smtlib_deleteBuffer(d_data->buffer);
  }
    

  klee::expr::ExprHandle Parser::next() {
    // If no more commands are available, return a Null Expr
    if(d_data->temp.done) return NULL;//Expr();
    // Set the global var so the parser uses the right stream and EM
    parserTemp = &(d_data->temp);
    // Switch to our buffer, in case there are multiple instances of
    // the parser running
    smtlib_switchToBuffer(d_data->buffer);
    smtlib_setInteractive(d_data->temp.interactive);
    smtlibparse();
    // Reset the prompt to the main one
    d_data->temp.setPrompt1();
    return d_data->temp.expr;
  }

  bool Parser::done() const { return d_data->temp.done; }

  void Parser::printLocation(ostream & out) const
  {
      out << d_data->temp.fileName << ":" << d_data->temp.lineNum;
  }

  void Parser::reset()
  {
    d_data->temp.expr = NULL;//Expr();
  }
  

} // end of namespace CVC3
