// FIXME: This file is a bastard child of opt.cpp and llvm-ld's
// Optimize.cpp. This stuff should live in common code.

//===- Optimize.cpp - Optimize a complete program -------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements all optimization of the linked module for llvm-ld.
//
//===----------------------------------------------------------------------===//

#include "ModuleHelper.h"

#include "Passes.h"
#include "klee/Config/Version.h"
#include "klee/Support/OptionCategories.h"

#include "llvm/IR/DataLayout.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/IPO/GlobalDCE.h"
#include "llvm/Transforms/IPO/Internalize.h"
#include "llvm/Transforms/IPO/StripSymbols.h"
#include "llvm/Transforms/Scalar/Scalarizer.h"
#include "llvm/Transforms/Scalar/SimplifyCFG.h"
#include "llvm/Transforms/Utils/LowerSwitch.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"

using namespace llvm;
using namespace klee;

namespace {

static cl::opt<bool>
    DisableInline("disable-inlining",
                  cl::desc("Do not run the inliner pass (default=false)"),
                  cl::init(false), cl::cat(klee::ModuleCat));

static cl::opt<bool> DisableInternalize(
    "disable-internalize",
    cl::desc("Do not mark all symbols as internal (default=false)"),
    cl::init(false), cl::cat(klee::ModuleCat));

static cl::opt<bool> VerifyEach("verify-each",
                                cl::desc("Verify intermediate results of all "
                                         "optimization passes (default=false)"),
                                cl::init(false), cl::cat(klee::ModuleCat));

static cl::opt<bool> Strip(
    "strip-all",
    cl::desc("Strip all symbol information from executable (default=false)"),
    cl::init(false), cl::cat(klee::ModuleCat));

static cl::opt<bool> StripDebug(
    "strip-debug",
    cl::desc("Strip debugger symbol info from executable (default=false)"),
    cl::init(false), cl::cat(klee::ModuleCat));

void runModulePassManager(Module &M, ModulePassManager MPM) {
  LoopAnalysisManager LAM;
  FunctionAnalysisManager FAM;
  CGSCCAnalysisManager CGAM;
  ModuleAnalysisManager MAM;

  PassBuilder PB;
  PB.registerModuleAnalyses(MAM);
  PB.registerCGSCCAnalyses(CGAM);
  PB.registerFunctionAnalyses(FAM);
  PB.registerLoopAnalyses(LAM);
  PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);

  MPM.run(M, MAM);
}

void runFunctionPassManager(Module &M, FunctionPassManager FPM) {
  ModulePassManager MPM;
  MPM.addPass(createModuleToFunctionPassAdaptor(std::move(FPM)));
  runModulePassManager(M, std::move(MPM));
}

void verifyModuleIfRequested(Module &M) {
  if (VerifyEach && verifyModule(M, &errs()))
    report_fatal_error("Module verification failed");
}

void keepPreservedFunctionsAlive(Module &M,
                                 ArrayRef<const char *> preservedFunctions) {
  SmallVector<GlobalValue *, 16> used;
  for (const char *name : preservedFunctions) {
    Function *F = M.getFunction(name);
    if (F && !F->isDeclaration())
      used.push_back(F);
  }

  if (!used.empty())
    appendToCompilerUsed(M, used);
}

void runFinalKleeCleanup(Module &M, SwitchImplType SwitchType) {
  FunctionPassManager FPM;
  FPM.addPass(SimplifyCFGPass());
  if (SwitchType == SwitchImplType::eSwitchTypeLLVM)
    FPM.addPass(llvm::LowerSwitchPass());
  runFunctionPassManager(M, std::move(FPM));

  if (SwitchType == SwitchImplType::eSwitchTypeSimple) {
    klee::LowerSwitchPass P;
    for (auto &F : M) {
      if (!F.isDeclaration())
        P.runOnFunction(F);
    }
  }

  DataLayout targetData(&M);
  IntrinsicCleanerPass(targetData).runOnModule(M);

  FunctionPassManager ScalarizerFPM;
  ScalarizerFPM.addPass(ScalarizerPass());
  runFunctionPassManager(M, std::move(ScalarizerFPM));

  klee::PhiCleanerPass PhiCleaner;
  for (auto &F : M) {
    if (!F.isDeclaration())
      PhiCleaner.runOnFunction(F);
  }

  FunctionAliasPass().runOnModule(M);
}

} // namespace

void klee::optimizeModule(llvm::Module *M,
                          llvm::ArrayRef<const char *> preservedFunctions) {
  keepPreservedFunctionsAlive(*M, preservedFunctions);

  ModulePassManager MPM;

  if (StripDebug) {
    MPM.addPass(StripDebugDeclarePass());
    MPM.addPass(StripDeadDebugInfoPass());
  }

  PassBuilder PB;
  if (DisableInline) {
    MPM.addPass(createModuleToFunctionPassAdaptor(
        PB.buildFunctionSimplificationPipeline(OptimizationLevel::O2,
                                               ThinOrFullLTOPhase::None)));
#if LLVM_VERSION_CODE >= LLVM_VERSION(17, 0)
    MPM.addPass(PB.buildModuleOptimizationPipeline(OptimizationLevel::O2,
                                                   ThinOrFullLTOPhase::None));
#else
    MPM.addPass(PB.buildModuleOptimizationPipeline(OptimizationLevel::O2));
#endif
  } else {
    MPM.addPass(PB.buildPerModuleDefaultPipeline(OptimizationLevel::O2));
  }

  if (!DisableInternalize) {
    auto PreserveFunctions = [=](const GlobalValue &GV) {
      StringRef GVName = GV.getName();

      for (const char *fun : preservedFunctions)
        if (GVName.equals(fun))
          return true;

      return false;
    };
    MPM.addPass(InternalizePass(PreserveFunctions));
    MPM.addPass(GlobalDCEPass());
  }

  if (Strip)
    MPM.addPass(StripSymbolsPass());
  else if (StripDebug) {
    MPM.addPass(StripDebugDeclarePass());
    MPM.addPass(StripDeadDebugInfoPass());
  }

  MPM.addPass(GlobalDCEPass());

  if (VerifyEach)
    MPM.addPass(VerifierPass());

  runModulePassManager(*M, std::move(MPM));
  verifyModuleIfRequested(*M);
}

void klee::optimiseAndPrepare(bool OptimiseKLEECall, bool Optimize,
                              SwitchImplType SwitchType, std::string EntryPoint,
                              llvm::ArrayRef<const char *> preservedFunctions,
                              llvm::Module *module) {
  // Preserve all functions containing klee-related function calls from being
  // optimised around.
  if (!OptimiseKLEECall)
    OptNonePass().runOnModule(*module);

  if (Optimize)
    optimizeModule(module, preservedFunctions);

  // Needs to happen after linking and optimization, since both can rewrite
  // global constructor/destructor lists.
  injectStaticConstructorsAndDestructors(module, EntryPoint);

  runFinalKleeCleanup(*module, SwitchType);
}
