//===-- ImmutableSet.h ------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef __UTIL_IMMUTABLESET_H__
#define __UTIL_IMMUTABLESET_H__

#include <functional>

#include "ImmutableTree.h"

namespace klee {
  template<class T>
  struct _Identity {
    T &operator()(T &a) const { return a; }
    const T &operator()(const T &a) const { return a; }
  };
  
  template<class T, class CMP=std::less<T> >
  class ImmutableSet {
  public:
    typedef T key_type;
    typedef T value_type;

    typedef ImmutableTree<T, T, _Identity<T>, CMP> Tree;
    typedef typename Tree::iterator iterator;

  private:
    Tree elts;

    ImmutableSet(const Tree &b): elts(b) {}

  public:
    ImmutableSet() {}
    ImmutableSet(const ImmutableSet &b) : elts(b.elts) {}
    ~ImmutableSet() {}

    ImmutableSet &operator=(const ImmutableSet &b) { elts = b.elts; return *this; }
    
    bool empty() const { 
      return elts.empty(); 
    }
    size_t count(const key_type &key) const { 
      return elts.count(key); 
    }
    const value_type *lookup(const key_type &key) const { 
      return elts.lookup(key); 
    }
    const value_type &min() const { 
      return elts.min(); 
    }
    const value_type &max() const { 
      return elts.max(); 
    }
    size_t size() { 
      return elts.size(); 
    }

    ImmutableSet insert(const value_type &value) const {
      return elts.insert(value); 
    }
    ImmutableSet replace(const value_type &value) const {
      return elts.replace(value); 
    }
    ImmutableSet remove(const key_type &key) const { 
      return elts.remove(key); 
    }
    ImmutableSet popMin(const value_type &valueOut) const { 
      return elts.popMin(valueOut); 
    }
    ImmutableSet popMax(const value_type &valueOut) const { 
      return elts.popMax(valueOut); 
    }

    iterator begin() const { 
      return elts.begin(); 
    }
    iterator end() const { 
      return elts.end(); 
    }
    iterator find(const key_type &key) const { 
      return elts.find(key); 
    }
    iterator lower_bound(const key_type &key) const { 
      return elts.lower_bound(key); 
    }
    iterator upper_bound(const key_type &key) const { 
      return elts.upper_bound(key); 
    }

    static size_t getAllocated() { return Tree::allocated; }
  };

}

#endif
