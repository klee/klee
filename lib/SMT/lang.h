/*****************************************************************************/
/*!
 * \file lang.h
 * \brief Definition of input and output languages to CVC3
 * 
 * Author: Mehul Trivedi
 * 
 * Created: Thu Jul 29 11:56:34 2004
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

#ifndef _cvc3__lang_h_
#define _cvc3__lang_h_

#include "debug.h"

namespace CVC3 {

  //! Different input languages
  typedef enum {
    //! Nice SAL-like language for manually written specs
    PRESENTATION_LANG,
    //! SMT-LIB format
    SMTLIB_LANG,
    //! Lisp-like format for automatically generated specs
    LISP_LANG,
    AST_LANG,	

    /* @brief AST is only for printing the Expr abstract syntax tree in lisp
       format; there is no such input language <em>per se</em> */
    SIMPLIFY_LANG,
    //! for output into Simplify format
    TPTP_LANG
    //! for output in TPTP format
  } InputLanguage;
  
  inline InputLanguage getLanguage(const std::string& lang) {
    if (lang.size() > 0) {
      if(lang[0] == 'p') return PRESENTATION_LANG;
      if(lang[0] == 'l') return LISP_LANG;
      if(lang[0] == 'a') return AST_LANG;
      if(lang[0] == 't') return TPTP_LANG;
      if(lang[0] == 's') {
        if (lang.size() > 1 && lang[1] == 'i') return SIMPLIFY_LANG;
        else return SMTLIB_LANG;
      }
      
    }

    throw Exception("Bad input language specified");
    return AST_LANG;
  }

} // end of namespace CVC3

#endif
