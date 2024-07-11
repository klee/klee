#ifndef KLEE_CXXTYPEMANAGER_H
#define KLEE_CXXTYPEMANAGER_H

#include "../TypeManager.h"

#include "klee/Module/KType.h"

#include <map>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace llvm {
class Type;
class Function;
} // namespace llvm

namespace klee {
class Expr;
class KModule;
template <class> class ref;

namespace cxxtypes {

enum CXXTypeKind {
  DEFAULT,
  COMPOSITE,
  STRUCT,
  INTEGER,
  FP,
  ARRAY,
  POINTER,
  FUNCTION
};

class KCXXCompositeType;
class KCXXStructType;
class KCXXIntegerType;
class KCXXFloatingPointType;
class KCXXArrayType;
class KCXXPointerType;
class KCXXType;
class KCXXFunctionType;
} // namespace cxxtypes

class CXXTypeManager final : public TypeManager {
protected:
  virtual void onFinishInitModule() override;

public:
  CXXTypeManager(KModule *);

  virtual KType *getWrappedType(llvm::Type *) override;
  virtual KType *handleAlloc(ref<Expr>) override;
  virtual KType *handleRealloc(KType *, ref<Expr>) override;
};

/**
 * Classes for cpp type system. Rules described below
 * relies on Strict-Aliasing-Rule. See more on this page:
 * https://en.cppreference.com/w/cpp/language/reinterpret_cast
 */
namespace cxxtypes {

class KCXXType : public KType {
  friend CXXTypeManager;
  friend KCXXStructType;

private:
  static bool isAccessingFromChar(KCXXType *accessingType);

protected:
  /**
   * Field for llvm RTTI system.
   */
  CXXTypeKind typeKind;

  KCXXType(llvm::Type *, TypeManager *);

public:
  /**
   * Checks the first access to this type from specified.
   * Using isAccessingFromChar and then cast parameter
   * to KCXXType. Method is declared as final because it
   * is an 'edge' between LLVM and CXX type systems.
   */
  virtual bool isAccessableFrom(KType *) const final override;
  virtual bool isAccessableFrom(KCXXType *) const;

  virtual ref<Expr> getContentRestrictions(ref<Expr>) const override;
  virtual ref<Expr> getPointersRestrictions(ref<Expr>) const;

  CXXTypeKind getTypeKind() const;

  static bool classof(const KType *);
};

/**
 * Composite type represents multuple kinds of types in one memory
 * location. E.g., this type can apper if we use placement new
 * on array.
 */
class KCXXCompositeType : public KCXXType {
  friend CXXTypeManager;

private:
  bool containsSymbolic = false;
  std::unordered_map<size_t, unsigned> nonTypedMemorySegments;

  std::map<size_t, std::pair<KType *, size_t>> typesLocations;
  std::unordered_set<KType *> insertedTypes;

protected:
  KCXXCompositeType(KType *, TypeManager *, ref<Expr>);

public:
  virtual ref<Expr> getPointersRestrictions(ref<Expr>) const override;
  virtual void handleMemoryAccess(KType *, ref<Expr>, ref<Expr> size,
                                  bool isWrite) override;
  virtual bool isAccessableFrom(KCXXType *) const override;

  static bool classof(const KType *);
};

/**
 * Struct type can be accessed from all other types, that
 * it might contain inside. Holds additional information
 * about types inside.
 */
class KCXXStructType : public KCXXType {
  friend CXXTypeManager;

private:
  bool isUnion = false;

protected:
  KCXXStructType(llvm::Type *, TypeManager *);

public:
  virtual ref<Expr> getPointersRestrictions(ref<Expr>) const override;
  virtual bool isAccessableFrom(KCXXType *) const override;

  static bool classof(const KType *);
};

/**
 * Function type can be accessed obly from another
 * function type.
 */
class KCXXFunctionType : public KCXXType {
  friend CXXTypeManager;

private:
  KCXXType *returnType;
  std::vector<KType *> arguments;

  bool innerIsAccessableFrom(KCXXType *) const;
  bool innerIsAccessableFrom(KCXXFunctionType *) const;

protected:
  KCXXFunctionType(llvm::Type *, TypeManager *);

public:
  virtual bool isAccessableFrom(KCXXType *) const override;

  static bool classof(const KType *);
};

/**
 * Integer type can be accessed from another integer type
 * of the same type.
 */
class KCXXIntegerType : public KCXXType {
  friend CXXTypeManager;

private:
  bool innerIsAccessableFrom(KCXXType *) const;
  bool innerIsAccessableFrom(KCXXIntegerType *) const;

protected:
  KCXXIntegerType(llvm::Type *, TypeManager *);

public:
  virtual bool isAccessableFrom(KCXXType *) const override;

  static bool classof(const KType *);
};

/**
 * Floating point type can be access from another floating
 * point type of the same type.
 */
class KCXXFloatingPointType : public KCXXType {
  friend CXXTypeManager;

private:
  bool innerIsAccessableFrom(KCXXType *) const;
  bool innerIsAccessableFrom(KCXXFloatingPointType *) const;

protected:
  KCXXFloatingPointType(llvm::Type *, TypeManager *);

public:
  virtual bool isAccessableFrom(KCXXType *) const override;

  static bool classof(const KType *);
};

/**
 * Array type can be accessed from another array type of the
 * same size or from type of its elements. Types of array elements
 * must be the same.
 */
class KCXXArrayType : public KCXXType {
  friend CXXTypeManager;

private:
  KCXXType *elementType;
  size_t arrayElementsCount;

  bool innerIsAccessableFrom(KCXXType *) const;
  bool innerIsAccessableFrom(KCXXArrayType *) const;

protected:
  KCXXArrayType(llvm::Type *, TypeManager *);

public:
  virtual ref<Expr> getPointersRestrictions(ref<Expr>) const override;
  virtual bool isAccessableFrom(KCXXType *) const override;

  static bool classof(const KType *);
};

/**
 * Pointer Type can be accessed from another pointer type.
 * Pointer elements type must be the same.
 */
class KCXXPointerType : public KCXXType {
  friend CXXTypeManager;

private:
  KCXXType *elementType;
  bool innerIsAccessableFrom(KCXXType *) const;
  bool innerIsAccessableFrom(KCXXPointerType *) const;

protected:
  KCXXPointerType(llvm::Type *, TypeManager *);

public:
  virtual ref<Expr> getPointersRestrictions(ref<Expr>) const override;
  virtual bool isAccessableFrom(KCXXType *) const override;

  bool isPointerToChar() const;
  bool isPointerToFunction() const;

  static bool classof(const KType *);
};

} /*namespace cxxtypes*/

} /*namespace klee*/

#endif /*KLEE_CXXTYPEMANAGER_H*/
