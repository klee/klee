/*****************************************************************************/
/*!
 * \file parser.h
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
 * The top-level API to the parser.  At this level, it is simply a
 * stream of commands (Expr's) terminated by an infinite sequence of
 * Null Expr.  It has a support to parse several different input
 * languages (as many as we care to implement), and may take a file
 * name, or an istream to read from (default: std::cin, or stdin).
 * Using iostream means no more worries about whether to close files
 * or not.
 */
/*****************************************************************************/

#ifndef _cvc3__parser_h_
#define _cvc3__parser_h_

#include "exception.h"
#include "lang.h"

namespace CVC3 {

  class ValidityChecker;
  class Expr;
  
  // Internal parser state and other data
  class ParserData;

  class Parser {
  private:
    ParserData* d_data;
    // Internal methods for constructing and destroying the actual parser
    void initParser();
    void deleteParser();
  public:
    // Constructors
    Parser(ValidityChecker* vc, InputLanguage lang,
	   // The 'interactive' flag is ignored when fileName != ""
	   bool interactive = true,
	   const std::string& fileName = "");
    Parser(ValidityChecker* vc, InputLanguage lang, std::istream& is,
	   bool interactive = false);
    // Destructor
    ~Parser();
    // Read the next command.  
    Expr next();
    // Check if we are done (end of input has been reached)
    bool done() const;
    // The same check can be done by using the class Parser's value as
    // a Boolean
    operator bool() const { return done(); }
    void printLocation(std::ostream & out) const;
    // Reset any local data that depends on validity checker
    void reset();
  }; // end of class Parser

  // The global pointer to ParserTemp.  Each instance of class Parser
  // sets this pointer before any calls to the lexer.  We do it this
  // way because flex and bison use global vars, and we want each
  // Parser object to appear independent.
  class ParserTemp;
  extern ParserTemp* parserTemp;

} // end of namespace CVC3

#endif
