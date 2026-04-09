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

#include "klee/Support/CompilerWarning.h"

#include <llvm/Support/NativeFormatting.h>
DISABLE_WARNING_PUSH
DISABLE_WARNING_DEPRECATED_DECLARATIONS
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
DISABLE_WARNING_POP

using namespace llvm;
using namespace klee;

char SignedMultiArithOverflowCheckPass::ID;

bool SignedMultiArithOverflowCheckPass::runOnModule(Module &M) {
  std::vector<llvm::BinaryOperator *> overflowInstructions;

  for (auto &F : M) {
    for (auto &BB : F) {
      for (auto &I : BB) {
        auto binOp = dyn_cast<BinaryOperator>(&I);
        if (!binOp)
          continue;

        // find all [add|sub|mul] instructions
        auto opcode = binOp->getOpcode();
        if (opcode != Instruction::Add && opcode != Instruction::Sub &&
            opcode != Instruction::Mul)
          continue;

        // Check if the operand is already being checked for overflow
        if (KleeIRMetaData::hasAnnotation(I, "klee.check.overflow", "True"))
          continue;
        overflowInstructions.push_back(binOp);
      }
    }
  }

  // If nothing to do, return
  if (overflowInstructions.empty())
    return false;

  LLVMContext &ctx = M.getContext();
  KleeIRMetaData md(ctx);

  auto overflowCheckFunction =
      M.getOrInsertFunction("klee_signed_overflow_check", Type::getVoidTy(ctx),
                            Type::getInt1Ty(ctx),   // overflow flag
                            Type::getInt8PtrTy(ctx) // opcode string
      );

  for (auto &overflowInst : overflowInstructions) {
    llvm::Intrinsic::ID IID;

    llvm::IRBuilder<> Builder(overflowInst /* Inserts before arith op*/);
    llvm::IntegerType *Ty = cast<llvm::IntegerType>(overflowInst->getType());
    llvm::Value *opCodeStr =
        Builder.CreateGlobalStringPtr(overflowInst->getOpcodeName());

    llvm::Value *LHS = overflowInst->getOperand(0);
    llvm::Value *RHS = overflowInst->getOperand(1);

    switch (overflowInst->getOpcode()) {
    case Instruction::Add:
      IID = Intrinsic::sadd_with_overflow;
      break;
    case Instruction::Sub:
      IID = Intrinsic::ssub_with_overflow;
      break;
    case Instruction::Mul:
      IID = Intrinsic::smul_with_overflow;
      break;
    default:
      llvm_unreachable("Unsupported opcode");
    }

    // Replace all instances of respective instruction with overflow intrinsic
    llvm::Function *overflowIntrinsic = Intrinsic::getDeclaration(&M, IID, Ty);
    llvm::Value *pair = Builder.CreateCall(overflowIntrinsic, {LHS, RHS});

    // Collect the results of performing this intrinsic
    llvm::Value *result = Builder.CreateExtractValue(pair, {0});
    llvm::Value *overflow = Builder.CreateExtractValue(pair, {1});

    Builder.CreateCall(overflowCheckFunction, {overflow, opCodeStr});

    md.addAnnotation(*overflowInst, "klee.check.overflow", "True");
    overflowInst->replaceAllUsesWith(result);
    overflowInst->eraseFromParent();
  }

  return true;
}

char SignedDivOverflowCheckPass::ID;

bool SignedDivOverflowCheckPass::runOnModule(Module &M) {
  std::vector<llvm::BinaryOperator *> divInstruction;

  for (auto &F : M) {
    for (auto &BB : F) {
      for (auto &I : BB) {
        auto binOp = dyn_cast<BinaryOperator>(&I);
        if (!binOp)
          continue;

        // find all [s][div|rem] instructions
        auto opcode = binOp->getOpcode();
        if (opcode != Instruction::SDiv && opcode != Instruction::SRem)
          continue;

        /*
         * Check if the operands do not contain values, such that the operand
         * corresponding to the numerator is the smallest possible value
         * and the operand corresponding to the denominator is -1 (if constant
         * values)
         */
        llvm::Value *LHS = binOp->getOperand(0);
        llvm::Value *RHS = binOp->getOperand(1);

        if (auto *CLHS = llvm::dyn_cast<llvm::ConstantInt>(LHS)) {
          if (auto *CRHS = llvm::dyn_cast<llvm::ConstantInt>(RHS)) {
            bool isIntMin = CLHS->isMinSignedValue();
            bool isNegOne = CRHS->isAllOnesValue();

            if (!(isIntMin && isNegOne)) {
              continue;
            }
          }
        }

        // Check if the operand is already being checked for overflow
        if (KleeIRMetaData::hasAnnotation(I, "klee.check.overflow", "True"))
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

  auto overflowCheckFunction =
      M.getOrInsertFunction("klee_signed_overflow_check",
                            Type::getVoidTy(ctx),   // void
                            Type::getInt1Ty(ctx),   // overflow flag
                            Type::getInt8PtrTy(ctx) // opcode string
      );

  for (auto &divInst : divInstruction) {
    llvm::IRBuilder<> Builder(divInst /* Inserts before divInst*/);
    llvm::IntegerType *Ty = cast<llvm::IntegerType>(divInst->getType());

    llvm::Value *LHS = divInst->getOperand(0);
    llvm::Value *RHS = divInst->getOperand(1);
    llvm::Value *IntMin =
        Builder.getInt(llvm::APInt::getSignedMinValue(Ty->getBitWidth()));
    llvm::Value *NegOne = llvm::Constant::getAllOnesValue(Ty);

    llvm::Value *LHSCmp = Builder.CreateICmpEQ(LHS, IntMin);
    llvm::Value *RHSCmp = Builder.CreateICmpEQ(RHS, NegOne);
    llvm::Value *overflow = Builder.CreateAnd(LHSCmp, RHSCmp);

    llvm::Value *opcodeStr =
        Builder.CreateGlobalStringPtr(divInst->getOpcodeName());

    Builder.CreateCall(overflowCheckFunction, {overflow, opcodeStr});
    md.addAnnotation(*divInst, "klee.check.overflow", "True");
  }

  return true;
}

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
                            Type::getInt64Ty(ctx));

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
      Type::getInt64Ty(ctx));

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
