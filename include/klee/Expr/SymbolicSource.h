#ifndef KLEE_SYMBOLICSOURCE_H
#define KLEE_SYMBOLICSOURCE_H

#include "klee/ADT/Ref.h"

#include "llvm/IR/Function.h"

namespace klee {

class Expr;

class SymbolicSource {
public:
  /// @brief Required by klee::ref-managed objects
  class ReferenceCounter _refCount;

  enum class Kind {
    Constant,
    MakeSymbolic,
    LazyInitializationSymbolic,
    SymbolicAddress
  };

public:
  virtual ~SymbolicSource() {}
  virtual Kind getKind() const = 0;
  virtual std::string getName() const = 0;

  static bool classof(const SymbolicSource *) { return true; }
};

class ConstantSource : public SymbolicSource {
public:
  Kind getKind() const override { return Kind::Constant; }
  virtual std::string getName() const override { return "constant"; }

  static bool classof(const SymbolicSource *S) {
    return S->getKind() == Kind::Constant;
  }
  static bool classof(const ConstantSource *) { return true; }
};

class MakeSymbolicSource : public SymbolicSource {
public:
  Kind getKind() const override { return Kind::MakeSymbolic; }
  virtual std::string getName() const override { return "symbolic"; }

  static bool classof(const SymbolicSource *S) {
    return S->getKind() == Kind::MakeSymbolic;
  }
  static bool classof(const MakeSymbolicSource *) { return true; }
};

class SymbolicAddressSource : public SymbolicSource {
public:
  Kind getKind() const override { return Kind::SymbolicAddress; }
  virtual std::string getName() const override { return "symbolicAddress"; }

  static bool classof(const SymbolicSource *S) {
    return S->getKind() == Kind::SymbolicAddress;
  }
  static bool classof(const SymbolicAddressSource *) { return true; }
};

class LazyInitializationSymbolicSource : public SymbolicSource {
public:
  Kind getKind() const override { return Kind::LazyInitializationSymbolic; }
  virtual std::string getName() const override {
    return "lazyInitializationMakeSymbolic";
  }

  static bool classof(const SymbolicSource *S) {
    return S->getKind() == Kind::LazyInitializationSymbolic;
  }
  static bool classof(const LazyInitializationSymbolicSource *) { return true; }
};

} // namespace klee

#endif /* KLEE_SYMBOLICSOURCE_H */
