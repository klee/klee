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
#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 3)
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Instructions.h"
#else
#include "llvm/InlineAsm.h"
#include "llvm/LLVMContext.h"
#include "llvm/Support/IRBuilder.h"
#endif

#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Host.h"
#include "llvm/Target/TargetLowering.h"
#if LLVM_VERSION_CODE < LLVM_VERSION(3, 0)
#include "llvm/Target/TargetRegistry.h"
#else
#include "llvm/Support/TargetRegistry.h"
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
       triple.getOS() == llvm::Triple::Darwin)) {

    if (ia->getAsmString() == "" && ia->hasSideEffects()) {
#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 3)
      IRBuilder<> Builder(I);
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
#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 1)
  std::string HostTriple = llvm::sys::getDefaultTargetTriple();
#else
  std::string HostTriple = llvm::sys::getHostTriple();
#endif
  const Target *NativeTarget = TargetRegistry::lookupTarget(HostTriple, Err);

  TargetMachine * TM = 0;
  if (NativeTarget == 0) {
    klee_warning("Warning: unable to select native target: %s", Err.c_str());
    TLI = 0;
  } else {
#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 1)
    TM = NativeTarget->createTargetMachine(HostTriple, "", "",
                                                          TargetOptions());
#elif LLVM_VERSION_CODE >= LLVM_VERSION(3, 0)
    TM = NativeTarget->createTargetMachine(HostTriple, "", "");
#else
    TM = NativeTarget->createTargetMachine(HostTriple, "");
#endif
    TLI = TM->getTargetLowering();

    triple = llvm::Triple(HostTriple);
  }
  
  for (Module::iterator fi = M.begin(), fe = M.end(); fi != fe; ++fi) {
    for (Function::iterator bi = fi->begin(), be = fi->end(); bi != be; ++bi) {
      for (BasicBlock::iterator ii = bi->begin(), ie = bi->end(); ii != ie;) {
        Instruction *i = static_cast<Instruction *>(ii);
        ++ii;  
        changed |= runOnInstruction(M, i);
      }
    }
  }

  if (TM)
    delete TM;

  return changed;
}
