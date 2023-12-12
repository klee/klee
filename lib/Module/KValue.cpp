#include "klee/Support/CompilerWarning.h"
#include <klee/Module/KValue.h>

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

bool KValue::operator<(const KValue &rhs) const {
  if (getName() != rhs.getName()) {
    return getName() < rhs.getName();
  }
  return std::less<llvm::Value *>{}(unwrap(), rhs.unwrap());
}

[[nodiscard]] unsigned KValue::hash() const {
  return std::hash<std::string>{}(getName().str());
}
