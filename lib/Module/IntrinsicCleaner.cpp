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
#include "llvm/Support/IRBuilder.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#if LLVM_VERSION_CODE <= LLVM_VERSION(3, 1)
#include "llvm/Target/TargetData.h"
#else
#include "llvm/DataLayout.h"
#endif

using namespace llvm;

namespace klee {

char IntrinsicCleanerPass::ID;

bool IntrinsicCleanerPass::runOnModule(Module &M) {
  bool dirty = false;
  for (Module::iterator f = M.begin(), fe = M.end(); f != fe; ++f)
    for (Function::iterator b = f->begin(), be = f->end(); b != be; ++b)
      dirty |= runOnBasicBlock(*b);
  return dirty;
}

bool IntrinsicCleanerPass::runOnBasicBlock(BasicBlock &b) { 
  bool dirty = false;
  
#if LLVM_VERSION_CODE <= LLVM_VERSION(3, 1)
  unsigned WordSize = TargetData.getPointerSizeInBits() / 8;
#else
  unsigned WordSize = DataLayout.getPointerSizeInBits() / 8;
#endif
  for (BasicBlock::iterator i = b.begin(), ie = b.end(); i != ie;) {     
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
#if LLVM_VERSION_CODE < LLVM_VERSION(2, 8)
        Value *dst = ii->getOperand(1);
        Value *src = ii->getOperand(2);
#else
        Value *dst = ii->getArgOperand(0);
        Value *src = ii->getArgOperand(1);
#endif

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

      case Intrinsic::uadd_with_overflow: {
        IRBuilder<> builder(ii->getParent(), ii);

#if LLVM_VERSION_CODE < LLVM_VERSION(2, 8)
        Value *op1 = ii->getOperand(1);
        Value *op2 = ii->getOperand(2);
#else
        Value *op1 = ii->getArgOperand(0);
        Value *op2 = ii->getArgOperand(1);
#endif
        
        Value *result = builder.CreateAdd(op1, op2);
        Value *overflow = builder.CreateICmpULT(result, op1);
        
        Value *resultStruct =
          builder.CreateInsertValue(UndefValue::get(ii->getType()), result, 0);
        resultStruct = builder.CreateInsertValue(resultStruct, overflow, 1);
        
        ii->replaceAllUsesWith(resultStruct);
        ii->removeFromParent();
        delete ii;
        dirty = true;
        break;
      }

#if LLVM_VERSION_CODE < LLVM_VERSION(2, 7)
      case Intrinsic::dbg_stoppoint: {
        // We can remove this stoppoint if the next instruction is
        // sure to be another stoppoint. This is nice for cleanliness
        // but also important for switch statements where it can allow
        // the targets to be joined.
        bool erase = false;
        if (isa<DbgStopPointInst>(i) ||
            isa<UnreachableInst>(i)) {
          erase = true;
        } else if (isa<BranchInst>(i) ||
                   isa<SwitchInst>(i)) {
          BasicBlock *bb = i->getParent();
          erase = true;
          for (succ_iterator it=succ_begin(bb), ie=succ_end(bb);
               it!=ie; ++it) {
            if (!isa<DbgStopPointInst>(it->getFirstNonPHI())) {
              erase = false;
              break;
            }
          }
        }

        if (erase) {
          ii->eraseFromParent();
          dirty = true;
        }
        break;
      }

      case Intrinsic::dbg_region_start:
      case Intrinsic::dbg_region_end:
      case Intrinsic::dbg_func_start:
#else
      case Intrinsic::dbg_value:
#endif
      case Intrinsic::dbg_declare:
        // Remove these regardless of lower intrinsics flag. This can
        // be removed once IntrinsicLowering is fixed to not have bad
        // caches.
        ii->eraseFromParent();
        dirty = true;
        break;
                    
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
