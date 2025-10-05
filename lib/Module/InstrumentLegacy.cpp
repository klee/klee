//===-- InstrumentLegacy.cpp ------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#include "ModuleHelper.h"

#include "Passes.h"
#include "klee/Support/CompilerWarning.h"
#include "klee/Support/ErrorHandling.h"

DISABLE_WARNING_PUSH
DISABLE_WARNING_DEPRECATED_DECLARATIONS
#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/ValueSymbolTable.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/Scalarizer.h"
#include "llvm/Transforms/Utils.h"
#include "llvm/Transforms/Utils/Cloning.h"
DISABLE_WARNING_POP

using namespace llvm;
using namespace klee;

void klee::instrument(bool CheckDivZero, bool CheckOvershift,
                      llvm::Module *module) {
  // Inject checks prior to optimization... we also perform the
  // invariant transformations that we will end up doing later so that
  // optimize is seeing what is as close as possible to the final
  // module.
  legacy::PassManager pm;
  pm.add(new RaiseAsmPass());

  // This pass will scalarize as much code as possible so that the Executor
  // does not need to handle operands of vector type for most instructions
  // other than InsertElementInst and ExtractElementInst.
  //
  // NOTE: Must come before division/overshift checks because those passes
  // don't know how to handle vector instructions.
  pm.add(createScalarizerPass());

  // This pass will replace atomic instructions with non-atomic operations
  pm.add(createLowerAtomicPass());
  if (CheckDivZero)
    pm.add(new DivCheckPass());
  if (CheckOvershift)
    pm.add(new OvershiftCheckPass());

  llvm::DataLayout targetData(module);
  pm.add(new IntrinsicCleanerPass(targetData));
  pm.run(*module);
}

void klee::checkModule(bool DontVerify, llvm::Module *module) {
  InstructionOperandTypeCheckPass *operandTypeCheckPass =
      new InstructionOperandTypeCheckPass();

  legacy::PassManager pm;
  if (!DontVerify)
    pm.add(createVerifierPass());
  pm.add(operandTypeCheckPass);
  pm.run(*module);

  // Enforce the operand type invariants that the Executor expects.  This
  // implicitly depends on the "Scalarizer" pass to be run in order to succeed
  // in the presence of vector instructions.
  if (!operandTypeCheckPass->checkPassed()) {
    klee_error("Unexpected instruction operand types detected");
  }
}

void klee::optimiseAndPrepare(bool OptimiseKLEECall, bool Optimize,
                              SwitchImplType SwitchType, std::string EntryPoint,
                              llvm::ArrayRef<const char *> preservedFunctions,
                              llvm::Module *module) {
  // Preserve all functions containing klee-related function calls from being
  // optimised around
  if (!OptimiseKLEECall) {
    legacy::PassManager pm;
    pm.add(new OptNonePass());
    pm.run(*module);
  }

  if (Optimize)
    optimizeModule(module, preservedFunctions);

  // Needs to happen after linking (since ctors/dtors can be modified)
  // and optimization (since global optimization can rewrite lists).
  injectStaticConstructorsAndDestructors(module, EntryPoint);

  // Finally, run the passes that maintain invariants we expect during
  // interpretation. We run the intrinsic cleaner just in case we
  // linked in something with intrinsics but any external calls are
  // going to be unresolved. We really need to handle the intrinsics
  // directly I think?
  legacy::PassManager pm3;
  pm3.add(createCFGSimplificationPass());
  switch (SwitchType) {
  case SwitchImplType::eSwitchTypeInternal:
    break;
  case SwitchImplType::eSwitchTypeSimple:
    pm3.add(new LowerSwitchPass());
    break;
  case SwitchImplType::eSwitchTypeLLVM:
    pm3.add(createLowerSwitchPass());
    break;
  }

  llvm::DataLayout targetData(module);
  pm3.add(new IntrinsicCleanerPass(targetData));
  pm3.add(createScalarizerPass());
  pm3.add(new PhiCleanerPass());
  pm3.add(new FunctionAliasPass());
  pm3.run(*module);
}
