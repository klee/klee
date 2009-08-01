//===-- Passes.h ------------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_PASSES_H
#define KLEE_PASSES_H

#include "llvm/Constants.h"
#include "llvm/Instructions.h"
#include "llvm/Module.h"
#include "llvm/Pass.h"
#include "llvm/CodeGen/IntrinsicLowering.h"

namespace llvm {
  class Function;
  class Instruction;
  class Module;
  class TargetData;
  class Type;
}

namespace klee {

  /// RaiseAsmPass - This pass raises some common occurences of inline
  /// asm which are used by glibc into normal LLVM IR.
class RaiseAsmPass : public llvm::ModulePass {
  static char ID;

  llvm::Function *getIntrinsic(llvm::Module &M,
                               unsigned IID,
                               const llvm::Type **Tys,
                               unsigned NumTys);
  llvm::Function *getIntrinsic(llvm::Module &M,
                               unsigned IID, 
                               const llvm::Type *Ty0) {
    return getIntrinsic(M, IID, &Ty0, 1);
  }

  bool runOnInstruction(llvm::Module &M, llvm::Instruction *I);

public:
  RaiseAsmPass() : llvm::ModulePass((intptr_t) &ID) {}
  
  virtual bool runOnModule(llvm::Module &M);
};

  // This is a module pass because it can add and delete module
  // variables (via intrinsic lowering).
class IntrinsicCleanerPass : public llvm::ModulePass {
  static char ID;
  const llvm::TargetData &TargetData;
  llvm::IntrinsicLowering *IL;
  bool LowerIntrinsics;

  bool runOnBasicBlock(llvm::BasicBlock &b);
public:
  IntrinsicCleanerPass(const llvm::TargetData &TD,
                       bool LI=true)
    : llvm::ModulePass((intptr_t) &ID),
      TargetData(TD),
      IL(new llvm::IntrinsicLowering(TD)),
      LowerIntrinsics(LI) {}
  ~IntrinsicCleanerPass() { delete IL; } 
  
  virtual bool runOnModule(llvm::Module &M);
};
  
  // performs two transformations which make interpretation
  // easier and faster.
  //
  // 1) Ensure that all the PHI nodes in a basic block have
  //    the incoming block list in the same order. Thus the
  //    incoming block index only needs to be computed once
  //    for each transfer.
  // 
  // 2) Ensure that no PHI node result is used as an argument to
  //    a subsequent PHI node in the same basic block. This allows
  //    the transfer to execute the instructions in order instead
  //    of in two passes.
class PhiCleanerPass : public llvm::FunctionPass {
  static char ID;

public:
  PhiCleanerPass() : llvm::FunctionPass((intptr_t) &ID) {}
  
  virtual bool runOnFunction(llvm::Function &f);
};
  
class DivCheckPass : public llvm::ModulePass {
  static char ID;
public:
  DivCheckPass(): ModulePass((intptr_t) &ID) {}
  virtual bool runOnModule(llvm::Module &M);
};

/// LowerSwitchPass - Replace all SwitchInst instructions with chained branch
/// instructions.  Note that this cannot be a BasicBlock pass because it
/// modifies the CFG!
class LowerSwitchPass : public llvm::FunctionPass {
public:
  static char ID; // Pass identification, replacement for typeid
  LowerSwitchPass() : FunctionPass((intptr_t) &ID) {} 
  
  virtual bool runOnFunction(llvm::Function &F);
  
  struct SwitchCase {
    llvm ::Constant *value;
    llvm::BasicBlock *block;
    
    SwitchCase() : value(0), block(0) { }
    SwitchCase(llvm::Constant *v, llvm::BasicBlock *b) :
      value(v), block(b) { }
  };
  
  typedef std::vector<SwitchCase>           CaseVector;
  typedef std::vector<SwitchCase>::iterator CaseItr;
  
private:
  void processSwitchInst(llvm::SwitchInst *SI);
  void switchConvert(CaseItr begin,
                     CaseItr end,
                     llvm::Value *value,
                     llvm::BasicBlock *origBlock,
                     llvm::BasicBlock *defaultBlock);
};

}

#endif
