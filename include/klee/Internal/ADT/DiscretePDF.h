//===-- DiscretePDF.h -------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_DISCRETEPDF_H
#define KLEE_DISCRETEPDF_H

namespace klee {
  template <class T>
  class DiscretePDF {
    // not perfectly parameterized, but float/double/int should work ok,
    // although it would be better to have choose argument range from 0
    // to queryable max.
    typedef double weight_type;

  public:
    DiscretePDF();
    ~DiscretePDF();

    bool empty() const;
    void insert(T item, weight_type weight);
    void update(T item, weight_type newWeight);
    void remove(T item);
    bool inTree(T item);
    weight_type getWeight(T item);
	
    /* pick a tree element according to its
     * weight. p should be in [0,1).
     */
    T choose(double p);
    
  private:
    class Node;
    Node *m_root;
    
    Node **lookup(T item, Node **parent_out);
    void split(Node *node);
    void rotate(Node *node);
    void lengthen(Node *node);
    void propogateSumsUp(Node *n);
  };

}

#include "DiscretePDF.inc"

#endif /* KLEE_DISCRETEPDF_H */
