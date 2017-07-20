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
    bindings_ty bindings;
    
  public:
    Assignment() {}
    Assignment(const std::vector<const Array *> &objects,
               std::vector<std::vector<unsigned char> > &values) {
      std::vector< std::vector<unsigned char> >::iterator valIt = 
        values.begin();
      for (std::vector<const Array*>::const_iterator it = objects.begin(),
             ie = objects.end(); it != ie; ++it) {
        const Array *os = *it;
        std::vector<unsigned char> &arr = *valIt;
        bindings.insert(std::make_pair(os, arr));
        ++valIt;
      }
    }

    ref<Expr> evaluate(const Array *mo, unsigned index,
                       bool concretizeSymbolic) const;
    ref<Expr> evaluate(ref<Expr> e, bool concretizeSymbolic);
    void createConstraintsFromAssignment(std::vector<ref<Expr> > &out) const;

    template <typename InputIterator>
    bool satisfies(InputIterator begin, InputIterator end,
                   bool concretizeSymbolic);
    void dump();
  };
  
  class AssignmentEvaluator : public ExprEvaluator {
    const Assignment &a;
    bool concretizeSymbolic;

  protected:
    ref<Expr> getInitialValue(const Array &mo, unsigned index) {
      return a.evaluate(&mo, index, concretizeSymbolic);
    }
    
  public:
    AssignmentEvaluator(const Assignment &_a, bool _concretizeSymbolic)
        : a(_a), concretizeSymbolic(_concretizeSymbolic) {}
  };

  /***/

  inline ref<Expr> Assignment::evaluate(const Array *array, unsigned index,
                                        bool concretizeSymbolic) const {
    assert(array);
    bindings_ty::const_iterator it = bindings.find(array);
    if (it!=bindings.end() && index<it->second.size()) {
      return ConstantExpr::alloc(it->second[index], array->getRange());
    } else {
      if (!concretizeSymbolic) {
        return ReadExpr::create(UpdateList(array, 0), 
                                ConstantExpr::alloc(index, array->getDomain()));
      } else {
        return ConstantExpr::alloc(255, array->getRange());
      }
    }
  }

  inline ref<Expr> Assignment::evaluate(ref<Expr> e, bool concretizeSymbolic) {
    AssignmentEvaluator v(*this, concretizeSymbolic);
    return v.visit(e); 
  }

  template <typename InputIterator>
  inline bool Assignment::satisfies(InputIterator begin, InputIterator end,
                                    bool concretizeSymbolic) {
    AssignmentEvaluator v(*this, concretizeSymbolic);
    for (; begin!=end; ++begin)
      if (!v.visit(*begin)->isTrue())
        return false;
    return true;
  }
}

#endif
