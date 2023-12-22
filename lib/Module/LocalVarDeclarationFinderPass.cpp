//===-- LocalVarDeclarationFinderPass.cpp -----------------------*- C++ -*-===//
//
//                     The KLEEF Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#include "Passes.h"

#include "klee/Support/CompilerWarning.h"

DISABLE_WARNING_PUSH
DISABLE_WARNING_DEPRECATED_DECLARATIONS
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/Casting.h"
DISABLE_WARNING_POP

using namespace klee;

////////////////////////////////////////////////

char LocalVarDeclarationFinderPass::ID = 0;

bool LocalVarDeclarationFinderPass::runOnFunction(llvm::Function &function) {
  bool anyChanged = false;

  for (const llvm::BasicBlock &block : function) {
    for (const llvm::Instruction &instruction : block) {
      if (const llvm::DbgDeclareInst *debugDeclareInstruction =
              llvm::dyn_cast<const llvm::DbgDeclareInst>(&instruction)) {
        llvm::Value *source = debugDeclareInstruction->getAddress();
        if (llvm::Instruction *sourceInstruction =
                llvm::dyn_cast<llvm::Instruction>(source)) {
          sourceInstruction->setDebugLoc(
              debugDeclareInstruction->getDebugLoc());
          anyChanged = true;
        }
      }
    }
  }

  return anyChanged;
}
