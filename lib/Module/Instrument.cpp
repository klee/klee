//===-- Instrument.cpp ------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "ModuleHelper.h"

#include "Passes.h"
#include "klee/Config/Version.h"
#include "klee/Support/ErrorHandling.h"

#include "llvm/IR/DataLayout.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Support/raw_ostream.h"
#if LLVM_VERSION_CODE >= LLVM_VERSION(17, 0)
#include "llvm/Transforms/Scalar/LowerAtomicPass.h"
#else
#include "llvm/Transforms/Scalar/LowerAtomic.h"
#endif
#include "llvm/Transforms/Scalar/Scalarizer.h"

using namespace klee;

namespace {

void runModulePassManager(llvm::Module &M, llvm::ModulePassManager MPM) {
  llvm::LoopAnalysisManager LAM;
  llvm::FunctionAnalysisManager FAM;
  llvm::CGSCCAnalysisManager CGAM;
  llvm::ModuleAnalysisManager MAM;

  llvm::PassBuilder PB;
  PB.registerModuleAnalyses(MAM);
  PB.registerCGSCCAnalyses(CGAM);
  PB.registerFunctionAnalyses(FAM);
  PB.registerLoopAnalyses(LAM);
  PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);

  MPM.run(M, MAM);
}

void runFunctionPassManager(llvm::Module &M, llvm::FunctionPassManager FPM) {
  llvm::ModulePassManager MPM;
  MPM.addPass(llvm::createModuleToFunctionPassAdaptor(std::move(FPM)));
  runModulePassManager(M, std::move(MPM));
}

} // namespace

void klee::checkModule(bool DontVerify, llvm::Module *module) {
  if (!DontVerify && llvm::verifyModule(*module, &llvm::errs()))
    klee_error("Module verification failed");

  InstructionOperandTypeCheckPass operandTypeCheckPass;
  operandTypeCheckPass.runOnModule(*module);

  // Enforce the operand type invariants that the Executor expects. This
  // implicitly depends on the Scalarizer pass to be run in order to succeed
  // in the presence of vector instructions.
  if (!operandTypeCheckPass.checkPassed())
    klee_error("Unexpected instruction operand types detected");
}

void klee::instrument(bool CheckDivZero, bool CheckOvershift,
                      llvm::Module *module) {
  RaiseAsmPass().runOnModule(*module);

  // Scalarize before adding checks: KLEE's check passes expect scalar operands.
  llvm::FunctionPassManager FPM;
  FPM.addPass(llvm::ScalarizerPass());
  FPM.addPass(llvm::LowerAtomicPass());
  runFunctionPassManager(*module, std::move(FPM));

  if (CheckDivZero)
    DivCheckPass().runOnModule(*module);
  if (CheckOvershift)
    OvershiftCheckPass().runOnModule(*module);

  llvm::DataLayout targetData(module);
  IntrinsicCleanerPass(targetData).runOnModule(*module);
}
