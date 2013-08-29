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

#include "llvm/Constants.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Function.h"
#include "llvm/InstrTypes.h"
#include "llvm/Instruction.h"
#include "llvm/Instructions.h"
#include "llvm/IntrinsicInst.h"
#if LLVM_VERSION_CODE >= LLVM_VERSION(2, 7)
#include "llvm/LLVMContext.h"
#endif
#include "llvm/Module.h"
#include "llvm/Pass.h"
#include "llvm/Type.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#if LLVM_VERSION_CODE <= LLVM_VERSION(3, 1)
#include "llvm/Target/TargetData.h"
#else
#include "llvm/DataLayout.h"
#endif

using namespace llvm;
using namespace klee;

char DivCheckPass::ID;

bool DivCheckPass::runOnModule(Module &M) { 
  Function *divZeroCheckFunction = 0;

  bool moduleChanged = false;
  
  for (Module::iterator f = M.begin(), fe = M.end(); f != fe; ++f) {
    for (Function::iterator b = f->begin(), be = f->end(); b != be; ++b) {
      for (BasicBlock::iterator i = b->begin(), ie = b->end(); i != ie; ++i) {     
          if (BinaryOperator* binOp = dyn_cast<BinaryOperator>(i)) {
          // find all [s|u][div|mod] instructions
          Instruction::BinaryOps opcode = binOp->getOpcode();
          if (opcode == Instruction::SDiv || opcode == Instruction::UDiv ||
              opcode == Instruction::SRem || opcode == Instruction::URem) {
            
            CastInst *denominator =
              CastInst::CreateIntegerCast(i->getOperand(1),
                                          Type::getInt64Ty(getGlobalContext()),
                                          false,  /* sign doesn't matter */
                                          "int_cast_to_i64",
                                          i);
            
            // Lazily bind the function to avoid always importing it.
            if (!divZeroCheckFunction) {
              Constant *fc = M.getOrInsertFunction("klee_div_zero_check", 
                                                   Type::getVoidTy(getGlobalContext()), 
                                                   Type::getInt64Ty(getGlobalContext()), 
                                                   NULL);
              divZeroCheckFunction = cast<Function>(fc);
            }

	    CallInst::Create(divZeroCheckFunction, denominator, "", &*i);
            moduleChanged = true;
          }
        }
      }
    }
  }
  return moduleChanged;
}

char OvershiftCheckPass::ID;

bool OvershiftCheckPass::runOnModule(Module &M) {
  Function *overshiftCheckFunction = 0;

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

            ConstantInt *bitWidthC = ConstantInt::get(Type::getInt64Ty(getGlobalContext()),bitWidth,false);
            args.push_back(bitWidthC);

            CastInst *shift =
              CastInst::CreateIntegerCast(i->getOperand(1),
                                          Type::getInt64Ty(getGlobalContext()),
                                          false,  /* sign doesn't matter */
                                          "int_cast_to_i64",
                                          i);
            args.push_back(shift);


            // Lazily bind the function to avoid always importing it.
            if (!overshiftCheckFunction) {
              Constant *fc = M.getOrInsertFunction("klee_overshift_check",
                                                   Type::getVoidTy(getGlobalContext()),
                                                   Type::getInt64Ty(getGlobalContext()),
                                                   Type::getInt64Ty(getGlobalContext()),
                                                   NULL);
              overshiftCheckFunction = cast<Function>(fc);
            }

            // Inject CallInstr to check if overshifting possible
#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 0)
            CallInst::Create(overshiftCheckFunction, args, "", &*i);
#else
            CallInst::Create(overshiftCheckFunction, args.begin(), args.end(), "", &*i);
#endif
            moduleChanged = true;
          }
        }
      }
    }
  }
  return moduleChanged;
}
