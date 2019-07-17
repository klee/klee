//===-- Checks.cpp --------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "Passes.h"

#include "klee/Config/Version.h"

#include "KLEEIRMetaData.h"

#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/Pass.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

using namespace llvm;
using namespace klee;

char DivCheckPass::ID;

bool DivCheckPass::runOnModule(Module &M) {
  std::vector<llvm::BinaryOperator *> divInstruction;

  for (auto &F : M) {
    for (auto &BB : F) {
      for (auto &I : BB) {
        auto binOp = dyn_cast<BinaryOperator>(&I);
        if (!binOp)
          continue;

        // find all [s|u][div|rem] instructions
        auto opcode = binOp->getOpcode();
        if (opcode != Instruction::SDiv && opcode != Instruction::UDiv &&
            opcode != Instruction::SRem && opcode != Instruction::URem)
          continue;

        // Check if the operand is constant and not zero, skip in that case.
        const auto &operand = binOp->getOperand(1);
        if (const auto &coOp = dyn_cast<llvm::Constant>(operand)) {
          if (!coOp->isZeroValue())
            continue;
        }

        // Check if the operand is already checked by "klee_div_zero_check"
        if (KleeIRMetaData::hasAnnotation(I, "klee.check.div", "True"))
          continue;
        divInstruction.push_back(binOp);
      }
    }
  }

  // If nothing to do, return
  if (divInstruction.empty())
    return false;

  LLVMContext &ctx = M.getContext();
  KleeIRMetaData md(ctx);
  auto divZeroCheckFunction =
      M.getOrInsertFunction("klee_div_zero_check", Type::getVoidTy(ctx),
                            Type::getInt64Ty(ctx) KLEE_LLVM_GOIF_TERMINATOR);

  for (auto &divInst : divInstruction) {
    llvm::IRBuilder<> Builder(divInst /* Inserts before divInst*/);
    auto denominator =
        Builder.CreateIntCast(divInst->getOperand(1), Type::getInt64Ty(ctx),
                              false, /* sign doesn't matter */
                              "int_cast_to_i64");
    Builder.CreateCall(divZeroCheckFunction, denominator);
    md.addAnnotation(*divInst, "klee.check.div", "True");
  }

  return true;
}

char OvershiftCheckPass::ID;

bool OvershiftCheckPass::runOnModule(Module &M) {
  std::vector<llvm::BinaryOperator *> shiftInstructions;
  for (auto &F : M) {
    for (auto &BB : F) {
      for (auto &I : BB) {
        auto binOp = dyn_cast<BinaryOperator>(&I);
        if (!binOp)
          continue;

        // find all shift instructions
        auto opcode = binOp->getOpcode();
        if (opcode != Instruction::Shl && opcode != Instruction::LShr &&
            opcode != Instruction::AShr)
          continue;

        // Check if the operand is constant and not zero, skip in that case
        auto operand = binOp->getOperand(1);
        if (auto coOp = dyn_cast<llvm::ConstantInt>(operand)) {
          auto typeWidth =
              binOp->getOperand(0)->getType()->getScalarSizeInBits();
          // If the constant shift is positive and smaller,equal the type width,
          // we can ignore this instruction
          if (!coOp->isNegative() && coOp->getZExtValue() < typeWidth)
            continue;
        }

        if (KleeIRMetaData::hasAnnotation(I, "klee.check.shift", "True"))
          continue;

        shiftInstructions.push_back(binOp);
      }
    }
  }

  if (shiftInstructions.empty())
    return false;

  // Retrieve the checker function
  auto &ctx = M.getContext();
  KleeIRMetaData md(ctx);
  auto overshiftCheckFunction = M.getOrInsertFunction(
      "klee_overshift_check", Type::getVoidTy(ctx), Type::getInt64Ty(ctx),
      Type::getInt64Ty(ctx) KLEE_LLVM_GOIF_TERMINATOR);

  for (auto &shiftInst : shiftInstructions) {
    llvm::IRBuilder<> Builder(shiftInst);

    std::vector<llvm::Value *> args;

    // Determine bit width of first operand
    uint64_t bitWidth = shiftInst->getOperand(0)->getType()->getScalarSizeInBits();
    auto bitWidthC = ConstantInt::get(Type::getInt64Ty(ctx), bitWidth, false);
    args.push_back(bitWidthC);

    auto shiftValue =
        Builder.CreateIntCast(shiftInst->getOperand(1), Type::getInt64Ty(ctx),
                              false, /* sign doesn't matter */
                              "int_cast_to_i64");
    args.push_back(shiftValue);

    Builder.CreateCall(overshiftCheckFunction, args);
    md.addAnnotation(*shiftInst, "klee.check.shift", "True");
  }

  return true;
}
