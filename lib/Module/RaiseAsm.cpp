//===-- RaiseAsm.cpp ------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "Passes.h"
#include "klee/Config/Version.h"

#include "llvm/InlineAsm.h"
#if LLVM_VERSION_CODE >= LLVM_VERSION(2, 7)
#include "llvm/LLVMContext.h"
#endif
#if LLVM_VERSION_CODE >= LLVM_VERSION(2, 9)
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Host.h"
#include "llvm/Target/TargetLowering.h"
#if LLVM_VERSION_CODE < LLVM_VERSION(3, 0)
#include "llvm/Target/TargetRegistry.h"
#else
#include "llvm/Support/TargetRegistry.h"
#endif
#endif

using namespace llvm;
using namespace klee;

char RaiseAsmPass::ID = 0;

Function *RaiseAsmPass::getIntrinsic(llvm::Module &M,
                                     unsigned IID,
                                     LLVM_TYPE_Q Type **Tys,
                                     unsigned NumTys) {  
#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 0)
  return Intrinsic::getDeclaration(&M, (llvm::Intrinsic::ID) IID,
                                   llvm::ArrayRef<llvm::Type*>(Tys, NumTys));
#else
  return Intrinsic::getDeclaration(&M, (llvm::Intrinsic::ID) IID, Tys, NumTys);
#endif
}

// FIXME: This should just be implemented as a patch to
// X86TargetAsmInfo.cpp, then everyone will benefit.
bool RaiseAsmPass::runOnInstruction(Module &M, Instruction *I) {
  if (CallInst *ci = dyn_cast<CallInst>(I)) {
    if (InlineAsm *ia = dyn_cast<InlineAsm>(ci->getCalledValue())) {
#if LLVM_VERSION_CODE >= LLVM_VERSION(2, 9)
      (void) ia;
      return TLI && TLI->ExpandInlineAsm(ci);
#else
      const std::string &as = ia->getAsmString();
      const std::string &cs = ia->getConstraintString();
      const llvm::Type *T = ci->getType();

      // bswaps
#if LLVM_VERSION_CODE < LLVM_VERSION(2, 8)
      unsigned NumOperands = ci->getNumOperands();
      llvm::Value *Arg0 = NumOperands > 1 ? ci->getOperand(1) : 0;
#else
      unsigned NumOperands = ci->getNumArgOperands() + 1;
      llvm::Value *Arg0 = NumOperands > 1 ? ci->getArgOperand(0) : 0;
#endif
      if (Arg0 && T == Arg0->getType() &&
          ((T == llvm::Type::getInt16Ty(getGlobalContext()) && 
            as == "rorw $$8, ${0:w}" &&
            cs == "=r,0,~{dirflag},~{fpsr},~{flags},~{cc}") ||
           (T == llvm::Type::getInt32Ty(getGlobalContext()) &&
            as == "rorw $$8, ${0:w};rorl $$16, $0;rorw $$8, ${0:w}" &&
            cs == "=r,0,~{dirflag},~{fpsr},~{flags},~{cc}"))) {
        Function *F = getIntrinsic(M, Intrinsic::bswap, Arg0->getType());
#if LLVM_VERSION_CODE < LLVM_VERSION(2, 8)
        ci->setOperand(0, F);
#else
        ci->setCalledFunction(F);
#endif
        return true;
      }
#endif
    }
  }

  return false;
}

bool RaiseAsmPass::runOnModule(Module &M) {
  bool changed = false;

#if LLVM_VERSION_CODE >= LLVM_VERSION(2, 9)
  std::string Err;
#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 1)
  std::string HostTriple = llvm::sys::getDefaultTargetTriple();
#else
  std::string HostTriple = llvm::sys::getHostTriple();
#endif
  const Target *NativeTarget = TargetRegistry::lookupTarget(HostTriple, Err);
  if (NativeTarget == 0) {
    llvm::errs() << "Warning: unable to select native target: " << Err << "\n";
    TLI = 0;
  } else {
#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 1)
    TargetMachine *TM = NativeTarget->createTargetMachine(HostTriple, "", "",
                                                          TargetOptions());
#elif LLVM_VERSION_CODE >= LLVM_VERSION(3, 0)
    TargetMachine *TM = NativeTarget->createTargetMachine(HostTriple, "", "");
#else
    TargetMachine *TM = NativeTarget->createTargetMachine(HostTriple, "");
#endif
    TLI = TM->getTargetLowering();
  }
#endif
  
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
