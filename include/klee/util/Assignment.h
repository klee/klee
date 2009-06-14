//===-- Assignment.h --------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_UTIL_ASSIGNMENT_H
#define KLEE_UTIL_ASSIGNMENT_H

#include <map>

#include "klee/util/ExprEvaluator.h"

// FIXME: Rename?

namespace klee {
  class Array;

  class Assignment {
  public:
    typedef std::map<const Array*, std::vector<unsigned char> > bindings_ty;

    bool allowFreeValues;
    bindings_ty bindings;
    
  public:
    Assignment(bool _allowFreeValues=false) 
      : allowFreeValues(_allowFreeValues) {}
    Assignment(std::vector<const Array*> &objects, 
               std::vector< std::vector<unsigned char> > &values,
               bool _allowFreeValues=false) 
      : allowFreeValues(_allowFreeValues){
      std::vector< std::vector<unsigned char> >::iterator valIt = 
        values.begin();
      for (std::vector<const Array*>::iterator it = objects.begin(),
             ie = objects.end(); it != ie; ++it) {
        const Array *os = *it;
        std::vector<unsigned char> &arr = *valIt;
        bindings.insert(std::make_pair(os, arr));
        ++valIt;
      }
    }
    
    ref<Expr> evaluate(const Array *mo, unsigned index) const;
    ref<Expr> evaluate(ref<Expr> e);

    template<typename InputIterator>
    bool satisfies(InputIterator begin, InputIterator end);
  };
  
  class AssignmentEvaluator : public ExprEvaluator {
    const Assignment &a;

  protected:
    ref<Expr> getInitialValue(const Array &mo, unsigned index) {
      return a.evaluate(&mo, index);
    }
    
  public:
    AssignmentEvaluator(const Assignment &_a) : a(_a) {}    
  };

  /***/

  inline ref<Expr> Assignment::evaluate(const Array *array, 
                                        unsigned index) const {
    bindings_ty::const_iterator it = bindings.find(array);
    if (it!=bindings.end() && index<it->second.size()) {
      return ConstantExpr::alloc(it->second[index], Expr::Int8);
    } else {
      if (allowFreeValues) {
        return ReadExpr::create(UpdateList(array, 0), 
                                ConstantExpr::alloc(index, Expr::Int32));
      } else {
        return ConstantExpr::alloc(0, Expr::Int8);
      }
    }
  }

  inline ref<Expr> Assignment::evaluate(ref<Expr> e) { 
    AssignmentEvaluator v(*this);
    return v.visit(e); 
  }

  template<typename InputIterator>
  inline bool Assignment::satisfies(InputIterator begin, InputIterator end) {
    AssignmentEvaluator v(*this);
    for (; begin!=end; ++begin)
      if (!v.visit(*begin)->isTrue())
        return false;
    return true;
  }
}

#endif
