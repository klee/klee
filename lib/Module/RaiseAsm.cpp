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
#include "klee/Internal/Support/ErrorHandling.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Instructions.h"

#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/TargetRegistry.h"
#if LLVM_VERSION_CODE >= LLVM_VERSION(6, 0)
#include "llvm/CodeGen/TargetLowering.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"
#include "llvm/Target/TargetMachine.h"
#else
#include "llvm/Target/TargetLowering.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetSubtargetInfo.h"
#endif

using namespace llvm;
using namespace klee;

char RaiseAsmPass::ID = 0;

Function *RaiseAsmPass::getIntrinsic(llvm::Module &M, unsigned IID, Type **Tys,
                                     unsigned NumTys) {
  return Intrinsic::getDeclaration(&M, (llvm::Intrinsic::ID) IID,
                                   llvm::ArrayRef<llvm::Type*>(Tys, NumTys));
}

// FIXME: This should just be implemented as a patch to
// X86TargetAsmInfo.cpp, then everyone will benefit.
bool RaiseAsmPass::runOnInstruction(Module &M, Instruction *I) {
  // We can just raise inline assembler using calls
  CallInst *ci = dyn_cast<CallInst>(I);
  if (!ci)
    return false;

  InlineAsm *ia = dyn_cast<InlineAsm>(ci->getCalledValue());
  if (!ia)
    return false;

  // Try to use existing infrastructure
  if (!TLI)
    return false;

  if (TLI->ExpandInlineAsm(ci))
    return true;

  if (triple.getArch() == llvm::Triple::x86_64 &&
      (triple.getOS() == llvm::Triple::Linux ||
       triple.getOS() == llvm::Triple::Darwin ||
       triple.getOS() == llvm::Triple::FreeBSD)) {

    if (ia->getAsmString() == "" && ia->hasSideEffects()) {
      IRBuilder<> Builder(I);
#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 9)
      Builder.CreateFence(llvm::AtomicOrdering::SequentiallyConsistent);
#else
      Builder.CreateFence(llvm::SequentiallyConsistent);
#endif
      I->eraseFromParent();
      return true;
    }
  }

  return false;
}

bool RaiseAsmPass::runOnModule(Module &M) {
  bool changed = false;

  std::string Err;
  std::string HostTriple = llvm::sys::getDefaultTargetTriple();
  const Target *NativeTarget = TargetRegistry::lookupTarget(HostTriple, Err);

  TargetMachine * TM = 0;
  if (NativeTarget == 0) {
    klee_warning("Warning: unable to select native target: %s", Err.c_str());
    TLI = 0;
  } else {
#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 9)
    TM = NativeTarget->createTargetMachine(HostTriple, "", "", TargetOptions(),
        None);
    TLI = TM->getSubtargetImpl(*(M.begin()))->getTargetLowering();
#else
    TM = NativeTarget->createTargetMachine(HostTriple, "", "", TargetOptions());
    TLI = TM->getSubtargetImpl(*(M.begin()))->getTargetLowering();
#endif

    triple = llvm::Triple(HostTriple);
  }
  
  for (Module::iterator fi = M.begin(), fe = M.end(); fi != fe; ++fi) {
    for (Function::iterator bi = fi->begin(), be = fi->end(); bi != be; ++bi) {
      for (BasicBlock::iterator ii = bi->begin(), ie = bi->end(); ii != ie;) {
        Instruction *i = &*ii;
        ++ii;  
        changed |= runOnInstruction(M, i);
      }
    }
  }

  delete TM;

  return changed;
}
