#ifndef KLEE_SYMBOLICSOURCE_H
#define KLEE_SYMBOLICSOURCE_H

#include "klee/ADT/Ref.h"

#include "klee/ADT/SparseStorage.h"
#include "klee/Support/CompilerWarning.h"

DISABLE_WARNING_PUSH
DISABLE_WARNING_DEPRECATED_DECLARATIONS
#include "llvm/ADT/StringExtras.h"
#include "llvm/IR/Argument.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Instruction.h"
DISABLE_WARNING_POP

#include <set>
#include <string>
#include <vector>

namespace klee {

class Array;
class Expr;
class ConstantExpr;
class KModule;
struct KInstruction;

class SymbolicSource {
protected:
  unsigned hashValue;

public:
  /// @brief Required by klee::ref-managed objects
  class ReferenceCounter _refCount;
  static const unsigned MAGIC_HASH_CONSTANT = 39;

  enum Kind {
    Constant = 3,
    Uninitialied,
    SymbolicSizeConstantAddress,
    MakeSymbolic,
    LazyInitializationContent,
    LazyInitializationAddress,
    LazyInitializationSize,
    Instruction,
    Argument,
    Irreproducible,
    Alpha
  };

public:
  virtual ~SymbolicSource() {}
  virtual Kind getKind() const = 0;
  virtual std::string getName() const = 0;
  virtual std::set<const Array *> getRelatedArrays() const { return {}; }

  static bool classof(const SymbolicSource *) { return true; }
  unsigned hash() const { return hashValue; }
  virtual unsigned computeHash() = 0;
  virtual void print(llvm::raw_ostream &os) const;
  void dump() const;
  std::string toString() const;
  virtual int internalCompare(const SymbolicSource &b) const = 0;
  int compare(const SymbolicSource &b) const;
  bool equals(const SymbolicSource &b) const;
};

class ConstantSource : public SymbolicSource {
public:
  const SparseStorage<ref<ConstantExpr>> constantValues;

  ConstantSource(SparseStorage<ref<ConstantExpr>> _constantValues)
      : constantValues(std::move(_constantValues)) {
    assert(constantValues.defaultV() && "Constant must be constant!");
  }

  Kind getKind() const override { return Kind::Constant; }

  std::string getName() const override { return "constant"; }

  static bool classof(const SymbolicSource *S) {
    return S->getKind() == Kind::Constant;
  }

  static bool classof(const ConstantSource *) { return true; }

  unsigned computeHash() override;

  int internalCompare(const SymbolicSource &b) const override {
    if (getKind() != b.getKind()) {
      return getKind() < b.getKind() ? -1 : 1;
    }
    const ConstantSource &cb = static_cast<const ConstantSource &>(b);
    if (constantValues != cb.constantValues) {
      return constantValues < cb.constantValues ? -1 : 1;
    }
    return 0;
  }
};

class UninitializedSource : public SymbolicSource {
public:
  const unsigned version;
  const KInstruction *allocSite;

  UninitializedSource(unsigned version, const KInstruction *allocSite)
      : version(version), allocSite(allocSite) {}

  Kind getKind() const override { return Kind::Uninitialied; }

  std::string getName() const override { return "uninitialized"; }

  static bool classof(const SymbolicSource *S) {
    return S->getKind() == Kind::Uninitialied;
  }

  static bool classof(const UninitializedSource *) { return true; }

  unsigned computeHash() override;

  int internalCompare(const SymbolicSource &b) const override;
};

class SymbolicSizeConstantAddressSource : public SymbolicSource {
public:
  const unsigned version;
  const KInstruction *allocSite;
  ref<Expr> size;
  SymbolicSizeConstantAddressSource(unsigned _version,
                                    const KInstruction *_allocSite,
                                    ref<Expr> _size)
      : version(_version), allocSite(_allocSite), size(_size) {}

  Kind getKind() const override { return Kind::SymbolicSizeConstantAddress; }
  virtual std::string getName() const override {
    return "symbolicSizeConstantAddress";
  }

  static bool classof(const SymbolicSource *S) {
    return S->getKind() == Kind::SymbolicSizeConstantAddress;
  }
  static bool classof(const SymbolicSizeConstantAddressSource *) {
    return true;
  }

  virtual unsigned computeHash() override;

  virtual int internalCompare(const SymbolicSource &b) const override;
};

class MakeSymbolicSource : public SymbolicSource {
public:
  const std::string name;
  const unsigned version;

  MakeSymbolicSource(const std::string &_name, unsigned _version)
      : name(_name), version(_version) {}
  Kind getKind() const override { return Kind::MakeSymbolic; }
  virtual std::string getName() const override { return "makeSymbolic"; }

  static bool classof(const SymbolicSource *S) {
    return S->getKind() == Kind::MakeSymbolic;
  }
  static bool classof(const MakeSymbolicSource *) { return true; }

  virtual unsigned computeHash() override;

  virtual int internalCompare(const SymbolicSource &b) const override {
    if (getKind() != b.getKind()) {
      return getKind() < b.getKind() ? -1 : 1;
    }
    const MakeSymbolicSource &mb = static_cast<const MakeSymbolicSource &>(b);
    if (version != mb.version) {
      return version < mb.version ? -1 : 1;
    }
    if (name != mb.name) {
      return name < mb.name ? -1 : 1;
    }
    return 0;
  }
};

class LazyInitializationSource : public SymbolicSource {
public:
  const ref<Expr> pointer;
  LazyInitializationSource(ref<Expr> _pointer) : pointer(_pointer) {}

  static bool classof(const SymbolicSource *S) {
    return S->getKind() == Kind::LazyInitializationAddress ||
           S->getKind() == Kind::LazyInitializationSize ||
           S->getKind() == Kind::LazyInitializationContent;
  }

  virtual unsigned computeHash() override;

  virtual int internalCompare(const SymbolicSource &b) const override {
    if (getKind() != b.getKind()) {
      return getKind() < b.getKind() ? -1 : 1;
    }
    const LazyInitializationSource &lib =
        static_cast<const LazyInitializationSource &>(b);
    if (pointer != lib.pointer) {
      return pointer < lib.pointer ? -1 : 1;
    }
    return 0;
  }

  virtual std::set<const Array *> getRelatedArrays() const override;
};

class LazyInitializationAddressSource : public LazyInitializationSource {
public:
  LazyInitializationAddressSource(ref<Expr> pointer)
      : LazyInitializationSource(pointer) {}
  Kind getKind() const override { return Kind::LazyInitializationAddress; }
  virtual std::string getName() const override {
    return "lazyInitializationAddress";
  }

  static bool classof(const SymbolicSource *S) {
    return S->getKind() == Kind::LazyInitializationAddress;
  }
  static bool classof(const LazyInitializationAddressSource *) { return true; }
};

class LazyInitializationSizeSource : public LazyInitializationSource {
public:
  LazyInitializationSizeSource(const ref<Expr> &pointer)
      : LazyInitializationSource(pointer) {}
  Kind getKind() const override { return Kind::LazyInitializationSize; }
  virtual std::string getName() const override {
    return "lazyInitializationSize";
  }

  static bool classof(const SymbolicSource *S) {
    return S->getKind() == Kind::LazyInitializationSize;
  }
  static bool classof(const LazyInitializationSizeSource *) { return true; }
};

class LazyInitializationContentSource : public LazyInitializationSource {
public:
  LazyInitializationContentSource(ref<Expr> pointer)
      : LazyInitializationSource(pointer) {}
  Kind getKind() const override { return Kind::LazyInitializationContent; }
  virtual std::string getName() const override {
    return "lazyInitializationContent";
  }

  static bool classof(const SymbolicSource *S) {
    return S->getKind() == Kind::LazyInitializationContent;
  }
  static bool classof(const LazyInitializationContentSource *) { return true; }
};

class ValueSource : public SymbolicSource {
public:
  const int index;
  const KModule *km;
  ValueSource(int _index, const KModule *km) : index(_index), km(km) {}

  static bool classof(const SymbolicSource *S) {
    return S->getKind() == Kind::Instruction || S->getKind() == Kind::Argument;
  }

  virtual const llvm::Value &value() const = 0;

  virtual unsigned computeHash() = 0;
};

class ArgumentSource : public ValueSource {
public:
  const llvm::Argument &allocSite;
  ArgumentSource(const llvm::Argument &_allocSite, int _index,
                 const KModule *km)
      : ValueSource(_index, km), allocSite(_allocSite) {}

  Kind getKind() const override { return Kind::Argument; }
  virtual std::string getName() const override { return "argument"; }

  static bool classof(const SymbolicSource *S) {
    return S->getKind() == Kind::Argument;
  }

  static bool classof(const ArgumentSource *S) { return true; }

  const llvm::Value &value() const override { return allocSite; }

  virtual unsigned computeHash() override;

  virtual int internalCompare(const SymbolicSource &b) const override;
};

class InstructionSource : public ValueSource {
public:
  const llvm::Instruction &allocSite;
  InstructionSource(const llvm::Instruction &_allocSite, int _index,
                    const KModule *km)
      : ValueSource(_index, km), allocSite(_allocSite) {}

  Kind getKind() const override { return Kind::Instruction; }
  virtual std::string getName() const override { return "instruction"; }

  static bool classof(const SymbolicSource *S) {
    return S->getKind() == Kind::Instruction;
  }

  static bool classof(const InstructionSource *S) { return true; }

  const llvm::Value &value() const override { return allocSite; }

  virtual unsigned computeHash() override;

  virtual int internalCompare(const SymbolicSource &b) const override;
};

class IrreproducibleSource : public SymbolicSource {
public:
  const std::string name;
  IrreproducibleSource(const std::string &_name) : name(_name) {}

  Kind getKind() const override { return Kind::Irreproducible; }
  virtual std::string getName() const override { return "irreproducible"; }

  static bool classof(const SymbolicSource *S) {
    return S->getKind() == Kind::Irreproducible;
  }
  static bool classof(const IrreproducibleSource *) { return true; }

  virtual unsigned computeHash() override {
    unsigned res = getKind();
    for (unsigned i = 0, e = name.size(); i != e; ++i) {
      res = (res * SymbolicSource::MAGIC_HASH_CONSTANT) + name[i];
    }
    hashValue = res;
    return hashValue;
  }

  virtual int internalCompare(const SymbolicSource &b) const override {
    if (getKind() != b.getKind()) {
      return getKind() < b.getKind() ? -1 : 1;
    }
    const IrreproducibleSource &irb =
        static_cast<const IrreproducibleSource &>(b);
    if (name != irb.name) {
      return name < irb.name ? -1 : 1;
    }
    return 0;
  }
};

class AlphaSource : public SymbolicSource {
public:
  const unsigned index;

  AlphaSource(unsigned _index) : index(_index) {}
  Kind getKind() const override { return Kind::Alpha; }
  virtual std::string getName() const override { return "alpha"; }

  static bool classof(const SymbolicSource *S) {
    return S->getKind() == Kind::Alpha;
  }
  static bool classof(const AlphaSource *) { return true; }

  virtual unsigned computeHash() override {
    unsigned res = getKind();
    hashValue = res ^ (index * SymbolicSource::MAGIC_HASH_CONSTANT);
    return hashValue;
  }

  virtual int internalCompare(const SymbolicSource &b) const override {
    if (getKind() != b.getKind()) {
      return getKind() < b.getKind() ? -1 : 1;
    }
    const AlphaSource &amb = static_cast<const AlphaSource &>(b);
    if (index != amb.index) {
      return index < amb.index ? -1 : 1;
    }
    return 0;
  }
};

} // namespace klee

#endif /* KLEE_SYMBOLICSOURCE_H */
