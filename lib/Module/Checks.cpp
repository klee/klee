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
      }
    }
  }

  // If nothing to do, return
  if (divInstruction.empty())
    return false;

  LLVMContext &ctx = M.getContext();
  auto divZeroCheckFunction = cast<Function>(
      M.getOrInsertFunction("klee_div_zero_check", Type::getVoidTy(ctx),
                            Type::getInt64Ty(ctx), NULL));

  for (auto &divInst : divInstruction) {
    llvm::IRBuilder<> Builder(divInst /* Inserts before divInst*/);
    auto denominator =
        Builder.CreateIntCast(divInst->getOperand(1), Type::getInt64Ty(ctx),
                              false, /* sign doesn't matter */
                              "int_cast_to_i64");
    Builder.CreateCall(divZeroCheckFunction, denominator);
  }

  return true;
}

char OvershiftCheckPass::ID;

bool OvershiftCheckPass::runOnModule(Module &M) {
  Function *overshiftCheckFunction = 0;
  LLVMContext &ctx = M.getContext();

  bool moduleChanged = false;

  for (Module::iterator f = M.begin(), fe = M.end(); f != fe; ++f) {
    for (Function::iterator b = f->begin(), be = f->end(); b != be; ++b) {
      for (BasicBlock::iterator i = b->begin(), ie = b->end(); i != ie; ++i) {
          if (BinaryOperator* binOp = dyn_cast<BinaryOperator>(i)) {
          // find all shift instructions
          Instruction::BinaryOps opcode = binOp->getOpcode();

          if (opcode == Instruction::Shl ||
              opcode == Instruction::LShr ||
              opcode == Instruction::AShr ) {
            std::vector<llvm::Value*> args;

            // Determine bit width of first operand
            uint64_t bitWidth=i->getOperand(0)->getType()->getScalarSizeInBits();

            ConstantInt *bitWidthC = ConstantInt::get(Type::getInt64Ty(ctx),
		bitWidth, false);
            args.push_back(bitWidthC);

            CastInst *shift =
              CastInst::CreateIntegerCast(i->getOperand(1),
                                          Type::getInt64Ty(ctx),
                                          false,  /* sign doesn't matter */
                                          "int_cast_to_i64",
                                          &*i);
            args.push_back(shift);


            // Lazily bind the function to avoid always importing it.
            if (!overshiftCheckFunction) {
              Constant *fc = M.getOrInsertFunction("klee_overshift_check",
                                                   Type::getVoidTy(ctx),
                                                   Type::getInt64Ty(ctx),
                                                   Type::getInt64Ty(ctx),
                                                   NULL);
              overshiftCheckFunction = cast<Function>(fc);
            }

            // Inject CallInstr to check if overshifting possible
            CallInst *ci =
                CallInst::Create(overshiftCheckFunction, args, "", &*i);
            // set debug information from binary operand to preserve it
            ci->setDebugLoc(binOp->getDebugLoc());
            moduleChanged = true;
          }
        }
      }
    }
  }
  return moduleChanged;
}
