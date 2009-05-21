//===-- IntrinsicCleaner.cpp ----------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "Passes.h"

#include "llvm/Constants.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Function.h"
#include "llvm/InstrTypes.h"
#include "llvm/Instruction.h"
#include "llvm/Instructions.h"
#include "llvm/IntrinsicInst.h"
#include "llvm/Module.h"
#include "llvm/Pass.h"
#include "llvm/Type.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Target/TargetData.h"

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
        // normal instructions.  FIXME: This is broken for non-x86_32.
      case Intrinsic::vacopy: { // (dst, src) -> *((i8**) dst) = *((i8**) src)
        Value *dst = ii->getOperand(1);
        Value *src = ii->getOperand(2);
        Type *i8pp = PointerType::getUnqual(PointerType::getUnqual(Type::Int8Ty));
        Value *castedDst = CastInst::CreatePointerCast(dst, i8pp, "vacopy.cast.dst", ii);
        Value *castedSrc = CastInst::CreatePointerCast(src, i8pp, "vacopy.cast.src", ii);
        Value *load = new LoadInst(castedSrc, "vacopy.read", ii);
        new StoreInst(load, castedDst, false, ii);
        ii->removeFromParent();
        delete ii;
        break;
      }

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
