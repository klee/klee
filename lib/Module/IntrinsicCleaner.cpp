//===-- IntrinsicCleaner.cpp ----------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "Passes.h"

#include "klee/Config/Version.h"
#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 3)
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/IRBuilder.h"

#else
#include "llvm/Constants.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Function.h"
#include "llvm/InstrTypes.h"
#include "llvm/Instruction.h"
#include "llvm/Instructions.h"
#include "llvm/IntrinsicInst.h"
#include "llvm/LLVMContext.h"
#include "llvm/Module.h"
#include "llvm/Type.h"
#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 2)
#include "llvm/IRBuilder.h"
#else
#include "llvm/Support/IRBuilder.h"
#endif
#if LLVM_VERSION_CODE <= LLVM_VERSION(3, 1)
#include "llvm/Target/TargetData.h"
#else
#include "llvm/DataLayout.h"
#endif
#endif
#include "llvm/Pass.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

using namespace llvm;

namespace klee {

char IntrinsicCleanerPass::ID;

bool IntrinsicCleanerPass::runOnModule(Module &M) {
  bool dirty = false;
  for (Module::iterator f = M.begin(), fe = M.end(); f != fe; ++f)
    for (Function::iterator b = f->begin(), be = f->end(); b != be; ++b)
      dirty |= runOnBasicBlock(*b, M);
    if (Function *Declare = M.getFunction("llvm.trap"))
      Declare->eraseFromParent();
  return dirty;
}

bool IntrinsicCleanerPass::runOnBasicBlock(BasicBlock &b, Module &M) {
  bool dirty = false;
  bool block_split=false;
  
#if LLVM_VERSION_CODE <= LLVM_VERSION(3, 1)
  unsigned WordSize = TargetData.getPointerSizeInBits() / 8;
#else
  unsigned WordSize = DataLayout.getPointerSizeInBits() / 8;
#endif
  for (BasicBlock::iterator i = b.begin(), ie = b.end();
       (i != ie) && (block_split == false);) {
    IntrinsicInst *ii = dyn_cast<IntrinsicInst>(&*i);
    // increment now since LowerIntrinsic deletion makes iterator invalid.
    ++i;  
    if(ii) {
      switch (ii->getIntrinsicID()) {
      case Intrinsic::vastart:
      case Intrinsic::vaend:
        break;
        
        // Lower vacopy so that object resolution etc is handled by
        // normal instructions.
        //
        // FIXME: This is much more target dependent than just the word size,
        // however this works for x86-32 and x86-64.
      case Intrinsic::vacopy: { // (dst, src) -> *((i8**) dst) = *((i8**) src)
        Value *dst = ii->getArgOperand(0);
        Value *src = ii->getArgOperand(1);

        if (WordSize == 4) {
          Type *i8pp = PointerType::getUnqual(PointerType::getUnqual(Type::getInt8Ty(getGlobalContext())));
          Value *castedDst = CastInst::CreatePointerCast(dst, i8pp, "vacopy.cast.dst", ii);
          Value *castedSrc = CastInst::CreatePointerCast(src, i8pp, "vacopy.cast.src", ii);
          Value *load = new LoadInst(castedSrc, "vacopy.read", ii);
          new StoreInst(load, castedDst, false, ii);
        } else {
          assert(WordSize == 8 && "Invalid word size!");
          Type *i64p = PointerType::getUnqual(Type::getInt64Ty(getGlobalContext()));
          Value *pDst = CastInst::CreatePointerCast(dst, i64p, "vacopy.cast.dst", ii);
          Value *pSrc = CastInst::CreatePointerCast(src, i64p, "vacopy.cast.src", ii);
          Value *val = new LoadInst(pSrc, std::string(), ii); new StoreInst(val, pDst, ii);
          Value *off = ConstantInt::get(Type::getInt64Ty(getGlobalContext()), 1);
          pDst = GetElementPtrInst::Create(pDst, off, std::string(), ii);
          pSrc = GetElementPtrInst::Create(pSrc, off, std::string(), ii);
          val = new LoadInst(pSrc, std::string(), ii); new StoreInst(val, pDst, ii);
          pDst = GetElementPtrInst::Create(pDst, off, std::string(), ii);
          pSrc = GetElementPtrInst::Create(pSrc, off, std::string(), ii);
          val = new LoadInst(pSrc, std::string(), ii); new StoreInst(val, pDst, ii);
        }
        ii->removeFromParent();
        delete ii;
        break;
      }

      case Intrinsic::uadd_with_overflow:
      case Intrinsic::usub_with_overflow:
      case Intrinsic::umul_with_overflow: {
        IRBuilder<> builder(ii->getParent(), ii);
        Function *F = builder.GetInsertBlock()->getParent();

        Value *op1 = ii->getArgOperand(0);
        Value *op2 = ii->getArgOperand(1);
        
        Value *result = 0;
        Value *overflow = 0;
        if (ii->getIntrinsicID() == Intrinsic::uadd_with_overflow){
          result = builder.CreateAdd(op1, op2);
          overflow = builder.CreateICmpULT(result, op1);
        } else if (ii->getIntrinsicID() == Intrinsic::usub_with_overflow){
          result = builder.CreateSub(op1, op2);
          overflow = builder.CreateICmpUGT(result, op1);
        } else if (ii->getIntrinsicID() == Intrinsic::umul_with_overflow){
          BasicBlock *entry = builder.GetInsertBlock();
          entry->setName(Twine(entry->getName(), "_umul_start"));
          BasicBlock *cont_of = entry->splitBasicBlock(builder.GetInsertPoint(),
                                                       "umul_end");
          BasicBlock *on_of = BasicBlock::Create(builder.getContext(),
                                                 "umul_overflow", F, cont_of);

          // remove the branch inserted by splitBasicBlock, we'll add our own
          entry->getTerminator()->eraseFromParent();
          builder.SetInsertPoint(entry);
          Value *no_overflow = builder.getFalse();
          Value *one = ConstantInt::getSigned(op1->getType(), 1);
          Value *op1_g1 = builder.CreateICmpUGT(op1, one);
          Value *op2_g1 = builder.CreateICmpUGT(op2, one);
          Value *may_of = builder.CreateAnd(op1_g1, op2_g1);
          builder.CreateCondBr(may_of, on_of, cont_of);

          builder.SetInsertPoint(on_of);
          uint64_t bit_size = op1->getType()->getPrimitiveSizeInBits();
          Value *uint_max = ConstantInt::get(op1->getType(),
                                             APInt::getMaxValue(bit_size));
          Value *div1 = builder.CreateUDiv(uint_max, op1);
          Value *overflow1 = builder.CreateICmpUGT(op2, div1);
          builder.CreateBr(cont_of);

          builder.SetInsertPoint(cont_of, cont_of->begin());
          PHINode *phi_of = builder.CreatePHI(no_overflow->getType(), 2);
          phi_of->addIncoming(overflow1, on_of);
          phi_of->addIncoming(no_overflow, entry);

          result = builder.CreateMul(op1, op2);
          overflow = phi_of;
          block_split = true;
        }

        Value *resultStruct =
          builder.CreateInsertValue(UndefValue::get(ii->getType()), result, 0);
        resultStruct = builder.CreateInsertValue(resultStruct, overflow, 1);
        
        ii->replaceAllUsesWith(resultStruct);
        ii->removeFromParent();
        delete ii;
        dirty = true;
        break;
      }

      case Intrinsic::dbg_value:
      case Intrinsic::dbg_declare:
        // Remove these regardless of lower intrinsics flag. This can
        // be removed once IntrinsicLowering is fixed to not have bad
        // caches.
        ii->eraseFromParent();
        dirty = true;
        break;

      case Intrinsic::trap: {
        // Intrisic instruction "llvm.trap" found. Directly lower it to
        // a call of the abort() function.
        Function *F = cast<Function>(
          M.getOrInsertFunction(
            "abort", Type::getVoidTy(getGlobalContext()), NULL));
        F->setDoesNotReturn();
        F->setDoesNotThrow();

        CallInst::Create(F, Twine(), ii);
        new UnreachableInst(getGlobalContext(), ii);

        ii->eraseFromParent();

        dirty = true;
        break;
      }
                    
      default:
        if (LowerIntrinsics)
          IL->LowerIntrinsicCall(ii);
        dirty = true;
        break;
      }
    }
  }

  return dirty;
}
}
