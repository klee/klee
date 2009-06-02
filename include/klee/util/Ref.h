//===-- Ref.h ---------------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_REF_H
#define KLEE_REF_H

#include <assert.h>

class Expr;
class ConstantExpr;

class ExprVisitor;
class StackFrame;
class ObjectState;

template<class T>
class ref {
public:
  // default constructor: create a NULL reference
  ref() : constantWidth(Expr::InvalidWidth) {
    contents.ptr = 0;
  }

private:
  // if NotConstant, not this ref is not a constant.
  // otherwise, it's the width of the constant.
  Expr::Width constantWidth;
  static const Expr::Width NotConstant = (Expr::Width) 0;
  
  union {
    T *ptr;
    uint64_t val;
  } contents;
  
  void inc() {
    if (constantWidth == NotConstant &&
        contents.ptr) {
      contents.ptr->refCount++;
    }
  }
  
  void dec() {
    if (constantWidth == NotConstant &&
        contents.ptr &&
        --contents.ptr->refCount == 0) {
      delete contents.ptr;
    }
  }  

  friend class ExprVisitor;
  friend class ConstantExpr;

  // construct from constant
  ref(uint64_t val, Expr::Width w) : constantWidth(w) {
    contents.val = val;
  }

public:
  template<class U> friend class ref;
  template<class U> friend U* dyn_ref_cast(ref &src);
  template<class U> friend const U* dyn_ref_cast(const ref &src);
  template<class U> friend U* static_ref_cast(ref &src);
  template<class U> friend const U* static_ref_cast(const ref &src);

  // constructor from pointer
  ref(T *p) : constantWidth(NotConstant) {
    contents.ptr = p;
    inc();
  }
  
  // normal copy constructor
  ref(const ref<T> &r)
    : constantWidth(r.constantWidth), contents(r.contents) {
    inc();
  }
  
  // conversion constructor
  template<class U>
  ref (const ref<U> &r) {
    constantWidth = r.constantWidth;
    contents.val = r.contents.val;
    inc();
  }
  
  // pointer operations
  T *get () {
    // demand(constantWidth == NotConstant, "deref of constant");

    // allocate 
    if (constantWidth != NotConstant) {
      contents.ptr = dynamic_cast<T*>(Expr::createConstant(contents.val, constantWidth));
      assert(contents.ptr && "error with lazy constant initialization");
      constantWidth = NotConstant;
      
      inc();
    }
    return contents.ptr;
  }

  T *get () const {
    assert(constantWidth == NotConstant && "deref of constant");
    return contents.ptr;
  }

  // method calls for the constant optimization
  bool isConstant() const {
    if (constantWidth != NotConstant) {
      return true;
    } else if (contents.ptr) {
      return contents.ptr->getKind() == Expr::Constant;
    } else {
      return false; // should never happen, but nice check
    }
  }

  uint64_t getConstantValue() const {
    if (constantWidth) {
      return contents.val;
    } else {
      return contents.ptr->getConstantValue();
    }
  }

  unsigned hash() const {
    if (constantWidth) {
      return Expr::hashConstant(contents.val, constantWidth);
    } else {
      return contents.ptr->hash();
    }
  }

  unsigned computeHash() const {
    if (isConstant()) {
      return Expr::hashConstant(contents.val, constantWidth);
    } else {
      return contents.ptr->computeHash();
    }
  }

  void rehash() const {
    if (!isConstant())
      contents.ptr->computeHash();
  }
  
  Expr::Width getWidth() const {
    if (constantWidth != NotConstant)
      return constantWidth;
    return contents.ptr->getWidth();
  }

  Expr::Kind getKind() const {
    if (constantWidth != NotConstant)
      return Expr::Constant;
    return contents.ptr->getKind();
  }

  unsigned getNumKids() const {
    if (constantWidth != NotConstant)
      return 0;
    return contents.ptr->getNumKids();
  }

  ref<Expr> getKid(unsigned k) {
    if (constantWidth != NotConstant)
      return 0;
    return contents.ptr->getKid(k);
  }

  ~ref () { dec (); }

  /* The copy assignment operator must also explicitly be defined,
   * despite a redundant template. */
  ref<T> &operator= (const ref<T> &r) {
    dec();
    constantWidth = r.constantWidth;
    contents.val = r.contents.val;
    inc();
    
    return *this;
  }
  
  template<class U> ref<T> &operator= (const ref<U> &r) {
    dec();
    constantWidth = r.constantWidth;
    contents.val = r.contents.val;
    inc();
    
    return *this;
  }

  bool isNull() const { return !constantWidth && !contents.ptr; }

  // assumes non-null arguments
  int compare(const ref &rhs) const {
    Expr::Kind ak = getKind(), bk = rhs.getKind();
    if (ak!=bk) 
      return (ak < bk) ? -1 : 1;
    if (ak==Expr::Constant) {
      Expr::Width at = getWidth(), bt = rhs.getWidth();
      if (at!=bt) 
        return (at < bt) ? -1 : 1;
      uint64_t av = getConstantValue(), bv = rhs.getConstantValue();
      if (av<bv) {
        return -1;
      } else if (av>bv) {
        return 1;
      } else {
        return 0;
      }
    } else {
      return get()->compare(*rhs.get());
    }
  }

  // assumes non-null arguments
  bool operator<(const ref &rhs) const { return compare(rhs)<0; }
  bool operator==(const ref &rhs) const { return compare(rhs)==0; }
  bool operator!=(const ref &rhs) const { return compare(rhs)!=0; }
};


template<class T>
inline std::ostream &operator<<(std::ostream &os, const ref<T> &e) {
  if (e.isConstant()) {
    os << e.getConstantValue();
  } else {
    os << *e.get();
  }
  return os;
}

template<class U>
U* dyn_ref_cast(ref<Expr> &src) {
  if (src.constantWidth != ref<Expr>::NotConstant)
    return 0;
    
  return dynamic_cast<U*>(src.contents.ptr);
}

template<class U>
const U* dyn_ref_cast(const ref<Expr> &src) {
  if (src.constantWidth != ref<Expr>::NotConstant)
    return 0;
    
  return dynamic_cast<const U*>(src.contents.ptr);
}

template<class U>
U* static_ref_cast(ref<Expr> &src) {
  return static_cast<U*>(src.contents.ptr);
}

template<class U>
const U* static_ref_cast(const ref<Expr> &src) {
  return static_cast<const U*>(src.contents.ptr);
}

#endif /* KLEE_REF_H */
