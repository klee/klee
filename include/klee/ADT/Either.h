//===-- Either.h ------------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_EITHER_H
#define KLEE_EITHER_H

#include "klee/ADT/Ref.h"

#include "klee/Support/Casting.h"

#include <cassert>

namespace llvm {
class raw_ostream;
} // namespace llvm

namespace klee {

template <class T1, class T2> class either_left;
template <class T1, class T2> class either_right;

template <class T1, class T2> class either {
protected:
  friend class ref<either<T1, T2>>;
  friend class ref<const either<T1, T2>>;

  /// @brief Required by klee::ref-managed objects
  class ReferenceCounter _refCount;
  unsigned hashValue;

  static const unsigned MAGIC_HASH_CONSTANT = 39;

public:
  using left = either_left<T1, T2>;
  using right = either_right<T1, T2>;

  enum class EitherKind { Left, Right };

  virtual ~either() {}
  virtual EitherKind getKind() const = 0;

  static bool classof(const either<T1, T2> *) { return true; }
  // virtual unsigned hash() = 0;
  virtual int compare(const either<T1, T2> &b) = 0;
  virtual bool equals(const either<T1, T2> &b) = 0;

  unsigned hash() const { return hashValue; }
};

template <class T1, class T2> class either_left : public either<T1, T2> {
protected:
  friend class ref<either_left<T1, T2>>;
  friend class ref<const either_left<T1, T2>>;

private:
  ref<T1> value_;

  unsigned computeHash() {
    unsigned res = (unsigned)getKind();
    res = (res * either<T1, T2>::MAGIC_HASH_CONSTANT) + value_->hash();
    either<T1, T2>::hashValue = res;
    return either<T1, T2>::hashValue;
  }

public:
  either_left(ref<T1> leftValue) : value_(leftValue) { computeHash(); };

  ref<T1> value() const { return value_; }
  operator ref<T1> const() { return value_; }

  virtual typename either<T1, T2>::EitherKind getKind() const override {
    return either<T1, T2>::EitherKind::Left;
  }

  static bool classof(const either<T1, T2> *S) {
    return S->getKind() == either<T1, T2>::EitherKind::Left;
  }
  static bool classof(const either_left<T1, T2> *) { return true; }

  // virtual unsigned hash() override { return value_->hash(); };
  virtual int compare(const either<T1, T2> &b) override {
    if (b.getKind() != getKind()) {
      return b.getKind() < getKind() ? -1 : 1;
    }
    const either_left<T1, T2> &el = static_cast<const either_left<T1, T2> &>(b);
    if (el.value() != value()) {
      return el.value() < value() ? -1 : 1;
    }
    return 0;
  }

  virtual bool equals(const either<T1, T2> &b) override {
    if (b.getKind() != getKind()) {
      return false;
    }
    const either_left<T1, T2> &el = static_cast<const either_left<T1, T2> &>(b);
    return el.value() == value();
  }
};

template <class T1, class T2> class either_right : public either<T1, T2> {
protected:
  friend class ref<either_right<T1, T2>>;
  friend class ref<const either_right<T1, T2>>;

private:
  ref<T2> value_;

  unsigned computeHash() {
    unsigned res = (unsigned)getKind();
    res = (res * either<T1, T2>::MAGIC_HASH_CONSTANT) + value_->hash();
    either<T1, T2>::hashValue = res;
    return either<T1, T2>::hashValue;
  }

public:
  either_right(ref<T2> rightValue) : value_(rightValue) { computeHash(); };

  ref<T2> value() const { return value_; }
  operator ref<T2> const() { return value_; }

  virtual typename either<T1, T2>::EitherKind getKind() const override {
    return either<T1, T2>::EitherKind::Right;
  }

  static bool classof(const either<T1, T2> *S) {
    return S->getKind() == either<T1, T2>::EitherKind::Right;
  }
  static bool classof(const either_right<T1, T2> *) { return true; }

  // virtual unsigned hash() override { return value_->hash(); };
  virtual int compare(const either<T1, T2> &b) override {
    if (b.getKind() != getKind()) {
      return b.getKind() < getKind() ? -1 : 1;
    }
    const either_right<T1, T2> &el =
        static_cast<const either_right<T1, T2> &>(b);
    if (el.value() != value()) {
      return el.value() < value() ? -1 : 1;
    }
    return 0;
  }

  virtual bool equals(const either<T1, T2> &b) override {
    if (b.getKind() != getKind()) {
      return false;
    }
    const either_right<T1, T2> &el =
        static_cast<const either_right<T1, T2> &>(b);
    return el.value() == value();
  }
};
} // end namespace klee

#endif /* KLEE_EITHER_H */
