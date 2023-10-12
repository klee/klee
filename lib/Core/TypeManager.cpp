#include "TypeManager.h"

#include "klee/ADT/Ref.h"
#include "klee/Expr/Expr.h"
#include "klee/Expr/ExprHashMap.h"
#include "klee/Module/KInstruction.h"
#include "klee/Module/KModule.h"
#include "klee/Module/KType.h"

#include "klee/Support/CompilerWarning.h"
DISABLE_WARNING_PUSH
DISABLE_WARNING_DEPRECATED_DECLARATIONS
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/Casting.h"
DISABLE_WARNING_POP

#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace klee;

/**
 * Initializes type system with raw llvm types.
 */
TypeManager::TypeManager(KModule *parent) : parent(parent) {}

/**
 * Computes KType for given type, and cache it, if it was not
 * inititalized before. So, any two calls with the same argument
 * will return same KType's.
 */
KType *TypeManager::getWrappedType(llvm::Type *type) {
  if (typesMap.count(type) == 0) {
    types.emplace_back(new KType(type, this));
    typesMap.emplace(type, types.back().get());
    if (type && type->isPointerTy()) {
      getWrappedType(type->getPointerElementType());
    }
    if (type && type->isArrayTy()) {
      getWrappedType(type->getArrayElementType());
    }
  }
  return typesMap[type];
}

KType *TypeManager::getUnknownType() { return getWrappedType(nullptr); }

KType *TypeManager::handleAlloc(ref<Expr> size) { return getUnknownType(); }

KType *TypeManager::handleRealloc(KType *type, ref<Expr>) { return type; }

/**
 * Performs initialization for struct types, including inner types.
 * Note, that initialization for structs differs from initialization
 * for other types, as types from structs can create cyclic dependencies,
 * and that is why it cannot be done in constructor.
 */
void TypeManager::initTypesFromStructs() {
  /*
   * To collect information about all inner types
   * we will topologically sort dependencies between structures
   * (e.g. if struct A contains class B, we will make edge from A to B)
   * and pull types to top.
   */

  for (auto &structType : parent->module->getIdentifiedStructTypes()) {
    getWrappedType(structType);
  }

  std::unordered_set<llvm::StructType *> collectedStructTypes;
  for (const auto &it : typesMap) {
    if (llvm::StructType *itStruct =
            llvm::dyn_cast<llvm::StructType>(it.first)) {
      collectedStructTypes.insert(itStruct);
    }
  }

  for (auto &typesToOffsets : typesMap) {
    if (llvm::isa<llvm::StructType>(typesToOffsets.first)) {
      collectedStructTypes.insert(
          llvm::cast<llvm::StructType>(typesToOffsets.first));
    }
  }

  std::vector<llvm::StructType *> sortedStructTypesGraph;
  std::unordered_set<llvm::Type *> visitedStructTypesGraph;

  std::function<void(llvm::StructType *)> dfs = [this, &sortedStructTypesGraph,
                                                 &visitedStructTypesGraph,
                                                 &dfs](llvm::StructType *type) {
    visitedStructTypesGraph.insert(type);

    for (auto typeTo : type->elements()) {
      getWrappedType(typeTo);
      if (visitedStructTypesGraph.count(typeTo) == 0 && typeTo->isStructTy()) {
        dfs(llvm::cast<llvm::StructType>(typeTo));
      }
    }

    sortedStructTypesGraph.push_back(type);
  };

  for (auto &structType : collectedStructTypes) {
    dfs(structType);
  }

  for (auto structType : sortedStructTypesGraph) {
    if (structType->isOpaque()) {
      continue;
    }

    /* Here we make initializaion for inner types of given structure type */
    const llvm::StructLayout *structLayout =
        parent->targetData->getStructLayout(structType);
    for (unsigned idx = 0; idx < structType->getNumElements(); ++idx) {
      uint64_t offset = structLayout->getElementOffset(idx);
      llvm::Type *rawElementType = structType->getElementType(idx);
      typesMap[structType]->innerTypes[typesMap[rawElementType]].insert(offset);

      /* Provide initialization from types in inner class */
      for (auto &innerStructMemberTypesToOffsets :
           typesMap[rawElementType]->innerTypes) {
        KType *innerStructMemberType = innerStructMemberTypesToOffsets.first;
        const std::set<uint64_t> &innerTypeOffsets =
            innerStructMemberTypesToOffsets.second;

        /* Add offsets from inner class */
        for (uint64_t innerTypeOffset : innerTypeOffsets) {
          typesMap[structType]->innerTypes[innerStructMemberType].insert(
              offset + innerTypeOffset);
        }
      }
    }
  }
}

/**
 * Performs type system initialization for global objects.
 */
void TypeManager::initTypesFromGlobals() {
  for (auto &global : parent->module->getGlobalList()) {
    getWrappedType(global.getType());
  }
}

/**
 * Performs type system initialization for all instructions in
 * this module. Takes into consideration return and argument types.
 */
void TypeManager::initTypesFromInstructions() {
  for (auto &function : *(parent->module)) {
    for (auto &BasicBlock : function) {
      for (auto &inst : BasicBlock) {
        /* Register return type */
        getWrappedType(inst.getType());

        /* Register types for arguments */
        for (auto opb = inst.op_begin(), ope = inst.op_end(); opb != ope;
             ++opb) {
          getWrappedType((*opb)->getType());
        }
      }
    }
  }
}

void TypeManager::initTypeInfo() {
  for (auto &type : types) {
    llvm::Type *rawType = type->getRawType();
    if (rawType && rawType->isSized()) {
      type->alignment = parent->targetData->getABITypeAlignment(rawType);
      type->typeStoreSize = parent->targetData->getTypeStoreSize(rawType);
    }
  }
}

void TypeManager::onFinishInitModule() {}

/**
 * Method to initialize all types in given module.
 * Note, that it cannot be called in costructor
 * as implementation of getWrappedType can be different
 * for high-level languages. Note, that struct types is
 * called last, as it is required to know about all
 * structure types in code.
 */
void TypeManager::initModule() {
  initTypesFromGlobals();
  initTypesFromInstructions();
  initTypesFromStructs();
  initTypeInfo();
  onFinishInitModule();
}
