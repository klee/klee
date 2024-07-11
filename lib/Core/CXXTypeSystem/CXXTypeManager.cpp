#include "CXXTypeManager.h"
#include "../Memory.h"
#include "../TypeManager.h"

#include "klee/Expr/Constraints.h"
#include "klee/Expr/Expr.h"

#include "klee/Core/Context.h"
#include "klee/Module/KModule.h"
#include "klee/Module/KType.h"

#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/Casting.h"

#include <algorithm>
#include <cassert>
#include <utility>

using namespace llvm;
using namespace klee;

enum { DEMANGLER_BUFFER_SIZE = 4096, METADATA_SIZE = 16 };

CXXTypeManager::CXXTypeManager(KModule *parent) : TypeManager(parent) {}

/**
 * Factory method for KTypes. Note, that as there is no vector types in
 * C++, we interpret their type as their elements type.
 */
KType *CXXTypeManager::getWrappedType(llvm::Type *type) {
  if (typesMap.count(type) == 0) {
    cxxtypes::KCXXType *kt = nullptr;

    /* Special case when type is unknown */
    if (type == nullptr) {
      kt = new cxxtypes::KCXXType(type, this);
    } else {
      /* Vector types are considered as their elements type */
      llvm::Type *unwrappedRawType = type;
      if (unwrappedRawType->isVectorTy()) {
        unwrappedRawType =
            llvm::cast<llvm::VectorType>(unwrappedRawType)->getElementType();
      }

      switch (unwrappedRawType->getTypeID()) {
      case (llvm::Type::StructTyID):
        kt = new cxxtypes::KCXXStructType(unwrappedRawType, this);
        break;
      case (llvm::Type::IntegerTyID):
        kt = new cxxtypes::KCXXIntegerType(unwrappedRawType, this);
        break;
      case (llvm::Type::PPC_FP128TyID):
      case (llvm::Type::FP128TyID):
      case (llvm::Type::X86_FP80TyID):
      case (llvm::Type::DoubleTyID):
      case (llvm::Type::FloatTyID):
        kt = new cxxtypes::KCXXFloatingPointType(unwrappedRawType, this);
        break;
      case (llvm::Type::ArrayTyID):
        kt = new cxxtypes::KCXXArrayType(unwrappedRawType, this);
        break;
      case (llvm::Type::FunctionTyID):
        kt = new cxxtypes::KCXXFunctionType(unwrappedRawType, this);
        break;
      case (llvm::Type::PointerTyID):
        kt = new cxxtypes::KCXXPointerType(unwrappedRawType, this);
        break;
      default:
        kt = new cxxtypes::KCXXType(unwrappedRawType, this);
      }
    }

    types.emplace_back(kt);
    typesMap.emplace(type, kt);
  }
  return typesMap[type];
}

/**
 * We think about allocated memory as a memory without effective type,
 * i.e. with llvm::Type == nullptr.
 */
KType *CXXTypeManager::handleAlloc(ref<Expr> size) {
  cxxtypes::KCXXCompositeType *compositeType =
      new cxxtypes::KCXXCompositeType(getUnknownType(), this, size);
  types.emplace_back(compositeType);
  return compositeType;
}

/**
 * Creates a new type from this: copy all types from given
 * type, which lie in segment [address, addres + size).
 */
KType *CXXTypeManager::handleRealloc(KType *type, ref<Expr> size) {
  /**
   * C standard says that realloc can be called on memory, that were allocated
   * using malloc, calloc, memalign, or realloc. Therefore, let's fail execution
   * if this did not get appropriate pointer.
   */
  cxxtypes::KCXXCompositeType *reallocFromType =
      dyn_cast<cxxtypes::KCXXCompositeType>(type);
  assert(reallocFromType && "handleRealloc called on non CompositeType");

  cxxtypes::KCXXCompositeType *resultType =
      dyn_cast<cxxtypes::KCXXCompositeType>(handleAlloc(size));
  assert(resultType && "handleAlloc returned non CompositeType");

  /**
   * If we make realloc from simplified composite type or
   * allocated object with symbolic size, we will just return previous,
   * as we do not care about inner structure of given type anymore.
   */
  resultType->containsSymbolic = reallocFromType->containsSymbolic;
  ref<ConstantExpr> constantSize = llvm::dyn_cast<ConstantExpr>(size);
  if (reallocFromType->containsSymbolic || !constantSize) {
    resultType->insertedTypes = reallocFromType->insertedTypes;
    return resultType;
  }

  size_t sizeValue = constantSize->getZExtValue();
  for (auto offsetToTypeSizePair = reallocFromType->typesLocations.begin(),
            itEnd = reallocFromType->typesLocations.end();
       offsetToTypeSizePair != itEnd; ++offsetToTypeSizePair) {
    size_t prevOffset = offsetToTypeSizePair->first;
    KType *prevType = offsetToTypeSizePair->second.first;
    size_t prevSize = offsetToTypeSizePair->second.second;

    if (prevOffset < sizeValue) {
      resultType->handleMemoryAccess(
          prevType,
          ConstantExpr::alloc(prevOffset, Context::get().getPointerWidth()),
          ConstantExpr::alloc(prevSize, Context::get().getPointerWidth()),
          false);
    }
  }
  return resultType;
}

void CXXTypeManager::onFinishInitModule() {
  /* Here we try to get additional information from metadata
  for unions declared in global scope: we iterate through all
  metadata nodes availible for declarations until we meet one
  with type information. Then we check it tag. */
  for (auto &global : parent->module->globals()) {
    llvm::SmallVector<llvm::DIGlobalVariableExpression *, METADATA_SIZE>
        globalVariableInfo;
    global.getDebugInfo(globalVariableInfo);
    for (auto metaNode : globalVariableInfo) {
      llvm::DIGlobalVariable *variable = metaNode->getVariable();
      if (!variable) {
        continue;
      }

      llvm::DIType *type = variable->getType();

      if (!type) {
        continue;
      }

      if (type->getTag() == dwarf::Tag::DW_TAG_union_type) {
        KType *kt = getWrappedType(global.getValueType());
        llvm::cast<cxxtypes::KCXXStructType>(kt)->isUnion = true;
      }

      break;
    }
  }
}

/**
 * Util method to create constraints for pointers
 * for complex structures that can have many types located
 * by different offsets.
 */
static ref<Expr> getComplexPointerRestrictions(
    ref<Expr> object,
    const std::vector<std::pair<size_t, KType *>> &offsetsToTypes) {
  ConstraintSet restrictions;
  ref<Expr> resultCondition;
  for (auto &offsetToTypePair : offsetsToTypes) {
    size_t offset = offsetToTypePair.first;
    KType *extractedType = offsetToTypePair.second;

    ref<Expr> extractedOffset = ExtractExpr::create(
        object, offset * CHAR_BIT, extractedType->getSize() * CHAR_BIT);
    ref<Expr> innerAlignmentRequirement =
        llvm::cast<cxxtypes::KCXXType>(extractedType)
            ->getPointersRestrictions(extractedOffset);
    if (innerAlignmentRequirement.isNull()) {
      continue;
    }
    restrictions.addConstraint(innerAlignmentRequirement);
  }

  auto simplified = Simplificator::simplify(restrictions.cs());
  if (simplified.wasSimplified) {
    restrictions.changeCS(simplified.simplified);
  }
  for (auto restriction : restrictions.cs()) {
    if (resultCondition.isNull()) {
      resultCondition = restriction;
    } else {
      resultCondition = AndExpr::create(resultCondition, restriction);
    }
  }
  return resultCondition;
}

/* C++ KType base class */
cxxtypes::KCXXType::KCXXType(llvm::Type *type, TypeManager *parent)
    : KType(type, parent) {
  typeSystemKind = TypeSystemKind::Advanced;
  typeKind = DEFAULT;
}

bool cxxtypes::KCXXType::isAccessableFrom(KCXXType *) const { return true; }

bool cxxtypes::KCXXType::isAccessableFrom(KType *accessingType) const {
  KCXXType *accessingCXXType = dyn_cast_or_null<KCXXType>(accessingType);
  assert(accessingCXXType &&
         "Attempted to compare raw llvm type with C++ type!");
  return isAccessingFromChar(accessingCXXType) ||
         isAccessableFrom(accessingCXXType);
}

ref<Expr> cxxtypes::KCXXType::getContentRestrictions(ref<Expr> object) const {
  if (type == nullptr) {
    return nullptr;
  }
  llvm::Type *elementType = type->getPointerElementType();
  return llvm::cast<cxxtypes::KCXXType>(parent->getWrappedType(elementType))
      ->getPointersRestrictions(object);
}

ref<Expr> cxxtypes::KCXXType::getPointersRestrictions(ref<Expr>) const {
  return nullptr;
}

bool cxxtypes::KCXXType::isAccessingFromChar(KCXXType *accessingType) {
  /* Special case for unknown type */
  if (accessingType->getRawType() == nullptr) {
    return true;
  }

  assert(llvm::isa<KCXXPointerType>(accessingType) &&
         "Attempt to access to a memory via non-pointer type");

  return llvm::cast<KCXXPointerType>(accessingType)->isPointerToChar();
}

cxxtypes::CXXTypeKind cxxtypes::KCXXType::getTypeKind() const {
  return typeKind;
}

bool cxxtypes::KCXXType::classof(const KType *requestedType) {
  return requestedType->getTypeSystemKind() == TypeSystemKind::Advanced;
}

/* Composite type */
cxxtypes::KCXXCompositeType::KCXXCompositeType(KType *type, TypeManager *parent,
                                               ref<Expr> objectSize)
    : KCXXType(type->getRawType(), parent) {
  typeKind = CXXTypeKind::COMPOSITE;

  if (ref<ConstantExpr> CE = llvm::dyn_cast<ConstantExpr>(objectSize)) {
    size_t size = CE->getZExtValue();
    if (type->getRawType() == nullptr) {
      ++nonTypedMemorySegments[size];
    }
    typesLocations[0] = std::make_pair(type, size);
  } else {
    containsSymbolic = true;
  }
  insertedTypes.emplace(type);
}

ref<Expr>
cxxtypes::KCXXCompositeType::getPointersRestrictions(ref<Expr> object) const {
  if (containsSymbolic) {
    return nullptr;
  }
  std::vector<std::pair<size_t, KType *>> offsetToTypes;
  for (auto &offsetToTypePair : typesLocations) {
    offsetToTypes.emplace_back(offsetToTypePair.first,
                               offsetToTypePair.second.first);
  }
  return getComplexPointerRestrictions(object, offsetToTypes);
}

void cxxtypes::KCXXCompositeType::handleMemoryAccess(KType *type,
                                                     ref<Expr> offset,
                                                     ref<Expr> size,
                                                     bool isWrite) {
  if (cxxtypes::KCXXPointerType *pointerType =
          llvm::dyn_cast<KCXXPointerType>(type)) {
    if (isWrite && pointerType->isPointerToChar()) {
      return;
    }
  }

  /*
   * We want to check adjacent types to ensure, that we did not overlapped
   * nothing, and if we overlapped, move bounds for types or even remove them.
   */
  ref<ConstantExpr> offsetConstant = dyn_cast<ConstantExpr>(offset);
  ref<ConstantExpr> sizeConstant = dyn_cast<ConstantExpr>(size);

  if (offsetConstant && sizeConstant && !containsSymbolic) {
    size_t offsetValue = offsetConstant->getZExtValue();
    size_t sizeValue = sizeConstant->getZExtValue();

    /* We support C-style principle of effective type.
    This might be not appropriate for C++, as C++ has
    "placement new" operator, but in case of C it is OK.
    Therefore we assume, that we write in memory with no
    effective type, i.e. do not overloap objects placed in this
    object before. */

    auto it = std::prev(typesLocations.upper_bound(offsetValue));

    /* We do not overwrite types in memory */
    if (it->second.first->getRawType() != nullptr) {
      return;
    }
    if (std::next(it) != typesLocations.end() &&
        std::next(it)->first < offsetValue + sizeValue) {
      return;
    }

    size_t tail = 0;

    size_t prevOffsetValue = it->first;
    size_t prevSizeValue = it->second.second;

    /* Calculate number of non-typed bytes after object */
    if (prevOffsetValue + prevSizeValue > offsetValue + sizeValue) {
      tail = (prevOffsetValue + prevSizeValue) - (offsetValue + sizeValue);
    }

    /* We will possibly cut this object belowe. So let's
    decrease counter immediately */
    if (--nonTypedMemorySegments[prevSizeValue] == 0) {
      nonTypedMemorySegments.erase(prevSizeValue);
    }

    /* Calculate space remaining for non-typed memory in the beginning */
    if (offsetValue - prevOffsetValue != 0) {
      it->second.second =
          std::min(prevSizeValue, offsetValue - prevOffsetValue);
      ++nonTypedMemorySegments[prevSizeValue];
    } else {
      typesLocations.erase(it);
    }

    typesLocations[offsetValue] = std::make_pair(type, sizeValue);
    if (typesLocations.count(offsetValue + sizeValue) == 0 && tail != 0) {
      ++nonTypedMemorySegments[tail];
      typesLocations[offsetValue + sizeValue] =
          std::make_pair(parent->getUnknownType(), tail);
    }

    if (nonTypedMemorySegments.empty()) {
      insertedTypes.erase(parent->getUnknownType());
    }
  } else {
    /*
     * If we have written object by a symbolic address, we will use
     * simplified representation for Composite Type, as it is too
     * dificult to determine relative location of objects in memory
     * (requires query the solver at least).
     */
    containsSymbolic = true;
  }
  /* We do not want to add nullptr type to composite type */
  if (type->getRawType() != nullptr) {
    insertedTypes.emplace(type);
  }
}

bool cxxtypes::KCXXCompositeType::isAccessableFrom(
    KCXXType *accessingType) const {
  for (auto &it : insertedTypes) {
    if (it->isAccessableFrom(accessingType)) {
      return true;
    }
  }
  return false;
}

bool cxxtypes::KCXXCompositeType::classof(const KType *requestedType) {
  return llvm::isa<KCXXType>(requestedType) &&
         llvm::cast<KCXXType>(requestedType)->getTypeKind() ==
             cxxtypes::CXXTypeKind::COMPOSITE;
}

/* Integer type */
cxxtypes::KCXXIntegerType::KCXXIntegerType(llvm::Type *type,
                                           TypeManager *parent)
    : KCXXType(type, parent) {
  typeKind = CXXTypeKind::INTEGER;
}

bool cxxtypes::KCXXIntegerType::isAccessableFrom(
    KCXXType *accessingType) const {
  if (llvm::isa<KCXXIntegerType>(accessingType)) {
    return innerIsAccessableFrom(cast<KCXXIntegerType>(accessingType));
  }
  return innerIsAccessableFrom(accessingType);
}

bool cxxtypes::KCXXIntegerType::innerIsAccessableFrom(
    KCXXType *accessingType) const {
  return accessingType->getRawType() == nullptr;
}

bool cxxtypes::KCXXIntegerType::innerIsAccessableFrom(
    KCXXIntegerType *accessingType) const {
  return accessingType->type == type;
}

bool cxxtypes::KCXXIntegerType::classof(const KType *requestedType) {
  return llvm::isa<KCXXType>(requestedType) &&
         llvm::cast<KCXXType>(requestedType)->getTypeKind() ==
             cxxtypes::CXXTypeKind::INTEGER;
}

/* Floating point type */
cxxtypes::KCXXFloatingPointType::KCXXFloatingPointType(llvm::Type *type,
                                                       TypeManager *parent)
    : KCXXType(type, parent) {
  typeKind = CXXTypeKind::FP;
}

bool cxxtypes::KCXXFloatingPointType::isAccessableFrom(
    KCXXType *accessingType) const {
  if (llvm::isa<KCXXFloatingPointType>(accessingType)) {
    return innerIsAccessableFrom(cast<KCXXFloatingPointType>(accessingType));
  }
  return innerIsAccessableFrom(accessingType);
}

bool cxxtypes::KCXXFloatingPointType::innerIsAccessableFrom(
    KCXXType *accessingType) const {
  return accessingType->getRawType() == nullptr;
}

bool cxxtypes::KCXXFloatingPointType::innerIsAccessableFrom(
    KCXXFloatingPointType *accessingType) const {
  return accessingType->getRawType() == type;
}

bool cxxtypes::KCXXFloatingPointType::classof(const KType *requestedType) {
  return llvm::isa<KCXXType>(requestedType) &&
         llvm::cast<KCXXType>(requestedType)->getTypeKind() ==
             cxxtypes::CXXTypeKind::FP;
}

/* Struct type */
cxxtypes::KCXXStructType::KCXXStructType(llvm::Type *type, TypeManager *parent)
    : KCXXType(type, parent) {
  typeKind = CXXTypeKind::STRUCT;
  /* Hard coded union identification, as we can not always
  get this info from metadata. */
  llvm::StructType *structType = llvm::cast<StructType>(type);
  if (structType->hasName()) {
    isUnion = structType->getStructName().startswith("union.");
  }
}

ref<Expr>
cxxtypes::KCXXStructType::getPointersRestrictions(ref<Expr> object) const {
  std::vector<std::pair<size_t, KType *>> offsetsToTypes;
  for (auto &innerTypeToOffsets : innerTypes) {
    KType *innerType = innerTypeToOffsets.first;
    if (!llvm::isa<KCXXPointerType>(innerType)) {
      continue;
    }
    for (auto &offset : innerTypeToOffsets.second) {
      offsetsToTypes.emplace_back(offset, innerType);
    }
  }
  return getComplexPointerRestrictions(object, offsetsToTypes);
}

bool cxxtypes::KCXXStructType::isAccessableFrom(KCXXType *accessingType) const {
  /* FIXME: this is a temporary hack for vtables in C++. Ideally, we
   * should demangle global variables to get additional info, at least
   * that global object is "special" (here it is about vtable).
   */
  if (llvm::isa<KCXXPointerType>(accessingType) &&
      llvm::cast<KCXXPointerType>(accessingType)->isPointerToFunction()) {
    return true;
  }

  if (isUnion) {
    return true;
  }

  for (auto &innerTypesToOffsets : innerTypes) {
    KCXXType *innerType = cast<KCXXType>(innerTypesToOffsets.first);

    /* To prevent infinite recursion */
    if (isa<KCXXStructType>(innerType)) {
      if (innerType == accessingType) {
        return true;
      }
    } else if (innerType->isAccessableFrom(accessingType)) {
      return true;
    }
  }
  return false;
}

bool cxxtypes::KCXXStructType::classof(const KType *requestedType) {
  return llvm::isa<KCXXType>(requestedType) &&
         llvm::cast<KCXXType>(requestedType)->getTypeKind() ==
             cxxtypes::CXXTypeKind::STRUCT;
}

/* Array type */
cxxtypes::KCXXArrayType::KCXXArrayType(llvm::Type *type, TypeManager *parent)
    : KCXXType(type, parent) {
  typeKind = CXXTypeKind::ARRAY;

  llvm::Type *rawArrayType = llvm::cast<llvm::ArrayType>(type);
  KType *elementKType =
      parent->getWrappedType(rawArrayType->getArrayElementType());
  assert(llvm::isa<KCXXType>(elementKType) &&
         "Type manager returned non CXX type for array element");
  elementType = cast<KCXXType>(elementKType);
  arrayElementsCount = rawArrayType->getArrayNumElements();
}

ref<Expr>
cxxtypes::KCXXArrayType::getPointersRestrictions(ref<Expr> object) const {
  std::vector<std::pair<size_t, KType *>> offsetsToTypes;
  for (unsigned idx = 0; idx < arrayElementsCount; ++idx) {
    offsetsToTypes.emplace_back(idx * elementType->getSize(), elementType);
  }
  return getComplexPointerRestrictions(object, offsetsToTypes);
}

bool cxxtypes::KCXXArrayType::isAccessableFrom(KCXXType *accessingType) const {
  if (llvm::isa<KCXXArrayType>(accessingType)) {
    return innerIsAccessableFrom(llvm::cast<KCXXArrayType>(accessingType));
  }
  return innerIsAccessableFrom(accessingType);
}

bool cxxtypes::KCXXArrayType::innerIsAccessableFrom(
    KCXXType *accessingType) const {
  return (accessingType->getRawType() == nullptr) ||
         elementType->isAccessableFrom(accessingType);
}

bool cxxtypes::KCXXArrayType::innerIsAccessableFrom(
    KCXXArrayType *accessingType) const {
  /* Arrays of unknown size in llvm has 1 element inside */
  return (arrayElementsCount == accessingType->arrayElementsCount ||
          arrayElementsCount == 1 || accessingType->arrayElementsCount == 1) &&
         elementType->isAccessableFrom(accessingType->elementType);
}

bool cxxtypes::KCXXArrayType::classof(const KType *requestedType) {
  return llvm::isa<KCXXType>(requestedType) &&
         llvm::cast<KCXXType>(requestedType)->getTypeKind() ==
             cxxtypes::CXXTypeKind::ARRAY;
}

/* Function type */
cxxtypes::KCXXFunctionType::KCXXFunctionType(llvm::Type *type,
                                             TypeManager *parent)
    : KCXXType(type, parent) {
  typeKind = CXXTypeKind::FUNCTION;

  assert(type->isFunctionTy() &&
         "Given non-function type to construct KFunctionType!");
  llvm::FunctionType *function = llvm::cast<llvm::FunctionType>(type);
  returnType = llvm::dyn_cast<cxxtypes::KCXXType>(
      parent->getWrappedType(function->getReturnType()));
  assert(returnType != nullptr && "Type manager returned non CXXKType");

  for (auto argType : function->params()) {
    KType *argKType = parent->getWrappedType(argType);
    assert(llvm::isa<cxxtypes::KCXXType>(argKType) &&
           "Type manager return non CXXType for function argument");
    arguments.push_back(cast<cxxtypes::KCXXType>(argKType));
  }
}

bool cxxtypes::KCXXFunctionType::isAccessableFrom(
    KCXXType *accessingType) const {
  if (llvm::isa<KCXXFunctionType>(accessingType)) {
    return innerIsAccessableFrom(llvm::cast<KCXXFunctionType>(accessingType));
  }
  return innerIsAccessableFrom(accessingType);
}

bool cxxtypes::KCXXFunctionType::innerIsAccessableFrom(
    KCXXType *accessingType) const {
  return accessingType->getRawType() == nullptr;
}

bool cxxtypes::KCXXFunctionType::innerIsAccessableFrom(
    KCXXFunctionType *accessingType) const {
  unsigned currentArgCount = type->getFunctionNumParams();
  unsigned accessingArgCount = accessingType->type->getFunctionNumParams();

  if (!type->isFunctionVarArg() && currentArgCount != accessingArgCount) {
    return false;
  }

  for (unsigned idx = 0; idx < std::min(currentArgCount, accessingArgCount);
       ++idx) {
    if (type->getFunctionParamType(idx) !=
        accessingType->type->getFunctionParamType(idx)) {
      return false;
    }
  }

  /*
   * FIXME: We need to check return value, but it can differ though in llvm IR.
   * E.g., first member in structs is i32 (...), that can be accessed later
   * by void (...). Need a research how to maintain it properly.
   */
  return true;
}

bool cxxtypes::KCXXFunctionType::classof(const KType *requestedType) {
  return llvm::isa<KCXXType>(requestedType) &&
         llvm::cast<KCXXType>(requestedType)->getTypeKind() ==
             cxxtypes::CXXTypeKind::FUNCTION;
}

/* Pointer type */
cxxtypes::KCXXPointerType::KCXXPointerType(llvm::Type *type,
                                           TypeManager *parent)
    : KCXXType(type, parent) {
  typeKind = CXXTypeKind::POINTER;

  elementType =
      cast<KCXXType>(parent->getWrappedType(type->getPointerElementType()));
}

bool cxxtypes::KCXXPointerType::isAccessableFrom(
    KCXXType *accessingType) const {
  if (llvm::isa<KCXXPointerType>(accessingType)) {
    return innerIsAccessableFrom(llvm::cast<KCXXPointerType>(accessingType));
  }
  return innerIsAccessableFrom(accessingType);
}

bool cxxtypes::KCXXPointerType::innerIsAccessableFrom(
    KCXXType *accessingType) const {
  return accessingType->getRawType() == nullptr;
}

ref<Expr>
cxxtypes::KCXXPointerType::getPointersRestrictions(ref<Expr> object) const {
  /**
   * We assume that alignment is always a power of 2 and has
   * a bit representation as 00...010...00. By subtracting 1
   * we are getting 00...011...1. Then we apply this mask to
   * address and require, that bitwise AND should give 0 (i.e.
   * non of the last bits is 1).
   */
  ref<Expr> appliedAlignmentMask = AndExpr::create(
      Expr::createPointer(elementType->getAlignment() - 1), object);

  ref<Expr> sizeExpr = Expr::createPointer(elementType->getSize() - 1);

  ref<Expr> objectUpperBound = AddExpr::create(object, sizeExpr);

  return AndExpr::create(Expr::createIsZero(appliedAlignmentMask),
                         UgeExpr::create(objectUpperBound, sizeExpr));
}

bool cxxtypes::KCXXPointerType::innerIsAccessableFrom(
    KCXXPointerType *accessingType) const {
  return elementType->isAccessableFrom(accessingType->elementType);
}

bool cxxtypes::KCXXPointerType::isPointerToChar() const {
  if (llvm::isa<KCXXIntegerType>(elementType)) {
    return (elementType->getRawType()->getIntegerBitWidth() == 8);
  }
  return false;
}

bool cxxtypes::KCXXPointerType::isPointerToFunction() const {
  return llvm::isa<KCXXFunctionType>(elementType);
}

bool cxxtypes::KCXXPointerType::classof(const KType *requestedType) {
  return llvm::isa<KCXXType>(requestedType) &&
         llvm::cast<KCXXType>(requestedType)->getTypeKind() ==
             cxxtypes::CXXTypeKind::POINTER;
}
