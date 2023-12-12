//===-- KCallable.h ---------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_KCALLABLE_H
#define KLEE_KCALLABLE_H

#include <string>

#include "klee/Module/KValue.h"
#include "klee/Support/CompilerWarning.h"
DISABLE_WARNING_PUSH
DISABLE_WARNING_DEPRECATED_DECLARATIONS
#include "llvm/ADT/Twine.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Support/Casting.h"
DISABLE_WARNING_POP

namespace klee {
/// Wrapper for callable objects passed in callExternalFunction
class KCallable : public KValue {
protected:
  KCallable(llvm::Value *value, KValue::Kind kind) : KValue(value, kind) {}

public:
  virtual llvm::FunctionType *getFunctionType() const = 0;

  static bool classof(const KValue *rhs) {
    return rhs->getKind() == KValue::Kind::FUNCTION ||
           rhs->getKind() == KValue::Kind::INLINE_ASM;
  }
};

class KInlineAsm : public KCallable {
private:
  /* Prepared name of ASM code */
  std::string name;

  static unsigned getFreshAsmId() {
    static unsigned globalId = 0;
    return globalId++;
  }

public:
  KInlineAsm(llvm::InlineAsm *inlineAsm)
      : KCallable(inlineAsm, KValue::Kind::INLINE_ASM),
        name("__asm__" + llvm::Twine(getFreshAsmId()).str()) {}

  llvm::StringRef getName() const override { return name; }

  llvm::FunctionType *getFunctionType() const override {
    return inlineAsm()->getFunctionType();
  }

  static bool classof(const KValue *rhs) {
    return rhs->getKind() == KValue::Kind::INLINE_ASM;
  }

  [[nodiscard]] llvm::InlineAsm *inlineAsm() const {
    return llvm::dyn_cast<llvm::InlineAsm>(value);
  }
};

} // namespace klee

#endif /* KLEE_KCALLABLE_H */
