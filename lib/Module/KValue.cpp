#include "klee/Module/KValue.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/IR/Value.h"

using namespace klee;

llvm::Value *KValue::unwrap() const { return value; }

llvm::StringRef KValue::getName() const {
  return value && value->hasName() ? value->getName() : "";
}
