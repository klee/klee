//===-- RaiseAsm.cpp ------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "Passes.h"

#include "llvm/InlineAsm.h"
#if !(LLVM_VERSION_MAJOR == 2 && LLVM_VERSION_MINOR < 7)
#include "llvm/LLVMContext.h"
#endif

using namespace llvm;
using namespace klee;

char RaiseAsmPass::ID = 0;

Function *RaiseAsmPass::getIntrinsic(llvm::Module &M,
                                     unsigned IID,
                                     const Type **Tys,
                                     unsigned NumTys) {  
  return Intrinsic::getDeclaration(&M, (llvm::Intrinsic::ID) IID, Tys, NumTys);
}

// FIXME: This should just be implemented as a patch to
// X86TargetAsmInfo.cpp, then everyone will benefit.
bool RaiseAsmPass::runOnInstruction(Module &M, Instruction *I) {
  if (CallInst *ci = dyn_cast<CallInst>(I)) {
    if (InlineAsm *ia = dyn_cast<InlineAsm>(ci->getCalledValue())) {
      const std::string &as = ia->getAsmString();
      const std::string &cs = ia->getConstraintString();
      const llvm::Type *T = ci->getType();

      // bswaps
      if (ci->getNumOperands() == 2 && 
          T == ci->getOperand(1)->getType() &&
          ((T == llvm::Type::getInt16Ty(getGlobalContext()) && 
            as == "rorw $$8, ${0:w}" &&
            cs == "=r,0,~{dirflag},~{fpsr},~{flags},~{cc}") ||
           (T == llvm::Type::getInt32Ty(getGlobalContext()) &&
            as == "rorw $$8, ${0:w};rorl $$16, $0;rorw $$8, ${0:w}" &&
            cs == "=r,0,~{dirflag},~{fpsr},~{flags},~{cc}"))) {
        llvm::Value *Arg0 = ci->getOperand(1);
        Function *F = getIntrinsic(M, Intrinsic::bswap, Arg0->getType());
        ci->setOperand(0, F);
        return true;
      }
    }
  }

  return false;
}

bool RaiseAsmPass::runOnModule(Module &M) {
  bool changed = false;
  
  for (Module::iterator fi = M.begin(), fe = M.end(); fi != fe; ++fi) {
    for (Function::iterator bi = fi->begin(), be = fi->end(); bi != be; ++bi) {
      for (BasicBlock::iterator ii = bi->begin(), ie = bi->end(); ii != ie;) {
        Instruction *i = ii;
        ++ii;  
        changed |= runOnInstruction(M, i);
      }
    }
  }

  return changed;
}
