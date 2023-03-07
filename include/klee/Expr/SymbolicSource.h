#ifndef KLEE_SYMBOLICSOURCE_H
#define KLEE_SYMBOLICSOURCE_H

#include "klee/ADT/Ref.h"
#include <string>

namespace klee {

class Array;

class SymbolicSource {
public:
  /// @brief Required by klee::ref-managed objects
  class ReferenceCounter _refCount;

  enum class Kind {
    Constant,
    ConstantWithSymbolicSize,
    LazyInitializationSymbolic,
    MakeSymbolic,
    SymbolicAddress,
    SymbolicSize,
    Irreproducible,
    SymbolicValue
  };

public:
  virtual ~SymbolicSource() {}
  virtual Kind getKind() const = 0;
  virtual std::string getName() const = 0;
  virtual bool isSymcrete() const = 0;

  virtual int compare(const SymbolicSource &another) {
    return getKind() == another.getKind();
  }

  static bool classof(const SymbolicSource *) { return true; }
};

class ConstantSource : public SymbolicSource {
public:
  Kind getKind() const override { return Kind::Constant; }
  virtual std::string getName() const override { return "constant"; }
  virtual bool isSymcrete() const override { return false; }

  static bool classof(const SymbolicSource *S) {
    return S->getKind() == Kind::Constant;
  }
};

class ConstantWithSymbolicSizeSource : public SymbolicSource {
public:
  const unsigned defaultValue;
  ConstantWithSymbolicSizeSource(unsigned _defaultValue)
      : defaultValue(_defaultValue) {}

  Kind getKind() const override { return Kind::ConstantWithSymbolicSize; }
  virtual std::string getName() const override { return "constant"; }
  virtual bool isSymcrete() const override { return false; }

  static bool classof(const SymbolicSource *S) {
    return S->getKind() == Kind::ConstantWithSymbolicSize;
  }
};

class MakeSymbolicSource : public SymbolicSource {
public:
  Kind getKind() const override { return Kind::MakeSymbolic; }
  virtual std::string getName() const override { return "symbolic"; }
  virtual bool isSymcrete() const override { return false; }

  static bool classof(const SymbolicSource *S) {
    return S->getKind() == Kind::MakeSymbolic;
  }
};

class SymbolicAllocationSource : public SymbolicSource {
public:
  const Array *linkedArray;

  int compare(const SymbolicSource &another) {
    if (getKind() != another.getKind()) {
      return getKind() < another.getKind() ? -1 : 1;
    }
    const SymbolicAllocationSource *anotherCasted =
        cast<const SymbolicAllocationSource>(&another);
    if (linkedArray == anotherCasted->linkedArray) {
      return 0;
    }
    return linkedArray < anotherCasted->linkedArray ? -1 : 1;
  }

  static bool classof(const SymbolicSource *S) {
    return S->getKind() == Kind::SymbolicAddress ||
           S->getKind() == Kind::SymbolicSize;
  }
};

class SymbolicAddressSource : public SymbolicAllocationSource {
public:
  Kind getKind() const override { return Kind::SymbolicAddress; }
  virtual std::string getName() const override { return "symbolicAddress"; }
  virtual bool isSymcrete() const override { return true; }

  static bool classof(const SymbolicSource *S) {
    return S->getKind() == Kind::SymbolicAddress;
  }
};

class SymbolicSizeSource : public SymbolicAllocationSource {
public:
  Kind getKind() const override { return Kind::SymbolicSize; }
  virtual std::string getName() const override { return "symbolicSize"; }
  virtual bool isSymcrete() const override { return true; }

  static bool classof(const SymbolicSource *S) {
    return S->getKind() == Kind::SymbolicSize;
  }
};

class LazyInitializationSymbolicSource : public SymbolicSource {
public:
  Kind getKind() const override { return Kind::LazyInitializationSymbolic; }
  virtual std::string getName() const override {
    return "lazyInitializationSymbolic";
  }
  virtual bool isSymcrete() const override { return false; }

  static bool classof(const SymbolicSource *S) {
    return S->getKind() == Kind::LazyInitializationSymbolic;
  }
};

class IrreproducibleSource : public SymbolicSource {
public:
  Kind getKind() const override { return Kind::Irreproducible; }
  virtual std::string getName() const override { return "irreproducible"; }
  virtual bool isSymcrete() const override { return false; }

  static bool classof(const SymbolicSource *S) {
    return S->getKind() == Kind::Irreproducible;
  }
  static bool classof(const IrreproducibleSource *) { return true; }
};

class SymbolicValueSource : public SymbolicSource {
public:
  Kind getKind() const override { return Kind::SymbolicValue; }
  virtual std::string getName() const override { return "symbolicValue"; }
  virtual bool isSymcrete() const override { return false; }

  static bool classof(const SymbolicSource *S) {
    return S->getKind() == Kind::SymbolicValue;
  }
  static bool classof(const SymbolicValueSource *) { return true; }
};

} // namespace klee

#endif /* KLEE_SYMBOLICSOURCE_H */
