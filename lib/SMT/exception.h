/*****************************************************************************/
/*!
 * \file exception.h
 * 
 * Author: Sergey Berezin
 * 
 * Created: Thu Feb  6 13:09:44 2003
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
 * A generic exception.  Any thrown exception must inherit from this
 * class and whenever possible, set the error message.
 */
/*****************************************************************************/

#ifndef _cvc3__exception_h_
#define _cvc3__exception_h_

#include <string>
#include <iostream>

namespace CVC3 {

  class Exception {
  protected:
    std::string d_msg;
  public:
    // Constructors
    Exception(): d_msg("Unknown exception") { }
    Exception(const std::string& msg): d_msg(msg) { }
    Exception(const char* msg): d_msg(msg) { }
    // Destructor
    virtual ~Exception() { }
    // NON-VIRTUAL METHODs for setting and printing the error message
    void setMessage(const std::string& msg) { d_msg = msg; }
    // Printing: feel free to redefine toString().  When inherited,
    // it's recommended that this method print the type of exception
    // before the actual message.
    virtual std::string toString() const { return d_msg; }
    // No need to overload operator<< for the inherited classes
    friend std::ostream& operator<<(std::ostream& os, const Exception& e);

  }; // end of class Exception

  inline std::ostream& operator<<(std::ostream& os, const Exception& e) {
    return os << e.toString();
  }

} // end of namespace CVC3 

#endif
