#include "klee/Module/KValue.h"
#include "klee/Support/CompilerWarning.h"

DISABLE_WARNING_PUSH
DISABLE_WARNING_DEPRECATED_DECLARATIONS
#include "llvm/ADT/StringRef.h"
#include "llvm/IR/Value.h"
DISABLE_WARNING_POP

#include <functional>
#include <string>

using namespace klee;

llvm::Value *KValue::unwrap() const { return value; }

llvm::StringRef KValue::getName() const {
  return value && value->hasName() ? value->getName() : "";
}
