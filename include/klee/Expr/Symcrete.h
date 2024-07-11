#ifndef KLEE_SYMCRETE_H
#define KLEE_SYMCRETE_H

#include "klee/ADT/Ref.h"
#include "klee/Expr/ExprUtil.h"

#include <cstdint>
#include <vector>

namespace klee {

class Array;
class Expr;

typedef uint64_t IDType;

class Symcrete {
  friend class ConstraintSet;

public:
  enum class SymcreteKind {
    SK_ALLOC_SIZE,
    SK_ALLOC_ADDRESS,
    SK_LI_SIZE,
    SK_LI_ADDRESS
  };

private:
  static IDType idCounter;
  const SymcreteKind symcreteKind;

  std::vector<std::reference_wrapper<Symcrete>> _dependentSymcretes;
  std::vector<const Array *> _dependentArrays;

public:
  const IDType id;
  ReferenceCounter _refCount;

  const ref<Expr> symcretized;

  Symcrete(ref<Expr> e, SymcreteKind kind)
      : symcreteKind(kind), id(idCounter++), symcretized(e) {
    findObjects(e, _dependentArrays);
  }

  virtual ~Symcrete() = default;

  void addDependentSymcrete(ref<Symcrete> dependent) {
    _dependentSymcretes.push_back(*dependent);
  }

  const std::vector<std::reference_wrapper<Symcrete>> &
  dependentSymcretes() const {
    return _dependentSymcretes;
  }

  const std::vector<const Array *> &dependentArrays() const {
    return _dependentArrays;
  }

  SymcreteKind getKind() const { return symcreteKind; }

  bool equals(const Symcrete &rhs) const { return id == rhs.id; }

  int compare(const Symcrete &rhs) const {
    if (equals(rhs)) {
      return 0;
    }
    return id < rhs.id ? -1 : 1;
  }

  unsigned hash() { return id; }

  bool operator<(const Symcrete &rhs) const { return compare(rhs) < 0; };
};

struct SymcreteLess {
  bool operator()(const ref<Symcrete> &a, const ref<Symcrete> &b) const {
    return a < b;
  }
};

typedef std::set<ref<Symcrete>, SymcreteLess> SymcreteOrderedSet;

class AddressSymcrete : public Symcrete {
public:
  AddressSymcrete(ref<Expr> s, SymcreteKind kind) : Symcrete(s, kind) {
    assert((kind == SymcreteKind::SK_ALLOC_ADDRESS ||
            kind == SymcreteKind::SK_LI_ADDRESS) &&
           "wrong kind");
  }

  static bool classof(const Symcrete *symcrete) {
    return symcrete->getKind() == SymcreteKind::SK_ALLOC_SIZE ||
           symcrete->getKind() == SymcreteKind::SK_LI_SIZE;
  }
};

class AllocAddressSymcrete : public AddressSymcrete {
public:
  AllocAddressSymcrete(ref<Expr> s)
      : AddressSymcrete(s, SymcreteKind::SK_ALLOC_ADDRESS) {}

  static bool classof(const Symcrete *symcrete) {
    return symcrete->getKind() == SymcreteKind::SK_ALLOC_ADDRESS;
  }
};

class LazyInitializedAddressSymcrete : public AddressSymcrete {
public:
  LazyInitializedAddressSymcrete(ref<Expr> s)
      : AddressSymcrete(s, SymcreteKind::SK_LI_ADDRESS) {}

  static bool classof(const Symcrete *symcrete) {
    return symcrete->getKind() == SymcreteKind::SK_LI_ADDRESS;
  }
};

class SizeSymcrete : public Symcrete {
public:
  const AddressSymcrete &addressSymcrete;

  SizeSymcrete(ref<Expr> s, ref<AddressSymcrete> address, SymcreteKind kind)
      : Symcrete(s, kind), addressSymcrete(*address) {
    assert((kind == SymcreteKind::SK_ALLOC_SIZE ||
            kind == SymcreteKind::SK_LI_SIZE) &&
           "wrong kind");
    addDependentSymcrete(address);
  }

  static bool classof(const Symcrete *symcrete) {
    return symcrete->getKind() == SymcreteKind::SK_ALLOC_SIZE ||
           symcrete->getKind() == SymcreteKind::SK_LI_SIZE;
  }
};

class AllocSizeSymcrete : public SizeSymcrete {
public:
  AllocSizeSymcrete(ref<Expr> s, ref<AllocAddressSymcrete> address)
      : SizeSymcrete(s, address, SymcreteKind::SK_ALLOC_SIZE) {}

  static bool classof(const Symcrete *symcrete) {
    return symcrete->getKind() == SymcreteKind::SK_ALLOC_SIZE;
  }
};

class LazyInitializedSizeSymcrete : public SizeSymcrete {
public:
  LazyInitializedSizeSymcrete(ref<Expr> s,
                              ref<LazyInitializedAddressSymcrete> address)
      : SizeSymcrete(s, address, SymcreteKind::SK_LI_SIZE) {}

  static bool classof(const Symcrete *symcrete) {
    return symcrete->getKind() == SymcreteKind::SK_LI_SIZE;
  }
};

} // namespace klee

#endif
