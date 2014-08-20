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

#include "klee/Config/Version.h"
#include "llvm/PassManager.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/DynamicLibrary.h"

#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 3)
#include "llvm/IR/Module.h"
#include "llvm/IR/DataLayout.h"
#else
#include "llvm/Module.h"
#if LLVM_VERSION_CODE <= LLVM_VERSION(3, 1)
#include "llvm/Target/TargetData.h"
#else
#include "llvm/DataLayout.h"
#endif
#endif

#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 5)
#include "llvm/IR/Verifier.h"
#else
#include "llvm/Analysis/Verifier.h"
#endif

#include "llvm/Target/TargetMachine.h"
#include "llvm/Transforms/IPO.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Support/PluginLoader.h"
using namespace llvm;

// Don't verify at the end
static cl::opt<bool> DontVerify("disable-verify", cl::ReallyHidden);

static cl::opt<bool> DisableInline("disable-inlining",
  cl::desc("Do not run the inliner pass"));

static cl::opt<bool>
DisableOptimizations("disable-opt",
  cl::desc("Do not run any optimization passes"));

static cl::opt<bool> DisableInternalize("disable-internalize",
  cl::desc("Do not mark all symbols as internal"));

static cl::opt<bool> VerifyEach("verify-each",
 cl::desc("Verify intermediate results of all passes"));

static cl::alias ExportDynamic("export-dynamic",
  cl::aliasopt(DisableInternalize),
  cl::desc("Alias for -disable-internalize"));

static cl::opt<bool> Strip("strip-all", 
  cl::desc("Strip all symbol info from executable"));

static cl::alias A0("s", cl::desc("Alias for --strip-all"), 
  cl::aliasopt(Strip));

static cl::opt<bool> StripDebug("strip-debug",
  cl::desc("Strip debugger symbol info from executable"));

static cl::alias A1("S", cl::desc("Alias for --strip-debug"),
  cl::aliasopt(StripDebug));

// A utility function that adds a pass to the pass manager but will also add
// a verifier pass after if we're supposed to verify.
static inline void addPass(PassManager &PM, Pass *P) {
  // Add the pass to the pass manager...
  PM.add(P);

  // If we are verifying all of the intermediate steps, add the verifier...
  if (VerifyEach)
    PM.add(createVerifierPass());
}

namespace llvm {


static void AddStandardCompilePasses(PassManager &PM) {
  PM.add(createVerifierPass());                  // Verify that input is correct

#if LLVM_VERSION_CODE < LLVM_VERSION(3, 0)
  addPass(PM, createLowerSetJmpPass());          // Lower llvm.setjmp/.longjmp
#endif

  // If the -strip-debug command line option was specified, do it.
  if (StripDebug)
    addPass(PM, createStripSymbolsPass(true));

  if (DisableOptimizations) return;

  addPass(PM, createCFGSimplificationPass());    // Clean up disgusting code
  addPass(PM, createPromoteMemoryToRegisterPass());// Kill useless allocas
  addPass(PM, createGlobalOptimizerPass());      // Optimize out global vars
  addPass(PM, createGlobalDCEPass());            // Remove unused fns and globs
  addPass(PM, createIPConstantPropagationPass());// IP Constant Propagation
  addPass(PM, createDeadArgEliminationPass());   // Dead argument elimination
  addPass(PM, createInstructionCombiningPass()); // Clean up after IPCP & DAE
  addPass(PM, createCFGSimplificationPass());    // Clean up after IPCP & DAE

  addPass(PM, createPruneEHPass());              // Remove dead EH info
  addPass(PM, createFunctionAttrsPass());        // Deduce function attrs

  if (!DisableInline)
    addPass(PM, createFunctionInliningPass());   // Inline small functions
  addPass(PM, createArgumentPromotionPass());    // Scalarize uninlined fn args

#if LLVM_VERSION_CODE < LLVM_VERSION(3, 4)
  addPass(PM, createSimplifyLibCallsPass());     // Library Call Optimizations
#endif
  addPass(PM, createInstructionCombiningPass()); // Cleanup for scalarrepl.
  addPass(PM, createJumpThreadingPass());        // Thread jumps.
  addPass(PM, createCFGSimplificationPass());    // Merge & remove BBs
  addPass(PM, createScalarReplAggregatesPass()); // Break up aggregate allocas
  addPass(PM, createInstructionCombiningPass()); // Combine silly seq's

  addPass(PM, createTailCallEliminationPass());  // Eliminate tail calls
  addPass(PM, createCFGSimplificationPass());    // Merge & remove BBs
  addPass(PM, createReassociatePass());          // Reassociate expressions
  addPass(PM, createLoopRotatePass());
  addPass(PM, createLICMPass());                 // Hoist loop invariants
  addPass(PM, createLoopUnswitchPass());         // Unswitch loops.
  // FIXME : Removing instcombine causes nestedloop regression.
  addPass(PM, createInstructionCombiningPass());
  addPass(PM, createIndVarSimplifyPass());       // Canonicalize indvars
  addPass(PM, createLoopDeletionPass());         // Delete dead loops
  addPass(PM, createLoopUnrollPass());           // Unroll small loops
  addPass(PM, createInstructionCombiningPass()); // Clean up after the unroller
  addPass(PM, createGVNPass());                  // Remove redundancies
  addPass(PM, createMemCpyOptPass());            // Remove memcpy / form memset
  addPass(PM, createSCCPPass());                 // Constant prop with SCCP

  // Run instcombine after redundancy elimination to exploit opportunities
  // opened up by them.
  addPass(PM, createInstructionCombiningPass());

  addPass(PM, createDeadStoreEliminationPass()); // Delete dead stores
  addPass(PM, createAggressiveDCEPass());        // Delete dead instructions
  addPass(PM, createCFGSimplificationPass());    // Merge & remove BBs
  addPass(PM, createStripDeadPrototypesPass());  // Get rid of dead prototypes
#if LLVM_VERSION_CODE < LLVM_VERSION(3, 0)
  addPass(PM, createDeadTypeEliminationPass());  // Eliminate dead types
#endif
  addPass(PM, createConstantMergePass());        // Merge dup global constants
}

/// Optimize - Perform link time optimizations. This will run the scalar
/// optimizations, any loaded plugin-optimization modules, and then the
/// inter-procedural optimizations if applicable.
void Optimize(Module* M) {

  // Instantiate the pass manager to organize the passes.
  PassManager Passes;

  // If we're verifying, start off with a verification pass.
  if (VerifyEach)
    Passes.add(createVerifierPass());

#if LLVM_VERSION_CODE <= LLVM_VERSION(3, 1)
  // Add an appropriate TargetData instance for this module...
  addPass(Passes, new TargetData(M));
#elif LLVM_VERSION_CODE < LLVM_VERSION(3, 5)
  // Add an appropriate DataLayout instance for this module...
  addPass(Passes, new DataLayout(M));
#else
  // Add an appropriate DataLayout instance for this module...
  addPass(Passes, new DataLayoutPass(M));
#endif

  // DWD - Run the opt standard pass list as well.
  AddStandardCompilePasses(Passes);

  if (!DisableOptimizations) {
    // Now that composite has been compiled, scan through the module, looking
    // for a main function.  If main is defined, mark all other functions
    // internal.
    if (!DisableInternalize) {
#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 2)
      ModulePass *pass = createInternalizePass(std::vector<const char *>(1, "main"));
#else
      ModulePass *pass = createInternalizePass(true);
#endif
      addPass(Passes, pass);
    }

    // Propagate constants at call sites into the functions they call.  This
    // opens opportunities for globalopt (and inlining) by substituting function
    // pointers passed as arguments to direct uses of functions.  
    addPass(Passes, createIPSCCPPass());

    // Now that we internalized some globals, see if we can hack on them!
    addPass(Passes, createGlobalOptimizerPass());

    // Linking modules together can lead to duplicated global constants, only
    // keep one copy of each constant...
    addPass(Passes, createConstantMergePass());

    // Remove unused arguments from functions...
    addPass(Passes, createDeadArgEliminationPass());

    // Reduce the code after globalopt and ipsccp.  Both can open up significant
    // simplification opportunities, and both can propagate functions through
    // function pointers.  When this happens, we often have to resolve varargs
    // calls, etc, so let instcombine do this.
    addPass(Passes, createInstructionCombiningPass());

    if (!DisableInline)
      addPass(Passes, createFunctionInliningPass()); // Inline small functions

    addPass(Passes, createPruneEHPass());            // Remove dead EH info
    addPass(Passes, createGlobalOptimizerPass());    // Optimize globals again.
    addPass(Passes, createGlobalDCEPass());          // Remove dead functions

    // If we didn't decide to inline a function, check to see if we can
    // transform it to pass arguments by value instead of by reference.
    addPass(Passes, createArgumentPromotionPass());

    // The IPO passes may leave cruft around.  Clean up after them.
    addPass(Passes, createInstructionCombiningPass());
    addPass(Passes, createJumpThreadingPass());        // Thread jumps.
    addPass(Passes, createScalarReplAggregatesPass()); // Break up allocas

    // Run a few AA driven optimizations here and now, to cleanup the code.
    addPass(Passes, createFunctionAttrsPass());      // Add nocapture
    addPass(Passes, createGlobalsModRefPass());      // IP alias analysis

    addPass(Passes, createLICMPass());               // Hoist loop invariants
    addPass(Passes, createGVNPass());                // Remove redundancies
    addPass(Passes, createMemCpyOptPass());          // Remove dead memcpy's
    addPass(Passes, createDeadStoreEliminationPass()); // Nuke dead stores

    // Cleanup and simplify the code after the scalar optimizations.
    addPass(Passes, createInstructionCombiningPass());

    addPass(Passes, createJumpThreadingPass());        // Thread jumps.
    addPass(Passes, createPromoteMemoryToRegisterPass()); // Cleanup jumpthread.
    
    // Delete basic blocks, which optimization passes may have killed...
    addPass(Passes, createCFGSimplificationPass());

    // Now that we have optimized the program, discard unreachable functions...
    addPass(Passes, createGlobalDCEPass());
  }

  // If the -s or -S command line options were specified, strip the symbols out
  // of the resulting program to make it smaller.  -s and -S are GNU ld options
  // that we are supporting; they alias -strip-all and -strip-debug.
  if (Strip || StripDebug)
    addPass(Passes, createStripSymbolsPass(StripDebug && !Strip));

#if 0
  // Create a new optimization pass for each one specified on the command line
  std::auto_ptr<TargetMachine> target;
  for (unsigned i = 0; i < OptimizationList.size(); ++i) {
    const PassInfo *Opt = OptimizationList[i];
    if (Opt->getNormalCtor())
      addPass(Passes, Opt->getNormalCtor()());
    else
      llvm::errs() << "llvm-ld: cannot create pass: " << Opt->getPassName()
                << "\n";
  }
#endif

  // The user's passes may leave cruft around. Clean up after them them but
  // only if we haven't got DisableOptimizations set
  if (!DisableOptimizations) {
    addPass(Passes, createInstructionCombiningPass());
    addPass(Passes, createCFGSimplificationPass());
    addPass(Passes, createAggressiveDCEPass());
    addPass(Passes, createGlobalDCEPass());
  }

  // Make sure everything is still good.
  if (!DontVerify)
    Passes.add(createVerifierPass());

  // Run our queue of passes all at once now, efficiently.
  Passes.run(*M);
}

}
