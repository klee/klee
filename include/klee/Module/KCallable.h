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

#include "klee/Support/CompilerWarning.h"
DISABLE_WARNING_PUSH
DISABLE_WARNING_DEPRECATED_DECLARATIONS
#include "llvm/ADT/Twine.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/IR/LLVMContext.h"
DISABLE_WARNING_POP

namespace klee {
/// Wrapper for callable objects passed in callExternalFunction
class KCallable {
public:
  enum CallableKind { CK_Function, CK_InlineAsm };

private:
  const CallableKind Kind;

public:
  KCallable(CallableKind Kind) : Kind(Kind) {}

  CallableKind getKind() const { return Kind; }

  virtual llvm::StringRef getName() const = 0;
  virtual llvm::FunctionType *getFunctionType() const = 0;
  virtual llvm::Value *getValue() = 0;

  virtual ~KCallable() = default;
};

class KInlineAsm : public KCallable {
private:
  static unsigned getFreshAsmId() {
    static unsigned globalId = 0;
    return globalId++;
  }

  llvm::InlineAsm *value;
  std::string name;

public:
  KInlineAsm(llvm::InlineAsm *value)
      : KCallable(CK_InlineAsm), value(value),
        name("__asm__" + llvm::Twine(getFreshAsmId()).str()) {}

  llvm::StringRef getName() const override { return name; }

  llvm::FunctionType *getFunctionType() const override {
    return value->getFunctionType();
  }

  llvm::Value *getValue() override { return value; }

  static bool classof(const KCallable *callable) {
    return callable->getKind() == CK_InlineAsm;
  }

  llvm::InlineAsm *getInlineAsm() { return value; }
};

} // namespace klee

#endif /* KLEE_KCALLABLE_H */
