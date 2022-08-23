#ifndef KLEE_KTYPE_H
#define KLEE_KTYPE_H

#include <stddef.h>
#include <stdint.h>
#include <unordered_map>
#include <vector>

namespace llvm {
class Type;
class raw_ostream;
} // namespace llvm

namespace klee {
class Expr;
class TypeManager;

template <class> class ref;

enum TypeSystemKind { LLVM };
class KType {
  friend TypeManager;

protected:
  /**
   * Represents type of used TypeManager. Required
   * for llvm RTTI.
   */
  TypeSystemKind typeSystemKind;

  /**
   * Wrapped type.
   */
  llvm::Type *type;

  /**
   * Owning type manager system. Note, that only it can
   * create instances of KTypes.
   */
  TypeManager *parent;

  /**
   * Innner types. Maps type to their offsets in current
   * type. Should contain type itself and
   * all types, that can be found in that object.
   * For example, if object of type A contains object
   * of type B, then all types in B can be accessed via A.
   */
  std::unordered_map<KType *, std::vector<uint64_t>> innerTypes;

  KType(llvm::Type *, TypeManager *);

  /**
   * Object cannot been created within class, defferent
   * from TypeManager, as it is important to have only
   * one instance for every llvm::Type.
   */
  KType(const KType &) = delete;
  KType &operator=(const KType &) = delete;
  KType(KType &&) = delete;
  KType &operator=(KType &&) = delete;

public:
  /**
   * Method to check if 2 types are compatible.
   */
  virtual bool isAccessableFrom(KType *accessingType) const;

  /**
   * Handler for possible memory access. By default does nothing.
   */
  virtual void handleMemoryAccess(KType *, ref<Expr>, ref<Expr>, bool isWrite);

  /**
   * Returns the stored raw llvm type.
   */
  llvm::Type *getRawType() const;

  TypeSystemKind getTypeSystemKind() const;

  virtual void print(llvm::raw_ostream &) const;

  virtual ~KType() = default;
};
} // namespace klee

#endif
