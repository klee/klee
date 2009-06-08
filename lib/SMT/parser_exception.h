/*****************************************************************************/
/*!
 * \file parser_exception.h
 * \brief An exception thrown by the parser.
 *
 * Author: Sergey Berezin
 *
 * Created: Thu Feb  6 13:23:39 2003
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

#ifndef _cvc3__parser_exception_h_
#define _cvc3__parser_exception_h_

#include "exception.h"
#include <string>
#include <iostream>

namespace CVC3 {

  class ParserException: public Exception {
  public:
    // Constructors
    ParserException() { }
    ParserException(const std::string& msg): Exception(msg) { }
    ParserException(const char* msg): Exception(msg) { }
    // Destructor
    virtual ~ParserException() { }
    virtual std::string toString() const {
      return "Parse Error: " + d_msg;
    }
  }; // end of class ParserException

} // end of namespace CVC3

#endif
