//===-- ObjectHolder.h ------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_OBJECTHOLDER_H
#define KLEE_OBJECTHOLDER_H

namespace klee {
  class ObjectState;

  class ObjectHolder {
    ObjectState *os;
    
  public:
    ObjectHolder() : os(0) {}
    ObjectHolder(ObjectState *_os);
    ObjectHolder(const ObjectHolder &b);
    ~ObjectHolder(); 
    
    ObjectHolder &operator=(const ObjectHolder &b);

    operator class ObjectState *() { return os; }
    operator class ObjectState *() const { return (ObjectState*) os; }
  };
}

#endif

