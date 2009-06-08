/*****************************************************************************/
/*!
 * \file parser_temp.h
 *
 * Author: Sergey Berezin
 *
 * Created: Wed Feb  5 17:53:02 2003
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
 * A class used to communicate with the actual parser.  No one else
 * should use it.
 */
/*****************************************************************************/

#ifndef _cvc3__parser_temp_h_
#define _cvc3__parser_temp_h_

#include "expr.h"
#include "exception.h"

namespace CVC3 {

  class ValidityChecker;

  class ParserTemp {
  private:
    // Counter for uniqueID of bound variables
    int d_uid;
    // The main prompt when running interactive
    std::string prompt1;
    // The interactive prompt in the middle of a multi-line command
    std::string prompt2;
    // The currently used prompt
    std::string prompt;
  public:
    ValidityChecker* vc;
    std::istream* is;
    // The current input line
    int lineNum;
    // File name
    std::string fileName;
    // The last parsed Expr
    Expr expr;
    // Whether we are done or not
    bool done;
    // Whether we are running interactive
    bool interactive;
    // Whether arrays are enabled for smt-lib format
    bool arrFlag;
    // Whether bit-vectors are enabled for smt-lib format
    bool bvFlag;
    // Size of bit-vectors for smt-lib format
    int bvSize;
    // Did we encounter a formula query (smtlib)
    bool queryParsed;
    // Default constructor
    ParserTemp() : d_uid(0), prompt1("CVC> "), prompt2("- "),
      prompt("CVC> "), lineNum(1), done(false), arrFlag(false), queryParsed(false) { }
    // Parser error handling (implemented in parser.cpp)
    int error(const std::string& s);
    // Get the next uniqueID as a string
    std::string uniqueID() {
      std::ostringstream ss;
      ss << d_uid++;
      return ss.str();
    }
    // Get the current prompt
    std::string getPrompt() { return prompt; }
    // Set the prompt to the main one
    void setPrompt1() { prompt = prompt1; }
    // Set the prompt to the secondary one
    void setPrompt2() { prompt = prompt2; }
  };

} // end of namespace CVC3

#endif
