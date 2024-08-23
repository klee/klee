//===-- Executor.cpp ------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "Executor.h"

#include "AddressSpace.h"
#include "CXXTypeSystem/CXXTypeManager.h"
#include "ConstructStorage.h"
#include "CoreStats.h"
#include "DistanceCalculator.h"
#include "ExecutionState.h"
#include "ExternalDispatcher.h"
#include "GetElementPtrTypeIterator.h"
#include "ImpliedValue.h"
#include "Memory.h"
#include "MemoryManager.h"
#include "PForest.h"
#include "PTree.h"
#include "Searcher.h"
#include "SeedInfo.h"
#include "SpecialFunctionHandler.h"
#include "StatsTracker.h"
#include "TargetCalculator.h"
#include "TargetManager.h"
#include "TargetedExecutionManager.h"
#include "TimingSolver.h"
#include "TypeManager.h"
#include "UserSearcher.h"
#include "klee/ADT/Bits.h"
#include "klee/ADT/SparseStorage.h"
#include "klee/Core/Context.h"

#include "klee/ADT/KTest.h"
#include "klee/Config/Version.h"
#include "klee/Config/config.h"
#include "klee/Core/Interpreter.h"
#include "klee/Core/MockBuilder.h"
#include "klee/Core/TerminationTypes.h"
#include "klee/Expr/ArrayExprOptimizer.h"
#include "klee/Expr/ArrayExprVisitor.h"
#include "klee/Expr/Assignment.h"
#include "klee/Expr/Constraints.h"
#include "klee/Expr/Expr.h"
#include "klee/Expr/ExprHashMap.h"
#include "klee/Expr/ExprPPrinter.h"
#include "klee/Expr/ExprSMTLIBPrinter.h"
#include "klee/Expr/ExprUtil.h"
#include "klee/Expr/SymbolicSource.h"
#include "klee/Expr/Symcrete.h"
#include "klee/Module/Cell.h"
#include "klee/Module/CodeGraphInfo.h"
#include "klee/Module/KCallable.h"
#include "klee/Module/KInstruction.h"
#include "klee/Module/KModule.h"
#include "klee/Module/KType.h"
#include "klee/Module/SarifReport.h"
#include "klee/Solver/Common.h"
#include "klee/Solver/Solver.h"
#include "klee/Solver/SolverCmdLine.h"
#include "klee/Statistics/TimerStatIncrementer.h"
#include "klee/Support/Casting.h"
#include "klee/Support/ErrorHandling.h"
#include "klee/Support/FileHandling.h"
#include "klee/Support/ModuleUtil.h"
#include "klee/Support/OptionCategories.h"
#include "klee/Support/RoundingModeUtil.h"
#include "klee/System/MemoryUsage.h"
#include "klee/System/Time.h"

#include "CodeEvent.h"
#include "CodeLocation.h"
#include "EventRecorder.h"

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/IR/Attributes.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Operator.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Process.h"
#include "llvm/Support/raw_ostream.h"

#include <algorithm>
#include <cassert>
#include <cxxabi.h>
#include <iosfwd>
#include <iostream>
#include <sys/mman.h>
#include <sys/resource.h>
#include <type_traits>
#include <utility>
#include <vector>

#ifdef HAVE_CTYPE_EXTERNALS
#include <ctype.h>
#endif

using namespace llvm;
using namespace klee;

namespace klee {
cl::OptionCategory ExecCat("Execution option",
                           "These options configure execution.");

cl::OptionCategory DebugCat("Debugging options",
                            "These are debugging options.");

cl::OptionCategory ExtCallsCat("External call policy options",
                               "These options impact external calls.");

cl::OptionCategory SeedingCat(
    "Seeding options",
    "These options are related to the use of seeds to start exploration.");

cl::OptionCategory TestGenCat("Test generation options",
                              "These options impact test generation.");

cl::OptionCategory LazyInitCat("Lazy initialization option",
                               "These options configure lazy initialization.");

cl::opt<bool> UseAdvancedTypeSystem(
    "use-advanced-type-system",
    cl::desc("Use advanced information about type system from "
             "language (default=false)"),
    cl::init(false), cl::cat(ExecCat));

cl::opt<bool> MergedPointerDereference(
    "use-merged-pointer-dereference", cl::init(false),
    cl::desc("Enable merged pointer dereference (default=false)"),
    cl::cat(ExecCat));

cl::opt<bool> UseTypeBasedAliasAnalysis(
    "use-tbaa",
    cl::desc("Turns on restrictions based on types compatibility for "
             "symbolic pointers (default=false)"),
    cl::init(false), cl::cat(ExecCat));

cl::opt<bool>
    AlignSymbolicPointers("align-symbolic-pointers",
                          cl::desc("Makes symbolic pointers aligned according"
                                   "to the used type system (default=false)"),
                          cl::init(false), cl::cat(ExecCat));

cl::opt<bool>
    ExternCallsCanReturnNull("extern-calls-can-return-null", cl::init(false),
                             cl::desc("Enable case when extern call can crash "
                                      "and return null (default=false)"),
                             cl::cat(ExecCat));

cl::opt<size_t> OSCopySizeMemoryCheckThreshold(
    "os-copy-size-mem-check-threshold", cl::init(30000),
    cl::desc("Check memory usage when this amount of bytes dense OS is copied"),
    cl::cat(ExecCat));

cl::opt<size_t> StackCopySizeMemoryCheckThreshold(
    "stack-copy-size-mem-check-threshold", cl::init(30000),
    cl::desc("Check memory usage when state with stack this big (in bytes) is "
             "copied"),
    cl::cat(ExecCat));

namespace {

/*** Lazy initialization options ***/

enum class LazyInitializationPolicy {
  None,
  Only,
  All,
};

cl::opt<LazyInitializationPolicy> LazyInitialization(
    "use-lazy-initialization",
    cl::values(clEnumValN(LazyInitializationPolicy::None, "none",
                          "Disable lazy initialization."),
               clEnumValN(LazyInitializationPolicy::Only, "only",
                          "Only lazy initilization without resolving."),
               clEnumValN(LazyInitializationPolicy::All, "all",
                          "Enable lazy initialization (default).")),
    cl::init(LazyInitializationPolicy::All), cl::cat(LazyInitCat));

llvm::cl::opt<bool> UseSymbolicSizeLazyInit(
    "use-sym-size-li",
    llvm::cl::desc(
        "Allows lazy initialize symbolic size objects (default false)"),
    llvm::cl::init(false), llvm::cl::cat(LazyInitCat));

llvm::cl::opt<unsigned> MinElementSizeLazyInit(
    "min-element-size-li",
    llvm::cl::desc("Minimum size of an element of array for one lazy "
                   "initialization (default 4)"),
    llvm::cl::init(4), llvm::cl::cat(LazyInitCat));

llvm::cl::opt<unsigned> MinNumberElementsLazyInit(
    "min-number-elements-li",
    llvm::cl::desc("Minimum number of array elements for one lazy "
                   "initialization (default 4)"),
    llvm::cl::init(4), llvm::cl::cat(LazyInitCat));
} // namespace

cl::opt<std::string> FunctionCallReproduce(
    "function-call-reproduce", cl::init(""),
    cl::desc("Marks mentioned function as target for error-guided mode."),
    cl::cat(ExecCat));

llvm::cl::opt<bool> X86FPAsX87FP80(
    "x86FP-as-x87FP80", cl::init(false),
    cl::desc("Convert X86 fp values to X87FP80 during computation according to "
             "GCC behavior (default=false)"),
    cl::cat(ExecCat));

cl::opt<HaltExecution::Reason> DumpStatesOnHalt(
    "dump-states-on-halt", cl::init(HaltExecution::Reason::Unspecified),
    cl::values(
        clEnumValN(HaltExecution::Reason::NotHalt, "none",
                   "Do not dump test cases for all active states on exit."),
        clEnumValN(HaltExecution::Reason::Unspecified, "all",
                   "Dump test cases for all active states on exit (default)")),
    cl::cat(TestGenCat));
} // namespace klee

namespace {

/*** Test generation options ***/

cl::opt<bool> OnlyOutputStatesCoveringNew(
    "only-output-states-covering-new", cl::init(false),
    cl::desc("Only output test cases covering new code (default=false)"),
    cl::cat(TestGenCat));

cl::opt<bool> EmitAllErrors(
    "emit-all-errors", cl::init(false),
    cl::desc("Generate tests cases for all errors "
             "(default=false, i.e. one per (error,instruction) pair)"),
    cl::cat(TestGenCat));

cl::opt<bool> CoverOnTheFly(
    "cover-on-the-fly", cl::init(false),
    cl::desc("Generate tests cases for each new covered block or branch "
             "(default=false)"),
    cl::cat(TestGenCat));

cl::opt<bool> MemoryTriggerCoverOnTheFly(
    "mem-trigger-cof", cl::init(false),
    cl::desc("Start on the fly tests generation after approaching memory cup"
             "(default=false)"),
    cl::cat(TestGenCat));

cl::opt<std::string> DelayCoverOnTheFly(
    "delay-cover-on-the-fly", cl::init("0s"),
    cl::desc("Start on the fly tests generation after the specified duration. "
             "Set to 0s to disable (default=0s)"),
    cl::cat(TestGenCat));

cl::opt<unsigned> UninitMemoryTestMultiplier(
    "uninit-memory-test-multiplier", cl::init(6),
    cl::desc("Generate additional number of duplicate tests due to "
             "irreproducibility of uninitialized memory "
             "(default=6)"),
    cl::cat(TestGenCat));

/* Constraint solving options */

cl::opt<unsigned> MaxSymArraySize(
    "max-sym-array-size",
    cl::desc(
        "If a symbolic array exceeds this size (in bytes), symbolic addresses "
        "into this array are concretized.  Set to 0 to disable (default=0)"),
    cl::init(0), cl::cat(SolvingCat));

cl::opt<bool>
    SimplifySymIndices("simplify-sym-indices", cl::init(true),
                       cl::desc("Simplify symbolic accesses using equalities "
                                "from other constraints (default=true)"),
                       cl::cat(SolvingCat));

cl::opt<bool>
    EqualitySubstitution("equality-substitution", cl::init(true),
                         cl::desc("Simplify equality expressions before "
                                  "querying the solver (default=true)"),
                         cl::cat(SolvingCat));

cl::opt<bool> OnlyOutputMakeSymbolicArrays(
    "only-output-make-symbolic-arrays", cl::init(false),
    cl::desc(
        "Only output test data with klee_make_symbolic source (default=false)"),
    cl::cat(TestGenCat));

/*** External call policy options ***/

enum class ExternalCallPolicy {
  None,     // No external calls allowed
  Concrete, // Only external calls with concrete arguments allowed
  All,      // All external calls allowed
};

cl::opt<ExternalCallPolicy> ExternalCalls(
    "external-calls", cl::desc("Specify the external call policy"),
    cl::values(
        clEnumValN(
            ExternalCallPolicy::None, "none",
            "No external function calls are allowed.  Note that KLEE always "
            "allows some external calls with concrete arguments to go through "
            "(in particular printf and puts), regardless of this option."),
        clEnumValN(ExternalCallPolicy::Concrete, "concrete",
                   "Only external function calls with concrete arguments are "
                   "allowed (default)"),
        clEnumValN(ExternalCallPolicy::All, "all",
                   "All external function calls are allowed.  This concretizes "
                   "any symbolic arguments in calls to external functions.")),
    cl::init(ExternalCallPolicy::Concrete), cl::cat(ExtCallsCat));

cl::opt<bool> SuppressExternalWarnings(
    "suppress-external-warnings", cl::init(false),
    cl::desc("Supress warnings about calling external functions."),
    cl::cat(ExtCallsCat));

cl::opt<bool> AllExternalWarnings(
    "all-external-warnings", cl::init(false),
    cl::desc("Issue a warning everytime an external call is made, "
             "as opposed to once per function (default=false)"),
    cl::cat(ExtCallsCat));

/*** Seeding options ***/

cl::opt<bool> AlwaysOutputSeeds(
    "always-output-seeds", cl::init(true),
    cl::desc(
        "Dump test cases even if they are driven by seeds only (default=true)"),
    cl::cat(SeedingCat));

cl::opt<bool> OnlyReplaySeeds(
    "only-replay-seeds", cl::init(false),
    cl::desc("Discard states that do not have a seed (default=false)."),
    cl::cat(SeedingCat));

cl::opt<bool> OnlySeed("only-seed", cl::init(false),
                       cl::desc("Stop execution after seeding is done without "
                                "doing regular search (default=false)."),
                       cl::cat(SeedingCat));

cl::opt<bool>
    AllowSeedExtension("allow-seed-extension", cl::init(false),
                       cl::desc("Allow extra (unbound) values to become "
                                "symbolic during seeding (default=false)."),
                       cl::cat(SeedingCat));

cl::opt<bool> ZeroSeedExtension(
    "zero-seed-extension", cl::init(false),
    cl::desc(
        "Use zero-filled objects if matching seed not found (default=false)"),
    cl::cat(SeedingCat));

cl::opt<bool> AllowSeedTruncation(
    "allow-seed-truncation", cl::init(false),
    cl::desc("Allow smaller buffers than in seeds (default=false)."),
    cl::cat(SeedingCat));

cl::opt<bool> NamedSeedMatching(
    "named-seed-matching", cl::init(false),
    cl::desc("Use names to match symbolic objects to inputs (default=false)."),
    cl::cat(SeedingCat));

cl::opt<std::string>
    SeedTime("seed-time",
             cl::desc("Amount of time to dedicate to seeds, before normal "
                      "search (default=0s (off))"),
             cl::cat(SeedingCat));

/*** Debugging options ***/

/// The different query logging solvers that can switched on/off
enum PrintDebugInstructionsType {
  STDERR_ALL, ///
  STDERR_SRC,
  STDERR_COMPACT,
  FILE_ALL,    ///
  FILE_SRC,    ///
  FILE_COMPACT ///
};

llvm::cl::bits<PrintDebugInstructionsType> DebugPrintInstructions(
    "debug-print-instructions",
    llvm::cl::desc("Log instructions during execution."),
    llvm::cl::values(
        clEnumValN(STDERR_ALL, "all:stderr",
                   "Log all instructions to stderr "
                   "in format [src, inst_id, "
                   "llvm_inst]"),
        clEnumValN(STDERR_SRC, "src:stderr",
                   "Log all instructions to stderr in format [src, inst_id]"),
        clEnumValN(STDERR_COMPACT, "compact:stderr",
                   "Log all instructions to stderr in format [inst_id]"),
        clEnumValN(FILE_ALL, "all:file",
                   "Log all instructions to file "
                   "instructions.txt in format [src, "
                   "inst_id, llvm_inst]"),
        clEnumValN(FILE_SRC, "src:file",
                   "Log all instructions to file "
                   "instructions.txt in format [src, "
                   "inst_id]"),
        clEnumValN(FILE_COMPACT, "compact:file",
                   "Log all instructions to file instructions.txt in format "
                   "[inst_id]")),
    llvm::cl::CommaSeparated, cl::cat(DebugCat));

#ifdef HAVE_ZLIB_H
cl::opt<bool> DebugCompressInstructions(
    "debug-compress-instructions", cl::init(false),
    cl::desc(
        "Compress the logged instructions in gzip format (default=false)."),
    cl::cat(DebugCat));
#endif

cl::opt<bool> DebugCheckForImpliedValues(
    "debug-check-for-implied-values", cl::init(false),
    cl::desc("Debug the implied value optimization"), cl::cat(DebugCat));

bool allLeafsAreConstant(const ref<Expr> &expr) {
  if (isa<klee::ConstantExpr>(expr)) {
    return true;
  }

  if (isa<klee::ConstantPointerExpr>(expr)) {
    return true;
  }

  if (!isa<SelectExpr>(expr)) {
    return false;
  }

  const SelectExpr *sel = cast<SelectExpr>(expr);

  std::vector<ref<Expr>> alternatives;
  ArrayExprHelper::collectAlternatives(*sel, alternatives);

  for (auto leaf : alternatives) {
    if (!isa<klee::ConstantExpr>(leaf)) {
      return false;
    }
  }

  return true;
}

} // namespace

extern llvm::cl::opt<unsigned long> MaxConstantAllocationSize;
extern llvm::cl::opt<unsigned long> MaxSymbolicAllocationSize;
extern llvm::cl::opt<bool> UseSymbolicSizeAllocation;
extern llvm::cl::opt<TrackCoverageBy> TrackCoverage;

// XXX hack
extern "C" unsigned dumpStates, dumpPForest;
unsigned dumpStates = 0, dumpPForest = 0;

bool Interpreter::hasTargetForest() const { return false; }

const std::unordered_set<Intrinsic::ID> Executor::supportedFPIntrinsics = {
    // Intrinsic::fabs and Intrinsic::fma handled individually because of thier
    // presence in
    // mainline KLEE
    Intrinsic::sqrt, Intrinsic::maxnum, Intrinsic::minnum, Intrinsic::trunc,
    Intrinsic::rint};

const std::unordered_set<Intrinsic::ID> Executor::modelledFPIntrinsics = {
    Intrinsic::ceil, Intrinsic::copysign, Intrinsic::cos,   Intrinsic::exp2,
    Intrinsic::exp,  Intrinsic::floor,    Intrinsic::log10, Intrinsic::log2,
    Intrinsic::log,  Intrinsic::round,    Intrinsic::sin};

Executor::Executor(LLVMContext &ctx, const InterpreterOptions &opts,
                   InterpreterHandler *ih)
    : Interpreter(opts), interpreterHandler(ih), searcher(nullptr),
      externalDispatcher(new ExternalDispatcher(ctx)), statsTracker(0),
      pathWriter(0), symPathWriter(0), specialFunctionHandler(0),
      timers{time::Span(TimerInterval)}, guidanceKind(opts.Guidance),
      codeGraphInfo(new CodeGraphInfo()),
      distanceCalculator(new DistanceCalculator(*codeGraphInfo)),
      targetCalculator(new TargetCalculator(*codeGraphInfo)),
      targetManager(new TargetManager(guidanceKind, *distanceCalculator,
                                      *targetCalculator)),
      targetedExecutionManager(
          new TargetedExecutionManager(*codeGraphInfo, *targetManager)),
      replayKTest(0), replayPath(0), usingSeeds(0), atMemoryLimit(false),
      inhibitForking(false), coverOnTheFly(false),
      haltExecution(HaltExecution::NotHalt), ivcEnabled(false),
      debugLogBuffer(debugBufferString), sarifReport({}) {

  objectManager = std::make_unique<ObjectManager>();
  seedMap = std::make_unique<SeedMap>();
  objectManager->addSubscriber(seedMap.get());

  // Add first entry for single run
  sarifReport.version = "2.1.0";
  sarifReport.runs.push_back(RunJson{{}, ih->info()});

  if (interpreterOpts.MockStrategy == MockStrategyKind::Deterministic &&
      CoreSolverToUse != Z3_SOLVER) {
    klee_error("Deterministic mocks can be generated with Z3 solver only.\n");
  }
  if (interpreterOpts.MockStrategy == MockStrategyKind::Deterministic &&
      interpreterOpts.Mock != MockPolicy::All) {
    klee_error("Deterministic mocks can be generated only with "
               "`--mock-policy=all`.\n");
  }

  const time::Span maxTime{MaxTime};
  if (maxTime)
    timers.add(std::make_unique<Timer>(maxTime, [&] {
      klee_message("HaltTimer invoked");
      setHaltExecution(HaltExecution::MaxTime);
    }));

  if (CoverOnTheFly && guidanceKind != GuidanceKind::ErrorGuidance) {
    const time::Span delayTime{DelayCoverOnTheFly};
    if (delayTime)
      timers.add(
          std::make_unique<Timer>(delayTime, [&] { coverOnTheFly = true; }));
  }

  coreSolverTimeout = time::Span{MaxCoreSolverTime};
  if (coreSolverTimeout)
    UseForkedCoreSolver = true;
  std::unique_ptr<Solver> coreSolver = klee::createCoreSolver(CoreSolverToUse);
  if (!coreSolver) {
    klee_error("Failed to create core solver\n");
  }

  memory = std::make_unique<MemoryManager>();

  std::unique_ptr<Solver> solver = constructSolverChain(
      std::move(coreSolver),
      interpreterHandler->getOutputFilename(ALL_QUERIES_SMT2_FILE_NAME),
      interpreterHandler->getOutputFilename(SOLVER_QUERIES_SMT2_FILE_NAME),
      interpreterHandler->getOutputFilename(ALL_QUERIES_KQUERY_FILE_NAME),
      interpreterHandler->getOutputFilename(SOLVER_QUERIES_KQUERY_FILE_NAME));

  this->solver = std::make_unique<TimingSolver>(std::move(solver), optimizer,
                                                EqualitySubstitution);
  initializeSearchOptions();

  if (DebugPrintInstructions.isSet(FILE_ALL) ||
      DebugPrintInstructions.isSet(FILE_COMPACT) ||
      DebugPrintInstructions.isSet(FILE_SRC)) {
    std::string debug_file_name =
        interpreterHandler->getOutputFilename("instructions.txt");
    std::string error;
#ifdef HAVE_ZLIB_H
    if (!DebugCompressInstructions) {
#endif
      debugInstFile = klee_open_output_file(debug_file_name, error);
#ifdef HAVE_ZLIB_H
    } else {
      debug_file_name.append(".gz");
      debugInstFile = klee_open_compressed_output_file(debug_file_name, error);
    }
#endif
    if (!debugInstFile) {
      klee_error("Could not open file %s : %s", debug_file_name.c_str(),
                 error.c_str());
    }
  }
}

llvm::Module *Executor::setModule(
    std::vector<std::unique_ptr<llvm::Module>> &userModules,
    std::vector<std::unique_ptr<llvm::Module>> &libsModules,
    const ModuleOptions &opts, std::set<std::string> &&mainModuleFunctions,
    std::set<std::string> &&mainModuleGlobals, FLCtoOpcode &&origInstructions,
    const std::set<std::string> &ignoredExternals,
    std::vector<std::pair<std::string, std::string>> redefinitions) {
  assert(!kmodule && !userModules.empty() &&
         "can only register one module"); // XXX gross

  kmodule = std::make_unique<KModule>();

  // 1.) Link the modules together && 2.) Apply different instrumentation
  kmodule->link(userModules, 1);
  kmodule->instrument(opts);

  kmodule->link(libsModules, 2);
  kmodule->instrument(opts);

  {
    std::vector<std::unique_ptr<llvm::Module>> modules;
    // Link with KLEE intrinsics library before running any optimizations
    SmallString<128> LibPath(opts.LibraryDir);
    llvm::sys::path::append(LibPath, "libkleeRuntimeIntrinsic" +
                                         opts.OptSuffix + ".bca");
    std::string error;
    if (!klee::loadFileAsOneModule(
            LibPath.c_str(), kmodule->module->getContext(), modules, error)) {
      klee_error("Could not load KLEE intrinsic file %s", LibPath.c_str());
    }
    kmodule->link(modules, 2);
    kmodule->instrument(opts);
  }

  if (interpreterOpts.Mock == MockPolicy::All ||
      interpreterOpts.MockMutableGlobals == MockMutableGlobalsPolicy::All ||
      !opts.AnnotationsFile.empty()) {
    MockBuilder mockBuilder(kmodule->module.get(), opts, interpreterOpts,
                            ignoredExternals, redefinitions, interpreterHandler,
                            mainModuleFunctions, mainModuleGlobals);
    std::unique_ptr<llvm::Module> mockModule = mockBuilder.build();

    std::vector<std::unique_ptr<llvm::Module>> mockModules;
    mockModules.push_back(std::move(mockModule));
    // std::swap(mockModule, kmodule->module);
    kmodule->link(mockModules, 1);
    klee_message("Mock linkage: done");

    for (auto &global : kmodule->module->globals()) {
      if (global.isDeclaration()) {
        llvm::Constant *zeroInitializer =
            llvm::Constant::getNullValue(global.getValueType());
        if (!zeroInitializer) {
          klee_error("Unable to get zero initializer for '%s'",
                     global.getName().str().c_str());
        }
        global.setInitializer(zeroInitializer);
      }
    }
  }

  // 3.) Optimise and prepare for KLEE

  // Create a list of functions that should be preserved if used
  std::vector<const char *> preservedFunctions;
  specialFunctionHandler = new SpecialFunctionHandler(*this);
  specialFunctionHandler->prepare(preservedFunctions);

  // Preserve the free-standing library calls
  preservedFunctions.push_back("memset");
  preservedFunctions.push_back("memcpy");
  preservedFunctions.push_back("memcmp");
  preservedFunctions.push_back("memmove");

  if (FunctionCallReproduce != "") {
    preservedFunctions.push_back(FunctionCallReproduce.c_str());
  }

  // prevent elimination of the preservedFunctions functions
  for (auto pf : preservedFunctions) {
    auto f = kmodule->module->getFunction(pf);
    if (f) {
      f->addFnAttr(Attribute::OptimizeNone);
      f->addFnAttr(Attribute::NoInline);
    }
  }

  // except the entry point
  preservedFunctions.push_back(opts.EntryPoint.c_str());

  kmodule->optimiseAndPrepare(opts, preservedFunctions);
  kmodule->checkModule();

  // 4.) Manifest the module
  std::swap(kmodule->mainModuleFunctions, mainModuleFunctions);
  std::swap(kmodule->mainModuleGlobals, mainModuleGlobals);
  kmodule->manifest(interpreterHandler, interpreterOpts.Guidance,
                    StatsTracker::useStatistics());

  kmodule->origInstructions = origInstructions;

  specialFunctionHandler->bind();

  initializeTypeManager();

  if (StatsTracker::useStatistics() || userSearcherRequiresMD2U()) {
    statsTracker = new StatsTracker(
        *this, interpreterHandler->getOutputFilename("assembly.ll"),
        userSearcherRequiresMD2U());
  }

  // Initialize the context.
  DataLayout *TD = kmodule->targetData.get();
  Context::initialize(TD->isLittleEndian(),
                      (Expr::Width)TD->getPointerSizeInBits());

  return kmodule->module.get();
}

void Executor::setFunctionsByModule(FunctionsByModule &&fnsByModule) {
  functionsByModule = std::forward<FunctionsByModule>(fnsByModule);
}

Executor::~Executor() {
  delete typeSystemManager;
  delete externalDispatcher;
  delete specialFunctionHandler;
  delete statsTracker;
}

/***/

ref<Expr> X87FP80ToFPTrunc(ref<Expr> arg, [[maybe_unused]] Expr::Width type,
                           [[maybe_unused]] llvm::APFloat::roundingMode rm) {
  ref<Expr> result = arg;
#ifdef ENABLE_FP
  Expr::Width resultType = type;
  if (Context::get().getPointerWidth() == 32 && arg->getWidth() == Expr::Fl80) {
    result = FPTruncExpr::create(arg, resultType, rm);
  }
#else
  klee_message(
      "You may enable x86-as-x87FP80 behaviour by passing the following options"
      " to cmake:\n"
      "\"-DENABLE_FLOATING_POINT=ON\"\n");
#endif
  return result;
}

ref<Expr> FPToX87FP80Ext(ref<Expr> arg) {
  ref<Expr> result = arg;
#ifdef ENABLE_FP
  Expr::Width resultType = Expr::Fl80;
  if (Context::get().getPointerWidth() == 32) {
    result = FPExtExpr::create(arg, resultType);
  }
#else
  klee_message(
      "You may enable x86-as-x87FP80 behaviour by passing the following options"
      " to cmake:\n"
      "\"-DENABLE_FLOATING_POINT=ON\"\n");
#endif
  return result;
}

void Executor::initializeGlobalObject(ExecutionState &state, ObjectState *os,
                                      const Constant *c, unsigned offset) {
  const auto targetData = kmodule->targetData.get();
  if (const ConstantVector *cp = dyn_cast<ConstantVector>(c)) {
    unsigned elementSize =
        targetData->getTypeStoreSize(cp->getType()->getElementType());
    for (unsigned i = 0, e = cp->getNumOperands(); i != e; ++i)
      initializeGlobalObject(state, os, cp->getOperand(i),
                             offset + i * elementSize);
  } else if (isa<ConstantAggregateZero>(c)) {
    unsigned i, size = targetData->getTypeStoreSize(c->getType());
    if (offset) {
      for (i = 0; i < size; i++)
        os->write8(offset + i, (uint8_t)0);
    } else {
      os->initializeToZero();
    }
  } else if (const ConstantArray *ca = dyn_cast<ConstantArray>(c)) {
    unsigned elementSize =
        targetData->getTypeStoreSize(ca->getType()->getElementType());
    for (unsigned i = 0, e = ca->getNumOperands(); i != e; ++i)
      initializeGlobalObject(state, os, ca->getOperand(i),
                             offset + i * elementSize);
  } else if (const ConstantStruct *cs = dyn_cast<ConstantStruct>(c)) {
    const StructLayout *sl =
        targetData->getStructLayout(cast<StructType>(cs->getType()));
    for (unsigned i = 0, e = cs->getNumOperands(); i != e; ++i)
      initializeGlobalObject(state, os, cs->getOperand(i),
                             offset + sl->getElementOffset(i));
  } else if (const ConstantDataSequential *cds =
                 dyn_cast<ConstantDataSequential>(c)) {
    unsigned elementSize = targetData->getTypeStoreSize(cds->getElementType());
    for (unsigned i = 0, e = cds->getNumElements(); i != e; ++i)
      initializeGlobalObject(state, os, cds->getElementAsConstant(i),
                             offset + i * elementSize);
  } else if (!isa<UndefValue>(c) && !isa<MetadataAsValue>(c)) {
    unsigned StoreBits = targetData->getTypeStoreSizeInBits(c->getType());
    ref<Expr> C = evalConstant(c, state.roundingMode);

    if (c->getType()->isFloatingPointTy() &&
        Context::get().getPointerWidth() == 32) {
      C = X87FP80ToFPTrunc(C, getWidthForLLVMType(c->getType()),
                           state.roundingMode);
    }

    // Extend the constant if necessary;
    assert(StoreBits >= C->getWidth() && "Invalid store size!");
    if (StoreBits > C->getWidth())
      C = ZExtExpr::create(C, StoreBits);

    os->write(offset, C);
  }
}

ObjectPair Executor::addExternalObjectAsNonStatic(ExecutionState &state,
                                                  KType *type, unsigned size,
                                                  bool isReadOnly) {
  auto mo =
      allocate(state, Expr::createPointer(size), false, true, nullptr, 8, type);
  mo->isFixed = true;

  auto os = bindObjectInState(state, mo, type, false);
  os->setReadOnly(isReadOnly);

  return ObjectPair{mo, os};
}

ObjectPair Executor::addExternalObject(ExecutionState &state, const void *addr,
                                       KType *type, unsigned size,
                                       bool isReadOnly) {
  auto mo = memory->allocateFixed(reinterpret_cast<std::uint64_t>(addr), size,
                                  nullptr, type);
  ObjectState *os = bindObjectInState(state, mo, type, false);
  ref<ConstantExpr> seg =
      Expr::createPointer(reinterpret_cast<std::uint64_t>(addr));
  for (unsigned i = 0; i < size; i++) {
    ref<Expr> byte = ConstantExpr::create(((uint8_t *)addr)[i], Expr::Int8);
    os->write(i, PointerExpr::create(seg, byte));
  }
  if (isReadOnly)
    os->setReadOnly(true);
  return {mo, os};
}

extern void *__dso_handle __attribute__((__weak__));

void Executor::initializeGlobals(ExecutionState &state) {
  // allocate and initialize globals, done in two passes since we may
  // need address of a global in order to initialize some other one.

  // allocate memory objects for all globals
  allocateGlobalObjects(state);

  // initialize aliases first, may be needed for global objects
  initializeGlobalAliases(state);

  // finally, do the actual initialization
  initializeGlobalObjects(state);
}

#ifdef HAVE_CTYPE_EXTERNALS
static const std::size_t kCTypeMemSize = 384;
static const std::size_t kCTypeMemOffset = 128;

template <typename F>
decltype(auto) Executor::addCTypeFixedObject(ExecutionState &state,
                                             int addressSpaceNum, Module &m,
                                             F objectProvider) {
  using underlying_t = decltype(**objectProvider());

  static_assert(std::is_integral_v<std::remove_reference_t<underlying_t>>,
                "ctype structure expected to contain integral variables");

  // Obtain types for required object
  llvm::Type *type =
      llvm::IntegerType::get(m.getContext(), sizeof(underlying_t) * CHAR_BIT);
  llvm::Type *pointerType = llvm::PointerType::get(type, addressSpaceNum);

  auto ctypeObj =
      addExternalObject(state,
                        const_cast<void *>(reinterpret_cast<const void *>(
                            *objectProvider() - kCTypeMemOffset)),
                        typeSystemManager->getWrappedType(type),
                        kCTypeMemSize * sizeof(underlying_t), true);

  auto ctypePointerObj = addExternalObject(
      state, objectProvider(), typeSystemManager->getWrappedType(pointerType),
      Context::get().getPointerWidthInBytes(), true);
  state.addressSpace
      .getWriteable(ctypePointerObj.first, ctypePointerObj.second)
      ->write(0,
              ConstantPointerExpr::create(
                  ctypeObj.first->getBaseExpr(),
                  AddExpr::create(ctypeObj.first->getBaseExpr(),
                                  Expr::createPointer(kCTypeMemOffset *
                                                      sizeof(underlying_t)))));
  return objectProvider();
}

template <typename F>
decltype(auto) Executor::addCTypeModelledObject(ExecutionState &state,
                                                int addressSpaceNum, Module &m,
                                                F objectProvider) {
  using underlying_t = decltype(**objectProvider());

  static_assert(std::is_integral_v<std::remove_reference_t<underlying_t>>,
                "ctype structure expected to contain integral variables");
  // Obtain types for required object
  llvm::Type *type =
      llvm::IntegerType::get(m.getContext(), sizeof(underlying_t) * CHAR_BIT);
  llvm::Type *pointerType = llvm::PointerType::get(type, addressSpaceNum);

  // Allocate memory
  auto ctypeObj = addExternalObjectAsNonStatic(
      state, typeSystemManager->getWrappedType(type),
      kCTypeMemSize * sizeof(underlying_t), true);
  auto ctypePointerObj = addExternalObjectAsNonStatic(
      state, typeSystemManager->getWrappedType(pointerType),
      Context::get().getPointerWidthInBytes(), true);

  // Write address of memory into pointer
  state.addressSpace
      .getWriteable(ctypePointerObj.first, ctypePointerObj.second)
      ->write(0, AddExpr::create(ctypeObj.first->getBaseExpr(),
                                 Expr::createPointer(kCTypeMemOffset *
                                                     sizeof(underlying_t))));

  // Write content from actiual ctype into modelled memory
  ref<ConstantExpr> seg = cast<ConstantExpr>(ctypeObj.first->getBaseExpr());
  auto addr =
      reinterpret_cast<const uint8_t *>(*objectProvider() - kCTypeMemOffset);
  for (unsigned i = 0; i < kCTypeMemSize * sizeof(underlying_t); i++) {
    ref<Expr> byte = ConstantExpr::create(addr[i], Expr::Int8);
    state.addressSpace.getWriteable(ctypeObj.first, ctypeObj.second)
        ->write(i, PointerExpr::create(seg, byte));
  }

  // Return address to pointer
  return reinterpret_cast<decltype(objectProvider())>(
      cast<ConstantExpr>(ctypePointerObj.first->getBaseExpr())->getZExtValue());
};
#endif

void Executor::allocateGlobalObjects(ExecutionState &state) {
  Module *m = kmodule->module.get();
  unsigned int addressSpaceNum = kmodule->targetData->getAllocaAddrSpace();

  if (m->getModuleInlineAsm() != "")
    klee_warning("executable has module level assembly (ignoring)");
  // represent function globals using the address of the actual llvm function
  // object. given that we use malloc to allocate memory in states this also
  // ensures that we won't conflict. we don't need to allocate a memory object
  // since reading/writing via a function pointer is unsupported anyway.
  for (Function &f : *m) {
    ref<ConstantPointerExpr> addr;

    // If the symbol has external weak linkage then it is implicitly
    // not defined in this module; if it isn't resolvable then it
    // should be null.
    if (f.hasExternalWeakLinkage() &&
        !externalDispatcher->resolveSymbol(f.getName().str())) {
      addr = ConstantPointerExpr::create(Expr::createPointer(0),
                                         Expr::createPointer(0));
    } else {
      // We allocate an object to represent each function,
      // its address can be used for function pointers.
      // TODO: Check whether the object is accessed?

      const KFunction *kf = kmodule->functionMap.at(&f);
      ref<CodeLocation> fCodeLocation = CodeLocation::create(
          kf, kf->getSourceFilepath(), kf->getLine(), std::nullopt);

      auto mo = allocate(state, Expr::createPointer(8), false, true,
                         fCodeLocation, 8, typeSystemManager->getUnknownType());
      auto baseExpr = cast<ConstantExpr>(mo->getBaseExpr());
      addr = ConstantPointerExpr::create(baseExpr, baseExpr);
      legalFunctions.emplace(baseExpr->getZExtValue(), &f);
    }

    globalAddresses.emplace(&f, addr);
  }

#ifndef WINDOWS

  llvm::Type *pointerErrnoAddr = llvm::PointerType::get(
      llvm::IntegerType::get(m->getContext(), sizeof(*errno_addr) * CHAR_BIT),
      addressSpaceNum);
  const MemoryObject *errnoObj = nullptr;

  if (Context::get().getPointerWidth() == 32) {
    errnoObj = addExternalObjectAsNonStatic(
                   state, typeSystemManager->getWrappedType(pointerErrnoAddr),
                   sizeof(*errno_addr), false)
                   .first;
    errno_addr = reinterpret_cast<int *>(
        cast<ConstantExpr>(errnoObj->getBaseExpr())->getZExtValue());
  } else {
    errno_addr = getErrnoLocation();
    errnoObj =
        addExternalObject(state, (void *)errno_addr,
                          typeSystemManager->getWrappedType(pointerErrnoAddr),
                          sizeof *errno_addr, false)
            .first;
  }

  // Copy values from and to program space explicitly
  const_cast<MemoryObject *>(errnoObj)->isUserSpecified = true;
#endif

  // Disabled, we don't want to promote use of live externals.
#ifdef HAVE_CTYPE_EXTERNALS
#ifndef WINDOWS
#ifndef DARWIN

  /* from /usr/include/ctype.h:
         These point into arrays of 384, so they can be indexed by any
     `unsigned char' value [0,255]; by EOF (-1); or by any `signed char' value
         [-128,-1).  ISO C requires that the ctype functions work for
     `unsigned */

  // We assume that KLEE is running on 64-bit platform.
  // Therefore, for 32-bit binaries we can not just use addresses
  // of actual ctype* objects in memory.
  // Hence, we need to model then in address space.
  if (Context::get().getPointerWidth() == 32) {
    c_type_b_loc_addr =
        addCTypeModelledObject(state, addressSpaceNum, *m, __ctype_b_loc);
    c_type_tolower_addr =
        addCTypeModelledObject(state, addressSpaceNum, *m, __ctype_tolower_loc);
    c_type_toupper_addr =
        addCTypeModelledObject(state, addressSpaceNum, *m, __ctype_toupper_loc);
  } else {
    c_type_b_loc_addr =
        addCTypeFixedObject(state, addressSpaceNum, *m, __ctype_b_loc);
    c_type_tolower_addr =
        addCTypeFixedObject(state, addressSpaceNum, *m, __ctype_tolower_loc);
    c_type_toupper_addr =
        addCTypeFixedObject(state, addressSpaceNum, *m, __ctype_toupper_loc);
  }
#endif
#endif
#endif

  for (const GlobalVariable &v : m->globals()) {
    std::size_t globalObjectAlignment = getAllocationAlignment(&v);
    Type *ty = v.getValueType();
    ref<Expr> size = Expr::createPointer(0);
    if (ty->isSized()) {
      // size includes padding
      size = Expr::createPointer(kmodule->targetData->getTypeAllocSize(ty));
    }
    if (v.isDeclaration()) {
      // FIXME: We have no general way of handling unknown external
      // symbols. If we really cared about making external stuff work
      // better we could support user definition, or use the EXE style
      // hack where we check the object file information.

      if (!ty->isSized()) {
        klee_warning("Type for %.*s is not sized",
                     static_cast<int>(v.getName().size()), v.getName().data());
      }

      // XXX - DWD - hardcode some things until we decide how to fix.
#ifndef WINDOWS
      if (v.getName() == "_ZTVN10__cxxabiv117__class_type_infoE") {
        size = Expr::createPointer(0x2C);
      } else if (v.getName() == "_ZTVN10__cxxabiv120__si_class_type_infoE") {
        size = Expr::createPointer(0x2C);
      } else if (v.getName() == "_ZTVN10__cxxabiv121__vmi_class_type_infoE") {
        size = Expr::createPointer(0x2C);
      }
#endif

      if (size->isZero()) {
        const Array *array = makeArray(
            Expr::createPointer(Context::get().getPointerWidthInBytes()),
            SourceBuilder::irreproducible(v.getName().str()));
        size = Expr::createTempRead(array, Context::get().getPointerWidth());
        klee_warning("Unable to find size for global variable: %.*s (model "
                     "size as symbolic with irreproducible content)",
                     static_cast<int>(v.getName().size()), v.getName().data());
      }
    }

    const KGlobalVariable *kv = kmodule->globalMap.at(&v).get();
    ref<CodeLocation> vCodeLocation = CodeLocation::create(
        kv, kv->getSourceFilepath(), kv->getLine(), std::nullopt);

    if (!isa<ConstantExpr>(size)) {
      addConstraint(
          state, UleExpr::create(
                     ZExtExpr::create(size, Context::get().getPointerWidth()),
                     Expr::createPointer(MaxSymbolicAllocationSize)));
    }

    MemoryObject *mo = allocate(state, size, /*isLocal=*/false,
                                /*isGlobal=*/true, /*allocSite=*/vCodeLocation,
                                /*alignment=*/globalObjectAlignment,
                                typeSystemManager->getWrappedType(ty));
    if (!mo)
      klee_error("out of memory");

    globalObjects.emplace(&v, mo);
    globalAddresses.emplace(
        &v, PointerExpr::create(mo->getBaseExpr(), mo->getBaseExpr()));
  }
}

void Executor::initializeGlobalAlias(const llvm::Constant *c,
                                     ExecutionState &state) {
  // aliasee may either be a global value or constant expression
  const auto *ga = dyn_cast<GlobalAlias>(c);
  if (ga) {
    if (globalAddresses.count(ga)) {
      // already resolved by previous invocation
      return;
    }
    const llvm::Constant *aliasee = ga->getAliasee();
    if (const auto *gv = dyn_cast<GlobalValue>(aliasee)) {
      // aliasee is global value
      auto it = globalAddresses.find(gv);
      // uninitialized only if aliasee is another global alias
      if (it != globalAddresses.end()) {
        globalAddresses.emplace(ga, it->second);
        return;
      }
    }
  }

  // resolve aliases in all sub-expressions
  for (const auto *op : c->operand_values()) {
    initializeGlobalAlias(cast<Constant>(op), state);
  }

  if (ga) {
    // aliasee is constant expression (or global alias)
    globalAddresses.emplace(ga,
                            evalConstant(ga->getAliasee(), state.roundingMode));
  }
}

void Executor::initializeGlobalAliases(ExecutionState &state) {
  const Module *m = kmodule->module.get();
  for (const GlobalAlias &a : m->aliases()) {
    initializeGlobalAlias(&a, state);
  }
}

void Executor::initializeGlobalObjects(ExecutionState &state) {
  const Module *m = kmodule->module.get();

  for (const GlobalVariable &v : m->globals()) {
    MemoryObject *mo = globalObjects.find(&v)->second;
    ObjectState *os = bindObjectInState(
        state, mo, typeSystemManager->getWrappedType(v.getType()), false,
        nullptr);

    if (v.isDeclaration()) {
      if (isa<ConstantExpr>(mo->getSizeExpr())) {
        ref<ConstantExpr> sizeExpr = dyn_cast<ConstantExpr>(mo->getSizeExpr());
        assert(sizeExpr);
        size_t moSize = sizeExpr->getZExtValue();
        // Program already running -> object already initialized.
        // Read concrete value and write it to our copy.
        void *addr;
        if (v.getName() == "__dso_handle") {
          addr = &__dso_handle; // wtf ?
        } else {
          addr = externalDispatcher->resolveSymbol(v.getName().str());
        }
        if (!addr) {
          klee_error("Unable to load symbol(%.*s) while initializing globals",
                     static_cast<int>(v.getName().size()), v.getName().data());
        } else {
          for (unsigned offset = 0; offset < moSize; offset++) {
            os->write8(offset, static_cast<unsigned char *>(addr)[offset]);
          }
        }
      } else {
        executeMakeSymbolic(
            state, mo, typeSystemManager->getWrappedType(v.getType()),
            SourceBuilder::irreproducible("unsizedGlobal"), false);
      }
    } else if (v.hasInitializer()) {
      initializeGlobalObject(state, os, v.getInitializer(), 0);
      if (v.isConstant()) {
        os->setReadOnly(true);
        // initialise constant memory that may be used with external calls
        state.addressSpace.copyOutConcrete(mo, os, {});
      }
    }
  }
}

bool Executor::branchingPermitted(ExecutionState &state, unsigned N) {
  assert(N);
  if ((MaxMemoryInhibit && atMemoryLimit) || state.forkDisabled ||
      inhibitForking || (MaxForks != ~0u && stats::forks >= MaxForks)) {

    if (MaxMemoryInhibit && atMemoryLimit)
      klee_warning_once(0, "skipping fork (memory cap exceeded)");
    else if (state.forkDisabled)
      klee_warning_once(0, "skipping fork (fork disabled on current path)");
    else if (inhibitForking)
      klee_warning_once(0, "skipping fork (fork disabled globally)");
    else {
      state.targetForest.divideConfidenceBy(N);
      SetOfStates states = {&state};
      for (unsigned i = 0; i < N - 1; i++)
        decreaseConfidenceFromStoppedStates(states, HaltExecution::MaxForks);
      klee_warning_once(0, "skipping fork (max-forks reached)");
    }

    return false;
  }

  return true;
}

void Executor::branch(ExecutionState &state,
                      const std::vector<ref<Expr>> &conditions,
                      std::vector<ExecutionState *> &result,
                      BranchType reason) {
  TimerStatIncrementer timer(stats::forkTime);
  unsigned N = conditions.size();
  assert(N);

  if (!branchingPermitted(state, N)) {
    unsigned next = theRNG.getInt32() % N;
    for (unsigned i = 0; i < N; ++i) {
      if (i == next) {
        result.push_back(&state);
      } else {
        result.push_back(nullptr);
      }
    }
    stats::inhibitedForks += N - 1;
  } else {
    stats::forks += N - 1;

    // XXX do proper balance or keep random?
    result.push_back(&state);
    for (unsigned i = 1; i < N; ++i) {
      ExecutionState *es = result[theRNG.getInt32() % i];
      auto ns = objectManager->branchState(es, reason);
      result.push_back(ns);
    }
  }

  // If necessary redistribute seeds to match conditions, killing
  // states if necessary due to OnlyReplaySeeds (inefficient but
  // simple).

  std::map<ExecutionState *, std::vector<SeedInfo>>::iterator it =
      seedMap->find(&state);
  if (it != seedMap->end()) {
    std::vector<SeedInfo> seeds = it->second;
    seedMap->erase(it);

    // Assume each seed only satisfies one condition (necessarily true
    // when conditions are mutually exclusive and their conjunction is
    // a tautology).
    for (std::vector<SeedInfo>::iterator siit = seeds.begin(),
                                         siie = seeds.end();
         siit != siie; ++siit) {
      unsigned i;
      for (i = 0; i < N; ++i) {
        ref<ConstantExpr> res;
        bool success = solver->getValue(
            state.constraints.cs(), siit->assignment.evaluate(conditions[i]),
            res, state.queryMetaData);
        assert(success && "FIXME: Unhandled solver failure");
        (void)success;
        if (res->isTrue())
          break;
      }

      // If we didn't find a satisfying condition randomly pick one
      // (the seed will be patched).
      if (i == N)
        i = theRNG.getInt32() % N;

      // Extra check in case we're replaying seeds with a max-fork
      if (result[i])
        seedMap->at(result[i]).push_back(*siit);
    }

    if (OnlyReplaySeeds) {
      for (unsigned i = 0; i < N; ++i) {
        if (result[i] && !seedMap->count(result[i])) {
          terminateStateEarlyAlgorithm(*result[i],
                                       "Unseeded path during replay",
                                       StateTerminationType::Replay);
          result[i] = nullptr;
        }
      }
    }
  }

  for (unsigned i = 0; i < N; ++i)
    if (result[i]) {
      result[i]->afterFork = true;
      addConstraint(*result[i], conditions[i]);
    }
}

ref<Expr> Executor::maxStaticPctChecks(ExecutionState &current,
                                       ref<Expr> condition) {
  if (isa<klee::ConstantExpr>(condition))
    return condition;

  if (MaxStaticForkPct == 1. && MaxStaticSolvePct == 1. &&
      MaxStaticCPForkPct == 1. && MaxStaticCPSolvePct == 1.)
    return condition;

  // These checks are performed only after at least MaxStaticPctCheckDelay
  // forks have been performed since execution started
  if (stats::forks < MaxStaticPctCheckDelay)
    return condition;

  StatisticManager &sm = *theStatisticManager;
  CallPathNode *cpn = current.stack.infoStack().back().callPathNode;

  bool reached_max_fork_limit =
      (MaxStaticForkPct < 1. &&
       (sm.getIndexedValue(stats::forks, sm.getIndex()) >
        stats::forks * MaxStaticForkPct));

  bool reached_max_cp_fork_limit = (MaxStaticCPForkPct < 1. && cpn &&
                                    (cpn->statistics.getValue(stats::forks) >
                                     stats::forks * MaxStaticCPForkPct));

  bool reached_max_solver_limit =
      (MaxStaticSolvePct < 1 &&
       (sm.getIndexedValue(stats::solverTime, sm.getIndex()) >
        stats::solverTime * MaxStaticSolvePct));

  bool reached_max_cp_solver_limit =
      (MaxStaticCPForkPct < 1. && cpn &&
       (cpn->statistics.getValue(stats::solverTime) >
        stats::solverTime * MaxStaticCPSolvePct));

  if (reached_max_fork_limit || reached_max_cp_fork_limit ||
      reached_max_solver_limit || reached_max_cp_solver_limit) {
    ref<klee::ConstantExpr> value;
    bool success = solver->getValue(current.constraints.cs(), condition, value,
                                    current.queryMetaData);
    assert(success && "FIXME: Unhandled solver failure");
    (void)success;

    std::string msg("skipping fork and concretizing condition (MaxStatic*Pct "
                    "limit reached) at ");
    llvm::raw_string_ostream os(msg);
    os << current.prevPC->getSourceLocationString();
    klee_warning_once(0, "%s", os.str().c_str());

    addConstraint(current, EqExpr::create(value, condition));
    condition = value;
  }
  return condition;
}

/// returns true if we cannot use CFG reachability checks
/// from instr children to this target
/// to avoid solver calls
bool mustVisitForkBranches(ref<Target> target, KInstruction *instr) {
  // null check after deref error is checked after fork
  // but reachability of this target from instr children
  // will always give false, so we need to force visiting
  // fork branches here
  if (auto reprErrorTarget = dyn_cast<ReproduceErrorTarget>(target)) {
    return reprErrorTarget->isTheSameAsIn(instr) &&
           reprErrorTarget->isThatError(
               ReachWithError::NullCheckAfterDerefException);
  }
  return false;
}

bool Executor::canReachSomeTargetFromBlock(ExecutionState &es, KBlock *block) {
  if (interpreterOpts.Guidance != GuidanceKind::ErrorGuidance)
    return true;
  auto nextInstr = block->getFirstInstruction();
  for (const auto &p : *es.targetForest.getTopLayer()) {
    auto target = p.first;
    if (mustVisitForkBranches(target, es.prevPC))
      return true;
    auto dist = distanceCalculator->getDistance(nextInstr, es.stack.callStack(),
                                                target->getBlock());
    if (dist.result != WeightResult::Miss)
      return true;
  }
  return false;
}

Executor::StatePair Executor::fork(ExecutionState &current, ref<Expr> condition,
                                   KBlock *ifTrueBlock, KBlock *ifFalseBlock,
                                   BranchType reason) {
  bool isInternal = ifTrueBlock == ifFalseBlock;
  std::map<ExecutionState *, std::vector<SeedInfo>>::iterator it =
      seedMap->find(&current);
  bool isSeeding = it != seedMap->end();

  if (!isSeeding)
    condition = maxStaticPctChecks(current, condition);

  time::Span timeout = coreSolverTimeout;
  if (isSeeding)
    timeout *= static_cast<unsigned>(it->second.size());
  solver->setTimeout(timeout);

  bool shouldCheckTrueBlock = true, shouldCheckFalseBlock = true;
  if (!isInternal) {
    shouldCheckTrueBlock = canReachSomeTargetFromBlock(current, ifTrueBlock);
    shouldCheckFalseBlock = canReachSomeTargetFromBlock(current, ifFalseBlock);
  }
  PartialValidity res = PartialValidity::None;
  bool terminateEverything = false, success = true;
  if (!shouldCheckTrueBlock) {
    bool mayBeFalse = false;
    if (shouldCheckFalseBlock) {
      // only solver->check-sat(!condition)
      success = solver->mayBeFalse(current.constraints.cs(), condition,
                                   mayBeFalse, current.queryMetaData);
    }
    if (!success || !mayBeFalse)
      terminateEverything = true;
    else
      res = PartialValidity::MayBeFalse;
  } else if (!shouldCheckFalseBlock) {
    // only solver->check-sat(condition)
    bool mayBeTrue;
    success = solver->mayBeTrue(current.constraints.cs(), condition, mayBeTrue,
                                current.queryMetaData);
    if (!success || !mayBeTrue)
      terminateEverything = true;
    else
      res = PartialValidity::MayBeTrue;
  }
  if (terminateEverything) {
    current.pc = current.prevPC;
    terminateStateEarlyAlgorithm(current, "State missed all it's targets.",
                                 StateTerminationType::MissedAllTargets);
    return StatePair(nullptr, nullptr);
  }
  if (res != PartialValidity::None) {
    success = true;
  } else {
    success = solver->evaluate(current.constraints.cs(), condition, res,
                               current.queryMetaData);
  }
  solver->setTimeout(time::Span());
  if (!success) {
    current.pc = current.prevPC;
    terminateStateOnSolverError(current, "Query timed out (fork).");
    return StatePair(nullptr, nullptr);
  }

  if (!isSeeding) {
    if (replayPath && !isInternal) {
      assert(replayPosition < replayPath->size() &&
             "ran out of branches in replay path mode");
      bool branch = (*replayPath)[replayPosition++];

      if (res == PValidity::MustBeTrue) {
        assert(branch && "hit invalid branch in replay path mode");
      } else if (res == PValidity::MustBeFalse) {
        assert(!branch && "hit invalid branch in replay path mode");
      } else {
        // add constraints
        if (branch) {
          res = PValidity::MustBeTrue;
          addConstraint(current, condition);
        } else {
          res = PValidity::MustBeFalse;
          addConstraint(current, Expr::createIsZero(condition));
        }
      }
    } else if (res == PValidity::TrueOrFalse) {
      assert(!replayKTest && "in replay mode, only one branch can be true.");

      if (!branchingPermitted(current, 2)) {
        TimerStatIncrementer timer(stats::forkTime);
        if (theRNG.getBool()) {
          res = PValidity::MayBeTrue;
        } else {
          res = PValidity::MayBeFalse;
        }
        ++stats::inhibitedForks;
      }
    }
  }

  // Fix branch in only-replay-seed mode, if we don't have both true
  // and false seeds.
  if (isSeeding && (current.forkDisabled || OnlyReplaySeeds) &&
      res == PValidity::TrueOrFalse) {
    bool trueSeed = false, falseSeed = false;
    // Is seed extension still ok here?
    for (std::vector<SeedInfo>::iterator siit = it->second.begin(),
                                         siie = it->second.end();
         siit != siie; ++siit) {
      ref<ConstantExpr> res;
      bool success = solver->getValue(current.constraints.cs(),
                                      siit->assignment.evaluate(condition), res,
                                      current.queryMetaData);
      assert(success && "FIXME: Unhandled solver failure");
      (void)success;
      if (res->isTrue()) {
        trueSeed = true;
      } else {
        falseSeed = true;
      }
      if (trueSeed && falseSeed)
        break;
    }
    if (!(trueSeed && falseSeed)) {
      assert(trueSeed || falseSeed);

      res = trueSeed ? PValidity::MustBeTrue : PValidity::MustBeFalse;
      addConstraint(current,
                    trueSeed ? condition : Expr::createIsZero(condition));
    }
  }

  // XXX - even if the constraint is provable one way or the other we
  // can probably benefit by adding this constraint and allowing it to
  // reduce the other constraints. For example, if we do a binary
  // search on a particular value, and then see a comparison against
  // the value it has been fixed at, we should take this as a nice
  // hint to just use the single constraint instead of all the binary
  // search ones. If that makes sense.
  if (res == PValidity::MustBeTrue || res == PValidity::MayBeTrue) {
    if (!isInternal) {
      if (pathWriter) {
        current.pathOS << "1";
      }
    }

    if (res == PValidity::MayBeTrue) {
      addConstraint(current, condition);
    }

    return StatePair(&current, nullptr);
  } else if (res == PValidity::MustBeFalse || res == PValidity::MayBeFalse) {
    if (!isInternal) {
      if (pathWriter) {
        current.pathOS << "0";
      }
    }

    if (res == PValidity::MayBeFalse) {
      addConstraint(current, Expr::createIsZero(condition));
    }

    return StatePair(nullptr, &current);
  } else {
    // When does PValidity::None happen?
    assert(res == PValidity::TrueOrFalse);
    TimerStatIncrementer timer(stats::forkTime);
    ExecutionState *falseState, *trueState = &current;

    ++stats::forks;

    falseState = objectManager->branchState(trueState, reason);

    if (it != seedMap->end()) {
      std::vector<SeedInfo> seeds = it->second;
      it->second.clear();
      std::vector<SeedInfo> &trueSeeds = seedMap->at(trueState);
      std::vector<SeedInfo> &falseSeeds = seedMap->at(falseState);
      for (std::vector<SeedInfo>::iterator siit = seeds.begin(),
                                           siie = seeds.end();
           siit != siie; ++siit) {
        ref<ConstantExpr> res;
        bool success = solver->getValue(current.constraints.cs(),
                                        siit->assignment.evaluate(condition),
                                        res, current.queryMetaData);
        assert(success && "FIXME: Unhandled solver failure");
        (void)success;
        if (res->isTrue()) {
          trueSeeds.push_back(*siit);
        } else {
          falseSeeds.push_back(*siit);
        }
      }

      bool swapInfo = false;
      if (trueSeeds.empty()) {
        if (&current == trueState)
          swapInfo = true;
        seedMap->erase(trueState);
      }
      if (falseSeeds.empty()) {
        if (&current == falseState)
          swapInfo = true;
        seedMap->erase(falseState);
      }
      if (swapInfo) {
        std::swap(trueState->coveredNew, falseState->coveredNew);
        std::swap(trueState->coveredLines, falseState->coveredLines);
      }
    }

    if (pathWriter) {
      // Need to update the pathOS.id field of falseState, otherwise the same
      // id is used for both falseState and trueState.
      falseState->pathOS = pathWriter->open(current.pathOS);
      if (!isInternal) {
        trueState->pathOS << "1";
        falseState->pathOS << "0";
      }
    }
    if (symPathWriter) {
      falseState->symPathOS = symPathWriter->open(current.symPathOS);
      if (!isInternal) {
        trueState->symPathOS << "1";
        falseState->symPathOS << "0";
      }
    }

    trueState->afterFork = true;
    falseState->afterFork = true;
    addConstraint(*trueState, condition);
    addConstraint(*falseState, Expr::createIsZero(condition));

    // Kinda gross, do we even really still want this option?
    if (MaxDepth && MaxDepth <= trueState->depth) {
      terminateStateEarly(*trueState, "max-depth exceeded.",
                          StateTerminationType::MaxDepth);
      terminateStateEarly(*falseState, "max-depth exceeded.",
                          StateTerminationType::MaxDepth);
      return StatePair(nullptr, nullptr);
    }

    return StatePair(trueState, falseState);
  }
}

Executor::StatePair Executor::forkInternal(ExecutionState &current,
                                           ref<Expr> condition,
                                           BranchType reason) {
  return fork(current, condition, nullptr, nullptr, reason);
}

void Executor::addConstraint(ExecutionState &state, ref<Expr> condition) {
  condition =
      Simplificator::simplifyExpr(state.constraints.cs(), condition).simplified;
  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(condition)) {
    if (!CE->isTrue())
      llvm::report_fatal_error("attempt to add invalid constraint");
    return;
  }

  // Check to see if this constraint violates seeds.
  std::map<ExecutionState *, std::vector<SeedInfo>>::iterator it =
      seedMap->find(&state);
  if (it != seedMap->end()) {
    bool warn = false;
    for (std::vector<SeedInfo>::iterator siit = it->second.begin(),
                                         siie = it->second.end();
         siit != siie; ++siit) {
      bool res;
      solver->setTimeout(coreSolverTimeout);
      bool success = solver->mustBeFalse(state.constraints.cs(),
                                         siit->assignment.evaluate(condition),
                                         res, state.queryMetaData);
      solver->setTimeout(time::Span());
      assert(success && "FIXME: Unhandled solver failure");
      (void)success;
      if (res) {
        siit->patchSeed(state, condition, solver.get());
        warn = true;
      }
    }
    if (warn)
      klee_warning("seeds patched for violating constraint");
  }

  state.addConstraint(condition);

  if (ivcEnabled)
    doImpliedValueConcretization(state, condition,
                                 ConstantExpr::alloc(1, Expr::Bool));
}

const Cell &Executor::eval(const KInstruction *ki, unsigned index,
                           ExecutionState &state, StackFrame &sf,
                           bool isSymbolic) {
  assert(index < ki->inst()->getNumOperands());
  int vnumber = ki->operands[index];

  assert(vnumber != -1 &&
         "Invalid operand to eval(), not a value or constant!");

  // Determine if this is a constant or not.
  if (vnumber < 0) {
    unsigned index = -vnumber - 2;
    return kmodule->constantTable[index];
  } else {
    unsigned index = vnumber;
    ref<Expr> reg = sf.locals->at(index).value;
    if (isSymbolic && reg.isNull()) {
      prepareSymbolicRegister(state, sf, index);
    }
    return sf.locals->at(index);
  }
}

const Cell &Executor::eval(const KInstruction *ki, unsigned index,
                           ExecutionState &state, bool isSymbolic) {
  return eval(ki, index, state, state.stack.valueStack().back(), isSymbolic);
}

void Executor::bindLocal(const KInstruction *target, StackFrame &frame,
                         ref<Expr> value) {
  setDestCell(frame, target, value);
}

void Executor::bindArgument(KFunction *kf, unsigned index, StackFrame &frame,
                            ref<Expr> value) {
  setArgumentCell(frame, kf, index, value);
}

void Executor::bindLocal(const KInstruction *target, ExecutionState &state,
                         ref<Expr> value) {
  setDestCell(state, target, value);
}

void Executor::bindArgument(KFunction *kf, unsigned index,
                            ExecutionState &state, ref<Expr> value) {
  setArgumentCell(state, kf, index, value);
}

ref<Expr> Executor::toUnique(const ExecutionState &state, ref<Expr> e) {
  ref<Expr> result = e;
  solver->setTimeout(coreSolverTimeout);
  solver->tryGetUnique(state.constraints.cs(), e, result, state.queryMetaData);
  solver->setTimeout(time::Span());
  return result;
}

/* Concretize the given expression, and return a possible constant value.
   'reason' is just a documentation string stating the reason for
   concretization. */
ref<klee::ConstantExpr> Executor::toConstant(ExecutionState &state, ref<Expr> e,
                                             const char *reason) {
  e = Simplificator::simplifyExpr(state.constraints.cs(), e).simplified;
  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(e))
    return CE;

  ref<ConstantExpr> value;
  bool success =
      solver->getValue(state.constraints.cs(), e, value, state.queryMetaData);
  assert(success && "FIXME: Unhandled solver failure");
  (void)success;

  std::string str;
  llvm::raw_string_ostream os(str);
  os << "silently concretizing (reason: " << reason << ") expression " << e
     << " to value " << value << " (" << state.pc->getSourceFilepath() << ":"
     << state.pc->getLine() << ")";

  if (AllExternalWarnings)
    klee_warning("%s", os.str().c_str());
  else
    klee_warning_once(reason, "%s", os.str().c_str());

  addConstraint(state, EqExpr::create(e, value));

  return value;
}

ref<klee::ConstantPointerExpr>
Executor::toConstantPointer(ExecutionState &state, ref<PointerExpr> e,
                            const char *reason) {
  e = Simplificator::simplifyExpr(state.constraints.cs(), e).simplified;
  if (ConstantPointerExpr *CE = dyn_cast<ConstantPointerExpr>(e))
    return CE;

  ref<ConstantPointerExpr> pointer;
  bool success =
      solver->getValue(state.constraints.cs(), e, pointer, state.queryMetaData);
  assert(success && "FIXME: Unhandled solver failure");
  (void)success;

  std::string str;
  llvm::raw_string_ostream os(str);
  os << "silently concretizing (reason: " << reason << ") expression " << e
     << " to value " << pointer << " (" << state.pc->getSourceFilepath() << ":"
     << state.pc->getLine() << ")";

  if (AllExternalWarnings)
    klee_warning("%s", os.str().c_str());
  else
    klee_warning_once(reason, "%s", os.str().c_str());

  addConstraint(state, EqExpr::create(e, pointer));

  return pointer;
}

void Executor::executeGetValue(ExecutionState &state, ref<Expr> e,
                               KInstruction *target) {
  e = Simplificator::simplifyExpr(state.constraints.cs(), e).simplified;
  std::map<ExecutionState *, std::vector<SeedInfo>>::iterator it =
      seedMap->find(&state);
  if (it == seedMap->end() || isa<ConstantExpr>(e) ||
      isa<ConstantPointerExpr>(e)) {
    ref<Expr> value;
    e = optimizer.optimizeExpr(e, true);
    bool success =
        solver->getValue(state.constraints.cs(), e, value, state.queryMetaData);
    assert(success && "FIXME: Unhandled solver failure");
    (void)success;
    bindLocal(target, state, value);
  } else {
    std::set<ref<Expr>> values;
    for (std::vector<SeedInfo>::iterator siit = it->second.begin(),
                                         siie = it->second.end();
         siit != siie; ++siit) {
      ref<Expr> cond = siit->assignment.evaluate(e);
      cond = optimizer.optimizeExpr(cond, true);
      ref<ConstantExpr> value;
      bool success = solver->getValue(state.constraints.cs(), cond, value,
                                      state.queryMetaData);
      assert(success && "FIXME: Unhandled solver failure");
      (void)success;
      values.insert(value);
    }

    std::vector<ref<Expr>> conditions;
    for (std::set<ref<Expr>>::iterator vit = values.begin(), vie = values.end();
         vit != vie; ++vit)
      conditions.push_back(EqExpr::create(e, *vit));

    std::vector<ExecutionState *> branches;
    branch(state, conditions, branches, BranchType::GetVal);

    std::vector<ExecutionState *>::iterator bit = branches.begin();
    for (std::set<ref<Expr>>::iterator vit = values.begin(), vie = values.end();
         vit != vie; ++vit) {
      ExecutionState *es = *bit;
      if (es)
        bindLocal(target, *es, *vit);
      ++bit;
    }
  }
}

void Executor::printDebugInstructions(ExecutionState &state) {
  // print nothing if option unset
  if (DebugPrintInstructions.getBits() == 0)
    return;

  // set output stream (stderr/file)
  llvm::raw_ostream *stream = nullptr;
  if (DebugPrintInstructions.isSet(STDERR_ALL) ||
      DebugPrintInstructions.isSet(STDERR_SRC) ||
      DebugPrintInstructions.isSet(STDERR_COMPACT))
    stream = &llvm::errs();
  else
    stream = &debugLogBuffer;

  // print:
  //   [all]     src location:asm line:state ID:instruction
  //   [compact]              asm line:state ID
  //   [src]     src location:asm line:state ID
  if (!DebugPrintInstructions.isSet(STDERR_COMPACT) &&
      !DebugPrintInstructions.isSet(FILE_COMPACT)) {
    (*stream) << "     " << state.pc->getSourceLocationString() << ':';
  }
  {
    auto asmLine = state.pc->getKModule()->getAsmLine(state.pc->inst());
    if (asmLine.has_value()) {
      (*stream) << asmLine.value() << ':';
    }
  }
  (*stream) << state.getID() << ":";
  (*stream) << "[";
  for (auto target : state.targets()) {
    (*stream) << target->toString() << ",";
  }
  (*stream) << "]";

  if (DebugPrintInstructions.isSet(STDERR_ALL) ||
      DebugPrintInstructions.isSet(FILE_ALL))
    (*stream) << ':' << *(state.pc->inst());

  (*stream) << '\n';

  // flush to file
  if (DebugPrintInstructions.isSet(FILE_ALL) ||
      DebugPrintInstructions.isSet(FILE_COMPACT) ||
      DebugPrintInstructions.isSet(FILE_SRC)) {
    debugLogBuffer.flush();
    (*debugInstFile) << debugLogBuffer.str();
    debugBufferString = "";
  }
}

void Executor::stepInstruction(ExecutionState &state) {
  printDebugInstructions(state);
  if (statsTracker)
    statsTracker->stepInstruction(state);

  ++stats::instructions;
  ++state.steppedInstructions;
  if (isa<LoadInst>(state.pc->inst()) || isa<StoreInst>(state.pc->inst())) {
    ++state.steppedMemoryInstructions;
  }
  state.prevPC = state.pc;
  state.constraints.advancePath(state.pc);
  ++state.pc;
  state.afterFork = false;

  if (stats::instructions == MaxInstructions)
    haltExecution = HaltExecution::MaxInstructions;

  if (MaxSteppedInstructions &&
      state.steppedInstructions >= MaxSteppedInstructions)
    haltExecution = HaltExecution::MaxSteppedInstructions;
}

static inline const llvm::fltSemantics *fpWidthToSemantics(unsigned width) {
  switch (width) {
  case Expr::Int32:
    return &llvm::APFloat::IEEEsingle();
  case Expr::Int64:
    return &llvm::APFloat::IEEEdouble();
  case Expr::Fl80:
    return &llvm::APFloat::x87DoubleExtended();
  default:
    return 0;
  }
}

MemoryObject *Executor::serializeLandingpad(ExecutionState &state,
                                            const llvm::LandingPadInst &lpi,
                                            bool &stateTerminated) {
  stateTerminated = false;

  std::vector<unsigned char> serialized;
  std::vector<std::uint64_t> pointerMask;

  for (unsigned current_clause_id = 0; current_clause_id < lpi.getNumClauses();
       ++current_clause_id) {
    llvm::Constant *current_clause = lpi.getClause(current_clause_id);
    if (lpi.isCatch(current_clause_id)) {
      // catch-clause
      serialized.push_back(0);
      pointerMask.push_back(0);

      std::uint64_t ti_addr = 0;

      llvm::BitCastOperator *clause_bitcast =
          dyn_cast<llvm::BitCastOperator>(current_clause);
      if (clause_bitcast) {
        llvm::GlobalValue *clause_type =
            dyn_cast<GlobalValue>(clause_bitcast->getOperand(0));

        // Since global variable may have symbolic address,
        // here we must guarantee that the address of clause is
        // constant (which seems to be true).
        ti_addr = cast<ConstantExpr>(globalAddresses[clause_type]->getValue())
                      ->getZExtValue();
      } else if (current_clause->isNullValue()) {
        ti_addr = 0;
      } else {
        terminateStateOnExecError(
            state, "Internal: Clause is not a bitcast or null (catch-all)");
        stateTerminated = true;
        return nullptr;
      }
      const std::size_t old_size = serialized.size();
      serialized.resize(old_size + 8);
      pointerMask.resize(old_size + 8);
      memcpy(serialized.data() + old_size, &ti_addr, sizeof(ti_addr));
      for (size_t i = 0; i < sizeof(ti_addr); ++i) {
        pointerMask[old_size + i] = ti_addr;
      }
    } else if (lpi.isFilter(current_clause_id)) {
      if (current_clause->isNullValue()) {
        // special handling for a catch-all filter clause, i.e., "[0 x i8*]"
        // for this case we serialize 1 element..
        serialized.push_back(1);
        pointerMask.push_back(0);
        // which is a 64bit-wide 0.
        serialized.resize(serialized.size() + 8, 0);
        pointerMask.resize(pointerMask.size() + 8, 0);
      } else {
        llvm::ConstantArray const *ca =
            cast<llvm::ConstantArray>(current_clause);

        // serialize `num_elements+1` as unsigned char
        unsigned const num_elements = ca->getNumOperands();
        unsigned char serialized_num_elements = 0;

        if (num_elements >=
            std::numeric_limits<decltype(serialized_num_elements)>::max()) {
          terminateStateOnExecError(
              state, "Internal: too many elements in landingpad filter");
          stateTerminated = true;
          return nullptr;
        }

        serialized_num_elements = num_elements;
        serialized.push_back(serialized_num_elements + 1);
        pointerMask.push_back(0);

        // serialize the exception-types occurring in this filter-clause
        for (llvm::Value const *v : ca->operands()) {
          llvm::BitCastOperator const *bitcast =
              dyn_cast<llvm::BitCastOperator>(v);
          if (!bitcast) {
            terminateStateOnExecError(state,
                                      "Internal: expected value inside a "
                                      "filter-clause to be a bitcast");
            stateTerminated = true;
            return nullptr;
          }

          llvm::GlobalValue const *clause_value =
              dyn_cast<GlobalValue>(bitcast->getOperand(0));
          if (!clause_value) {
            terminateStateOnExecError(
                state, "Internal: expected value inside a "
                       "filter-clause bitcast to be a GlobalValue");
            stateTerminated = true;
            return nullptr;
          }

          // We assume again that the clause_value is a
          // constant global.
          std::uint64_t const ti_addr =
              cast<ConstantExpr>(globalAddresses[clause_value]->getValue())
                  ->getZExtValue();

          const std::size_t old_size = serialized.size();
          serialized.resize(old_size + 8);
          pointerMask.resize(old_size + 8);
          memcpy(serialized.data() + old_size, &ti_addr, sizeof(ti_addr));
          for (size_t i = 0; i < sizeof(ti_addr); ++i) {
            pointerMask[old_size + i] = ti_addr;
          }
        }
      }
    }
  }

  MemoryObject *mo =
      allocate(state, Expr::createPointer(serialized.size()), true, false,
               nullptr, 1, typeSystemManager->getUnknownType());
  ObjectState *os =
      bindObjectInState(state, mo, typeSystemManager->getUnknownType(), false);
  for (unsigned i = 0; i < serialized.size(); i++) {
    ref<ConstantExpr> sec = ConstantExpr::create(serialized[i], Expr::Int8);
    if (pointerMask.at(i)) {
      ref<ConstantExpr> seg = Expr::createPointer(pointerMask.at(i));
      os->write(i, PointerExpr::create(seg, sec));
    } else {
      os->write(i, sec);
    }
  }

  return mo;
}

void Executor::unwindToNextLandingpad(ExecutionState &state) {
  UnwindingInformation *ui = state.unwindingInformation.get();
  assert(ui && "unwinding without unwinding information");

  std::size_t startIndex = 0;
  std::size_t lowestStackIndex = 0;
  bool popFrames = false;

  if (auto *sui = dyn_cast<SearchPhaseUnwindingInformation>(ui)) {
    startIndex = sui->unwindingProgress;
    lowestStackIndex = 0;
    popFrames = false;
  } else if (auto *cui = dyn_cast<CleanupPhaseUnwindingInformation>(ui)) {
    startIndex = state.stack.callStack().size() - 1;
    lowestStackIndex = cui->catchingStackIndex;
    popFrames = true;
  } else {
    assert(false && "invalid UnwindingInformation subclass");
  }

  for (std::size_t i = startIndex; i > lowestStackIndex; i--) {
    auto const &sf = state.stack.callStack().at(i);

    Instruction *inst = sf.caller ? sf.caller->inst() : nullptr;

    if (popFrames) {
      state.popFrame();
    }

    if (InvokeInst *invoke = dyn_cast<InvokeInst>(inst)) {
      // we found the next invoke instruction in the call stack, handle it
      // depending on the current phase.
      if (auto *sui = dyn_cast<SearchPhaseUnwindingInformation>(ui)) {
        // in the search phase, run personality function to check if this
        // landingpad catches the exception

        LandingPadInst *lpi = invoke->getUnwindDest()->getLandingPadInst();
        assert(lpi && "unwind target of an invoke instruction did not lead to "
                      "a landingpad");

        // check if this is a pure cleanup landingpad first
        if (lpi->isCleanup() && lpi->getNumClauses() == 0) {
          // pure cleanup lpi, this can't be a handler, so skip it
          continue;
        }

        bool stateTerminated = false;
        MemoryObject *clauses_mo =
            serializeLandingpad(state, *lpi, stateTerminated);
        assert((stateTerminated != bool(clauses_mo)) &&
               "illegal serializeLandingpad result");

        if (stateTerminated) {
          return;
        }

        assert(sui->serializedLandingpad == nullptr &&
               "serializedLandingpad should be reset");
        sui->serializedLandingpad = clauses_mo;

        llvm::Function *personality_fn =
            kmodule->module->getFunction("_klee_eh_cxx_personality");
        KFunction *kf = kmodule->functionMap[personality_fn];

        state.pushFrame(state.prevPC, kf);
        state.pc = kf->instructions;
        state.increaseLevel();
        bindArgument(kf, 0, state, sui->exceptionObject);
        bindArgument(kf, 1, state, clauses_mo->getSizeExpr());
        bindArgument(kf, 2, state,
                     PointerExpr::create(clauses_mo->getBaseExpr(),
                                         clauses_mo->getBaseExpr()));

        if (statsTracker) {
          statsTracker->framePushed(
              state, &state.stack.infoStack()[state.stack.size() - 2]);
        }

        // make sure we remember our search progress afterwards
        sui->unwindingProgress = i - 1;
      } else {
        // in the cleanup phase, redirect control flow
        transferToBasicBlock(invoke->getUnwindDest(), invoke->getParent(),
                             state);
      }

      // we are done, stop search/unwinding here
      return;
    }
  }

  // no further invoke instruction/landingpad found
  if (isa<SearchPhaseUnwindingInformation>(ui)) {
    // in phase 1, simply stop unwinding. this will return
    // control flow back to _Unwind_RaiseException, which will
    // return the correct error.

    // clean up unwinding state
    state.unwindingInformation.reset();
  } else {
    // in phase 2, this represent a situation that should
    // not happen, as we only progressed to phase 2 because
    // we found a handler in phase 1.
    // therefore terminate the state.
    terminateStateOnExecError(state,
                              "Missing landingpad in phase 2 of unwinding");
  }
}

ref<klee::ConstantExpr> Executor::getEhTypeidFor(ref<Expr> type_info) {
  // FIXME: Handling getEhTypeidFor is non-deterministic and depends on the
  //        order states have been processed and executed.
  auto eh_type_iterator =
      std::find(std::begin(eh_typeids), std::end(eh_typeids), type_info);
  if (eh_type_iterator == std::end(eh_typeids)) {
    eh_typeids.push_back(type_info);
    eh_type_iterator = std::prev(std::end(eh_typeids));
  }
  // +1 because typeids must always be positive, so they can be distinguished
  // from 'no landingpad clause matched' which has value 0
  auto res = ConstantExpr::create(eh_type_iterator - std::begin(eh_typeids) + 1,
                                  Expr::Int32);
  return res;
}

void Executor::executeCall(ExecutionState &state, KInstruction *ki, Function *f,
                           std::vector<ref<Expr>> &arguments) {
  Instruction *i = ki->inst();
  if (isa_and_nonnull<DbgInfoIntrinsic>(i))
    return;
  if (f && f->isDeclaration()) {
#ifndef ENABLE_FP
    Intrinsic::ID id = f->getIntrinsicID();
    if (supportedFPIntrinsics.find(id) != supportedFPIntrinsics.end()) {
      klee_warning("unimplemented intrinsic: %s", f->getName().data());
      klee_message(
          "You may enable this intrinsic by passing the following options"
          " to cmake:\n"
          "\"-DENABLE_FLOATING_POINT=ON\"\n");
      terminateStateOnExecError(state, "unimplemented intrinsic",
                                StateTerminationType::Execution);
      return;
    }
    if (modelledFPIntrinsics.find(id) != modelledFPIntrinsics.end()) {
      klee_warning("unimplemented intrinsic: %s", f->getName().data());
      klee_message(
          "You may enable this intrinsic by passing the following options"
          " to cmake:\n"
          "\"-DENABLE_FLOATING_POINT=ON\"\n"
          "\"-DENABLE_FP_RUNTIME=ON\"\n");
      terminateStateOnExecError(state, "unimplemented intrinsic",
                                StateTerminationType::Execution);
      return;
    }
#endif
    switch (f->getIntrinsicID()) {
    case Intrinsic::not_intrinsic: {
      // state may be destroyed by this call, cannot touch
      callExternalFunction(state, ki, kmodule->functionMap[f], arguments);
      break;
    }
    case Intrinsic::fabs: {
#ifndef ENABLE_FP
      ref<ConstantExpr> arg = toConstant(state, arguments[0], "floating point");
      if (!fpWidthToSemantics(arg->getWidth()))
        return terminateStateOnExecError(
            state, "Unsupported intrinsic llvm.fabs call");

      llvm::APFloat Res(*fpWidthToSemantics(arg->getWidth()),
                        arg->getAPValue());
      Res = llvm::abs(Res);

      bindLocal(ki, state, ConstantExpr::alloc(Res.bitcastToAPInt()));
#else
      ref<Expr> op = eval(ki, 1, state).value;
      ref<Expr> result = FAbsExpr::create(op);
      bindLocal(ki, state, result);
#endif
      break;
    }
#ifdef ENABLE_FP
    case Intrinsic::sqrt: {
      ref<Expr> op = eval(ki, 1, state).value;
      ref<Expr> result = FSqrtExpr::create(op, state.roundingMode);
      bindLocal(ki, state, result);
      break;
    }

    case Intrinsic::maxnum:
    case Intrinsic::minnum: {
      ref<Expr> op1 = eval(ki, 1, state).value;
      ref<Expr> op2 = eval(ki, 2, state).value;
      assert(op1->getWidth() == op2->getWidth() && "type mismatch");
      ref<Expr> result;
      if (f->getIntrinsicID() == Intrinsic::maxnum) {
        result = FMaxExpr::create(op1, op2, state.roundingMode);
      } else {
        result = FMinExpr::create(op1, op2, state.roundingMode);
      }
      bindLocal(ki, state, result);
      break;
    }
    case Intrinsic::trunc: {
      FPTruncInst *fi = cast<FPTruncInst>(i);
      Expr::Width resultType = getWidthForLLVMType(fi->getType());
      ref<Expr> arg = eval(ki, 0, state).value;
      if (!fpWidthToSemantics(arg->getWidth()) ||
          !fpWidthToSemantics(resultType))
        return terminateStateOnExecError(state,
                                         "Unsupported FPTrunc operation");
      if (arg->getWidth() <= resultType)
        return terminateStateOnExecError(state, "Invalid FPTrunc");
      ref<Expr> result =
          FPTruncExpr::create(arg, resultType, state.roundingMode);
      bindLocal(ki, state, result);
      break;
    }
    case Intrinsic::rint: {
      ref<Expr> arg = eval(ki, 0, state).value;
      ref<Expr> result = FRintExpr::create(arg, state.roundingMode);
      bindLocal(ki, state, result);
      break;
    }
#endif

    case Intrinsic::fma:
    case Intrinsic::fmuladd: {
#ifndef ENABLE_FP
      // Both fma and fmuladd support float, double and fp80.  Note, that fp80
      // is not mentioned in the documentation of fmuladd, nevertheless, it is
      // still supported.  For details see
      // https://github.com/klee/klee/pull/1507/files#r894993332

      if (isa<VectorType>(i->getOperand(0)->getType()))
        return terminateStateOnExecError(
            state, f->getName() + " with vectors is not supported");

      ref<ConstantExpr> op1 =
          toConstant(state, eval(ki, 1, state).value, "floating point");
      ref<ConstantExpr> op2 =
          toConstant(state, eval(ki, 2, state).value, "floating point");
      ref<ConstantExpr> op3 =
          toConstant(state, eval(ki, 3, state).value, "floating point");

      if (!fpWidthToSemantics(op1->getWidth()) ||
          !fpWidthToSemantics(op2->getWidth()) ||
          !fpWidthToSemantics(op3->getWidth()))
        return terminateStateOnExecError(state, "Unsupported " + f->getName() +
                                                    " call");

      // (op1 * op2) + op3
      APFloat Res(*fpWidthToSemantics(op1->getWidth()), op1->getAPValue());
      Res.fusedMultiplyAdd(
          APFloat(*fpWidthToSemantics(op2->getWidth()), op2->getAPValue()),
          APFloat(*fpWidthToSemantics(op3->getWidth()), op3->getAPValue()),
          APFloat::rmNearestTiesToEven);

      bindLocal(ki, state, ConstantExpr::alloc(Res.bitcastToAPInt()));
      break;
#else
      ref<Expr> op1 = eval(ki, 1, state).value;
      ref<Expr> op2 = eval(ki, 2, state).value;
      ref<Expr> op3 = eval(ki, 3, state).value;
      assert(op1->getWidth() == op2->getWidth() &&
             op2->getWidth() == op3->getWidth() && "type mismatch");
      ref<Expr> result =
          FAddExpr::create(FMulExpr::create(op1, op2, state.roundingMode), op3,
                           state.roundingMode);
      bindLocal(ki, state, result);
      break;
#endif
    }

#if LLVM_VERSION_CODE >= LLVM_VERSION(12, 0)
    case Intrinsic::abs: {
      if (isa<VectorType>(i->getOperand(0)->getType()))
        return terminateStateOnExecError(
            state, "llvm.abs with vectors is not supported");

      ref<Expr> op = eval(ki, 1, state).value;
      ref<Expr> poison = eval(ki, 2, state).value;

      assert(poison->getWidth() == 1 && "Second argument is not an i1");
      unsigned bw = op->getWidth();

      uint64_t moneVal = APInt(bw, -1, true).getZExtValue();
      uint64_t sminVal = APInt::getSignedMinValue(bw).getZExtValue();

      ref<ConstantExpr> zero = ConstantExpr::create(0, bw);
      ref<ConstantExpr> mone = ConstantExpr::create(moneVal, bw);
      ref<ConstantExpr> smin = ConstantExpr::create(sminVal, bw);

      if (poison->isTrue()) {
        ref<Expr> issmin = EqExpr::create(op, smin);
        if (issmin->isTrue())
          return terminateStateOnExecError(
              state, "llvm.abs called with poison and INT_MIN");
      }

      // conditions to flip the sign: INT_MIN < op < 0
      ref<Expr> negative = SltExpr::create(op, zero);
      ref<Expr> notsmin = NeExpr::create(op, smin);
      ref<Expr> cond = AndExpr::create(negative, notsmin);

      // flip and select the result
      ref<Expr> flip = MulExpr::create(op, mone);
      ref<Expr> result = SelectExpr::create(cond, flip, op);

      bindLocal(ki, state, result);
      break;
    }

    case Intrinsic::smax:
    case Intrinsic::smin:
    case Intrinsic::umax:
    case Intrinsic::umin: {
      if (isa<VectorType>(i->getOperand(0)->getType()) ||
          isa<VectorType>(i->getOperand(1)->getType()))
        return terminateStateOnExecError(
            state, "llvm.{s,u}{max,min} with vectors is not supported");

      ref<Expr> op1 = eval(ki, 1, state).value;
      ref<Expr> op2 = eval(ki, 2, state).value;

      ref<Expr> cond = nullptr;
      if (f->getIntrinsicID() == Intrinsic::smax)
        cond = SgtExpr::create(op1, op2);
      else if (f->getIntrinsicID() == Intrinsic::smin)
        cond = SltExpr::create(op1, op2);
      else if (f->getIntrinsicID() == Intrinsic::umax)
        cond = UgtExpr::create(op1, op2);
      else // (f->getIntrinsicID() == Intrinsic::umin)
        cond = UltExpr::create(op1, op2);

      ref<Expr> result = SelectExpr::create(cond, op1, op2);
      bindLocal(ki, state, result);
      break;
    }
#endif

    case Intrinsic::fshr:
    case Intrinsic::fshl: {
      ref<Expr> op1 = eval(ki, 1, state).value;
      ref<Expr> op2 = eval(ki, 2, state).value;
      ref<Expr> op3 = eval(ki, 3, state).value;
      unsigned w = op1->getWidth();
      assert(w == op2->getWidth() && "type mismatch");
      assert(w == op3->getWidth() && "type mismatch");
      ref<Expr> c = ConcatExpr::create(op1, op2);
      // op3 = zeroExtend(op3 % w)
      op3 = URemExpr::create(op3, ConstantExpr::create(w, w));
      op3 = ZExtExpr::create(op3, w + w);
      if (f->getIntrinsicID() == Intrinsic::fshl) {
        // shift left and take top half
        ref<Expr> s = ShlExpr::create(c, op3);
        bindLocal(ki, state, ExtractExpr::create(s, w, w));
      } else {
        // shift right and take bottom half
        // note that LShr and AShr will have same behaviour
        ref<Expr> s = LShrExpr::create(c, op3);
        bindLocal(ki, state, ExtractExpr::create(s, 0, w));
      }
      break;
    }

    // va_arg is handled by caller and intrinsic lowering, see comment for
    // ExecutionState::varargs
    case Intrinsic::vastart: {
      StackFrame &sf = state.stack.valueStack().back();

      // varargs can be zero if no varargs were provided
      if (!sf.varargs)
        return;

      // FIXME: This is really specific to the architecture, not the pointer
      // size. This happens to work for x86-32 and x86-64, however.
      Expr::Width WordSize = Context::get().getPointerWidth();
      if (WordSize == Expr::Int32) {
        executeMemoryOperation(state, true, typeSystemManager->getUnknownType(),
                               makePointer(arguments[0]),
                               PointerExpr::create(sf.varargs->getBaseExpr(),
                                                   sf.varargs->getBaseExpr()),
                               0);
      } else {
        assert(WordSize == Expr::Int64 && "Unknown word size!");

        // x86-64 has quite complicated calling convention. However,
        // instead of implementing it, we can do a simple hack: just
        // make a function believe that all varargs are on stack.
        executeMemoryOperation(state, true, typeSystemManager->getUnknownType(),
                               makePointer(arguments[0]),
                               ConstantExpr::create(48, 32),
                               0); // gp_offset
        executeMemoryOperation(state, true, typeSystemManager->getUnknownType(),
                               AddExpr::create(makePointer(arguments[0]),
                                               ConstantExpr::create(4, 64)),
                               ConstantExpr::create(304, 32), 0); // fp_offset
        executeMemoryOperation(state, true, typeSystemManager->getUnknownType(),
                               AddExpr::create(makePointer(arguments[0]),
                                               ConstantExpr::create(8, 64)),
                               PointerExpr::create(sf.varargs->getBaseExpr(),
                                                   sf.varargs->getBaseExpr()),
                               0); // overflow_arg_area
        executeMemoryOperation(state, true, typeSystemManager->getUnknownType(),
                               AddExpr::create(makePointer(arguments[0]),
                                               ConstantExpr::create(16, 64)),
                               ConstantExpr::create(0, 64), 0); // reg_save_area
      }
      break;
    }

#ifdef SUPPORT_KLEE_EH_CXX
    case Intrinsic::eh_typeid_for: {
      bindLocal(ki, state, getEhTypeidFor(arguments.at(0)));
      break;
    }
#endif

    case Intrinsic::vaend:
      // va_end is a noop for the interpreter.
      //
      // FIXME: We should validate that the target didn't do something bad
      // with va_end, however (like call it twice).
      break;

    case Intrinsic::vacopy:
      // va_copy should have been lowered.
      //
      // FIXME: It would be nice to check for errors in the usage of this as
      // well.
    default:
      klee_warning("unimplemented intrinsic: %s", f->getName().data());
      terminateStateOnExecError(state, "unimplemented intrinsic");
      return;
    }

    if (InvokeInst *ii = dyn_cast<InvokeInst>(i)) {
      transferToBasicBlock(ii->getNormalDest(), i->getParent(), state);
    }
  } else {
    // Check if maximum stack size was reached.
    // We currently only count the number of stack frames
    if (RuntimeMaxStackFrames && state.stack.size() > RuntimeMaxStackFrames) {
      terminateStateEarly(state, "Maximum stack size reached.",
                          StateTerminationType::OutOfStackMemory);
      klee_warning("Maximum stack size reached.");
      return;
    }

    // FIXME: I'm not really happy about this reliance on prevPC but it is ok,
    // I guess. This just done to avoid having to pass KInstIterator
    // everywhere instead of the actual instruction, since we can't make a
    // KInstIterator from just an instruction (unlike LLVM).
    KFunction *kf = kmodule->functionMap[f];

    if (kmodule->inMainModule(*f) && kmodule->inMainModule(*i)) {
      state.eventsRecorder.record(new CallEvent(locationOf(state), kf));
    }

    state.pushFrame(state.prevPC, kf);
    transferToBasicBlock(&*kf->function()->begin(), state.getPrevPCBlock(),
                         state);

    if (statsTracker)
      statsTracker->framePushed(
          state, &state.stack.infoStack()[state.stack.size() - 2]);

    // TODO: support zeroext, signext, sret attributes

    unsigned callingArgs = arguments.size();
    unsigned funcArgs = f->arg_size();
    if (!f->isVarArg()) {
      if (callingArgs > funcArgs) {
        klee_warning_once(f, "calling %s with extra arguments.",
                          f->getName().data());
      } else if (callingArgs < funcArgs) {
        terminateStateOnUserError(state,
                                  "calling function with too few arguments");
        return;
      }
    } else {
      if (callingArgs < funcArgs) {
        terminateStateOnUserError(state,
                                  "calling function with too few arguments");
        return;
      }

      // Only x86-32 and x86-64 are supported
      Expr::Width WordSize = Context::get().getPointerWidth();
      assert(((WordSize == Expr::Int32) || (WordSize == Expr::Int64)) &&
             "Unknown word size!");

      uint64_t size = 0; // total size of variadic arguments
      bool requires16ByteAlignment = false;

      uint64_t offsets[callingArgs]; // offsets of variadic arguments
      uint64_t argWidth;             // width of current variadic argument

      const CallBase &cb = cast<CallBase>(*i);
      for (unsigned k = funcArgs; k < callingArgs; k++) {
        if (cb.isByValArgument(k)) {
          Type *t = cb.getParamByValType(k);
          argWidth = kmodule->targetData->getTypeSizeInBits(t);
        } else {
          argWidth = arguments[k]->getWidth();
        }

#if LLVM_VERSION_CODE >= LLVM_VERSION(11, 0)
        MaybeAlign ma = cb.getParamAlign(k);
        unsigned alignment = ma ? ma->value() : 0;
#else
        unsigned alignment = cb.getParamAlignment(k);
#endif

        if (WordSize == Expr::Int32 && !alignment)
          alignment = 4;
        else {
          // AMD64-ABI 3.5.7p5: Step 7. Align l->overflow_arg_area upwards to
          // a 16 byte boundary if alignment needed by type exceeds 8 byte
          // boundary.
          if (!alignment && argWidth > Expr::Int64) {
            alignment = 16;
            requires16ByteAlignment = true;
          }

          if (!alignment)
            alignment = 8;
        }

        size = llvm::alignTo(size, alignment);
        offsets[k] = size;

        if (WordSize == Expr::Int32)
          size += Expr::getMinBytesForWidth(argWidth);
        else {
          // AMD64-ABI 3.5.7p5: Step 9. Set l->overflow_arg_area to:
          // l->overflow_arg_area + sizeof(type)
          // Step 10. Align l->overflow_arg_area upwards to an 8 byte
          // boundary.
          size += llvm::alignTo(argWidth, WordSize) / 8;
        }
      }

      StackFrame &sf = state.stack.valueStack().back();
      MemoryObject *mo = sf.varargs = memory->allocate(
          Expr::createPointer(size), true, false, false, locationOf(state),
          (requires16ByteAlignment ? 16 : 8),
          typeSystemManager->getUnknownType());
      if (!mo && size) {
        terminateStateOnExecError(state, "out of memory (varargs)");
        return;
      }

      if (mo) {
        if ((WordSize == Expr::Int64) &&
            (cast<ConstantExpr>(mo->getBaseExpr())->getZExtValue() & 15) &&
            requires16ByteAlignment) {
          // Both 64bit Linux/Glibc and 64bit MacOSX should align to 16 bytes.
          klee_warning_once(
              0, "While allocating varargs: malloc did not align to 16 bytes.");
        }

        ObjectState *os = bindObjectInState(
            state, mo, typeSystemManager->getUnknownType(), true);

        llvm::Function::arg_iterator ati = std::next(f->arg_begin(), funcArgs);
        llvm::Type *argType = (ati == f->arg_end()) ? nullptr : ati->getType();

        for (unsigned k = funcArgs; k < callingArgs; k++) {
          if (!cb.isByValArgument(k)) {
            os->write(offsets[k], arguments[k]);
          } else {
            ref<PointerExpr> pointer = makePointer(arguments[k]);
            ConstantPointerExpr *CP = dyn_cast<ConstantPointerExpr>(pointer);
            assert(CP); // byval argument needs to be a concrete pointer

            ObjectPair idObject;
            state.addressSpace.resolveOne(
                CP, typeSystemManager->getWrappedType(argType), idObject);
            const ObjectState *osarg = idObject.second;
            assert(osarg);
            ref<ConstantExpr> sizeExpr =
                dyn_cast<ConstantExpr>(idObject.first->getSizeExpr());
            assert(sizeExpr);
            size_t moSize = sizeExpr->getZExtValue();
            for (unsigned i = 0; i < moSize; i++)
              os->write(offsets[k] + i, osarg->read8(i));
          }
          if (ati != f->arg_end()) {
            ++ati;
          }
        }
      }
    }

    unsigned numFormals = f->arg_size();
    for (unsigned k = 0; k < numFormals; k++)
      bindArgument(kf, k, state, arguments[k]);
  }

  if (!state.multiplexKF && !f->isIntrinsic() && !f->isDeclaration() &&
      kmodule->mainModuleFunctions.count(std::string(f->getName()))) {
    state.multiplexKF = ki->getKModule()->functionMap.at(f);
    multiplexReached++;
  }
}

void Executor::increaseProgressVelocity(ExecutionState &state, KBlock *block) {
  if (state.visited(block)) {
    if (state.progressVelocity >= 0) {
      state.progressVelocity -= state.progressAcceleration;
      state.progressAcceleration *= 2;
    }
  } else {
    if (state.progressVelocity >= 0) {
      state.progressVelocity += 1;
    } else {
      state.progressVelocity = 0;
      state.progressAcceleration = 1;
    }
  }
}

void Executor::transferToBasicBlock(KBlock *kdst, BasicBlock *src,
                                    ExecutionState &state) {
  // Note that in general phi nodes can reuse phi values from the same
  // block but the incoming value is the eval() result *before* the
  // execution of any phi nodes. this is pathological and doesn't
  // really seem to occur, but just in case we run the PhiCleanerPass
  // which makes sure this cannot happen and so it is safe to just
  // eval things in order. The PhiCleanerPass also makes sure that all
  // incoming blocks have the same order for each PHINode so we only
  // have to compute the index once.
  //
  // With that done we simply set an index in the state so that PHI
  // instructions know which argument to eval, set the pc, and continue.

  // XXX this lookup has to go ?
  state.pc = kdst->instructions;
  state.increaseLevel();
  if (state.pc->inst()->getOpcode() == Instruction::PHI) {
    PHINode *first = static_cast<PHINode *>(state.pc->inst());
    state.incomingBBIndex = first->getBasicBlockIndex(src);
  }
  increaseProgressVelocity(state, kdst);
}

void Executor::transferToBasicBlock(BasicBlock *dst, BasicBlock *src,
                                    ExecutionState &state) {
  KFunction *kf = state.stack.callStack().back().kf;
  auto kdst = kf->blockMap[dst];
  transferToBasicBlock(kdst, src, state);
}

void Executor::checkNullCheckAfterDeref(ref<Expr> cond, ExecutionState &state) {
  std::vector<ref<Expr>> conditions;
  Expr::splitAnds(cond, conditions);
  ref<EqExpr> eqPointerCheck = nullptr;
  for (auto &primitiveCond : conditions) {
    if (isa<EqExpr>(primitiveCond) &&
        cast<EqExpr>(primitiveCond)->left->getWidth() ==
            Context::get().getPointerWidth()) {
      eqPointerCheck = cast<EqExpr>(primitiveCond);
    }
    if (isa<EqExpr>(Expr::createIsZero(primitiveCond)) &&
        cast<EqExpr>(Expr::createIsZero(primitiveCond))->left->getWidth() ==
            Context::get().getPointerWidth()) {
      eqPointerCheck = cast<EqExpr>(Expr::createIsZero(primitiveCond));
    }
    if (eqPointerCheck && eqPointerCheck->left->isZero()) {
      llvm::errs();
    }
    if (eqPointerCheck && eqPointerCheck->left->isZero() &&
        state.resolvedPointers.count(
            makePointer(eqPointerCheck->right)->getBase())) {
      break;
    }
  }
  if (eqPointerCheck && eqPointerCheck->left->isZero() &&
      state.resolvedPointers.count(
          makePointer(eqPointerCheck->right)->getBase())) {
    reportStateOnTargetError(state,
                             ReachWithError::NullCheckAfterDerefException);
  }
}

void Executor::executeInstruction(ExecutionState &state, KInstruction *ki) {
  Instruction *i = ki->inst();

  if (guidanceKind == GuidanceKind::ErrorGuidance) {
    for (auto kvp : state.targetForest) {
      auto target = kvp.first;
      if (target->shouldFailOnThisTarget() &&
          cast<ReproduceErrorTarget>(target)->isThatError(
              ReachWithError::Reachable) &&
          target->getBlock() == ki->parent &&
          cast<ReproduceErrorTarget>(target)->isTheSameAsIn(ki)) {
        terminateStateOnTargetError(state, ReachWithError::Reachable);
        return;
      }
    }
  }

  switch (i->getOpcode()) {
    // Control flow
  case Instruction::Ret: {
    ReturnInst *ri = cast<ReturnInst>(i);
    KInstIterator kcaller = state.stack.callStack().back().caller;
    Instruction *caller = kcaller ? kcaller->inst() : nullptr;
    bool isVoidReturn = (ri->getNumOperands() == 0);
    ref<Expr> result = ConstantExpr::alloc(0, Expr::Bool);

    if (!isVoidReturn) {
      result = eval(ki, 0, state).value;
    }

    if (state.stack.size() <= 1) {
      assert(!caller && "caller set on initial stack frame");
      state.pc = state.prevPC;
      state.increaseLevel();
      terminateStateOnExit(state);
    } else {
      if (kmodule->inMainModule(*i)) {
        KFunction *callerFunction = kcaller->parent->parent;

        if (kmodule->WithPOSIXRuntime() &&
            callerFunction->getName() == "__klee_posix_wrapped_main") {
          callerFunction = state.stack.callStack().front().kf;
        }

        state.eventsRecorder.record(
            new ReturnEvent(locationOf(state), callerFunction));
      }

      state.popFrame();

      if (InvokeInst *ii = dyn_cast<InvokeInst>(caller)) {
        transferToBasicBlock(ii->getNormalDest(), caller->getParent(), state);
      } else {
        state.pc = kcaller;
        state.increaseLevel();
        ++state.pc;
      }

#ifdef SUPPORT_KLEE_EH_CXX
      if (ri->getFunction()->getName() == "_klee_eh_cxx_personality") {
        assert(dyn_cast<ConstantExpr>(result) &&
               "result from personality fn must be a concrete value");

        auto *sui = dyn_cast_or_null<SearchPhaseUnwindingInformation>(
            state.unwindingInformation.get());
        assert(sui && "return from personality function outside of "
                      "search phase unwinding");

        // unbind the MO we used to pass the serialized landingpad
        state.removePointerResolutions(sui->serializedLandingpad);
        state.addressSpace.unbindObject(sui->serializedLandingpad);
        sui->serializedLandingpad = nullptr;

        if (result->isZero()) {
          // this lpi doesn't handle the exception, continue the search
          unwindToNextLandingpad(state);
        } else {
          // a clause (or a catch-all clause or filter clause) matches:
          // remember the stack index and switch to cleanup phase
          state.unwindingInformation =
              std::make_unique<CleanupPhaseUnwindingInformation>(
                  sui->exceptionObject, cast<ConstantExpr>(result),
                  sui->unwindingProgress);
          // this pointer is now invalidated
          sui = nullptr;
          // continue the unwinding process (which will now start with the
          // cleanup phase)
          unwindToNextLandingpad(state);
        }

        // never return normally from the personality fn
        break;
      }
#endif // SUPPORT_KLEE_EH_CXX

      if (!isVoidReturn) {
        Type *t = caller->getType();
        if (t != Type::getVoidTy(i->getContext())) {
          // may need to do coercion due to bitcasts
          Expr::Width from = result->getWidth();
          Expr::Width to = getWidthForLLVMType(t);

          if (X86FPAsX87FP80 && t->isFloatingPointTy() &&
              Context::get().getPointerWidth() == 32) {
            result = FPToX87FP80Ext(result);
          } else if (from != to) {
            const CallBase &cb = cast<CallBase>(*caller);

            // XXX need to check other param attrs ?
            bool isSExt = cb.hasRetAttr(llvm::Attribute::SExt);
            if (isSExt) {
              result = SExtExpr::create(result, to);
            } else {
              result = ZExtExpr::create(result, to);
            }
          }

          bindLocal(kcaller, state, result);
        }
      } else {
        // We check that the return value has no users instead of
        // checking the type, since C defaults to returning int for
        // undeclared functions.
        if (kmodule->WithPOSIXRuntime() &&
            cast<KCallBlock>(kcaller->parent)->getKFunction() &&
            cast<KCallBlock>(kcaller->parent)->getKFunction()->getName() ==
                "__klee_posix_wrapped_main") {
          bindLocal(kcaller, state, ConstantExpr::alloc(0, Expr::Int32));
        } else if (!caller->use_empty()) {
          terminateStateOnExecError(
              state, "return void when caller expected a result");
        }
      }
    }
    break;
  }
  case Instruction::Br: {
    ref<CodeLocation> brLocation = locationOf(state);

    BranchInst *bi = cast<BranchInst>(i);
    if (bi->isUnconditional()) {
      // `getMetadata` is used instead of `hasMetadata`
      // because of backward compatibility with LLVM 9.
      if (bi->getMetadata("md.ret")) {
        state.stack.forceReturnLocation(locationOf(state));
      }

      transferToBasicBlock(bi->getSuccessor(0), bi->getParent(), state);
    } else {
      // FIXME: Find a way that we don't have this hidden dependency.
      assert(bi->getCondition() == bi->getOperand(0) && "Wrong operand index!");
      ref<Expr> cond = eval(ki, 0, state).value;

      cond = optimizer.optimizeExpr(cond, false);

      KFunction *kf = state.stack.callStack().back().kf;
      auto ifTrueBlock = kf->blockMap[bi->getSuccessor(0)];
      auto ifFalseBlock = kf->blockMap[bi->getSuccessor(1)];
      Executor::StatePair branches =
          fork(state, cond, ifTrueBlock, ifFalseBlock, BranchType::Conditional);

      if (branches.first && branches.second) {
        maxNewStateStackSize =
            std::max(maxNewStateStackSize,
                     branches.first->stack.stackRegisterSize() * 8);
      }

      // NOTE: There is a hidden dependency here, markBranchVisited
      // requires that we still be in the context of the branch
      // instruction (it reuses its statistic id). Should be cleaned
      // up with convenient instruction specific data.
      if (statsTracker)
        statsTracker->markBranchVisited(branches.first, branches.second);

      if (branches.first) {
        transferToBasicBlock(bi->getSuccessor(0), bi->getParent(),
                             *branches.first);
        if (kmodule->inMainModule(*i)) {
          BrEvent *brEvent = new BrEvent(brLocation);
          branches.first->eventsRecorder.record(&brEvent->withBranch(true));
        }
      }
      if (branches.second) {
        transferToBasicBlock(bi->getSuccessor(1), bi->getParent(),
                             *branches.second);
        if (kmodule->inMainModule(*i)) {
          BrEvent *brEvent = new BrEvent(brLocation);
          branches.second->eventsRecorder.record(&brEvent->withBranch(false));
        }
      }
      if (guidanceKind == GuidanceKind::ErrorGuidance) {
        checkNullCheckAfterDeref(cond, state);
      }
    }
    break;
  }
  case Instruction::IndirectBr: {
    // implements indirect branch to a label within the current function
    const auto bi = cast<IndirectBrInst>(i);
    auto address = eval(ki, 0, state).value;
    address = toUnique(state, address);

    // concrete address
    if (const auto CE = dyn_cast<ConstantExpr>(address.get())) {
      const auto bb_address =
          (BasicBlock *)CE->getZExtValue(Context::get().getPointerWidth());
      transferToBasicBlock(bb_address, bi->getParent(), state);
      break;
    }

    // symbolic address
    const auto numDestinations = bi->getNumDestinations();
    std::vector<BasicBlock *> targets;
    targets.reserve(numDestinations);
    std::vector<ref<Expr>> expressions;
    expressions.reserve(numDestinations);

    ref<Expr> errorCase = ConstantExpr::alloc(1, Expr::Bool);
    SmallPtrSet<BasicBlock *, 5> destinations;
    KFunction *kf = state.stack.callStack().back().kf;
    // collect and check destinations from label list
    for (unsigned k = 0; k < numDestinations; ++k) {
      // filter duplicates
      const auto d = bi->getDestination(k);
      if (destinations.count(d))
        continue;
      if (!canReachSomeTargetFromBlock(state, kf->blockMap[d]))
        continue;
      destinations.insert(d);

      // create address expression
      const auto PE = Expr::createPointer(reinterpret_cast<std::uint64_t>(d));
      ref<Expr> e = EqExpr::create(address, PE);

      // exclude address from errorCase
      errorCase = AndExpr::create(errorCase, Expr::createIsZero(e));

      // check feasibility
      bool result;
      bool success __attribute__((unused)) = solver->mayBeTrue(
          state.constraints.cs(), e, result, state.queryMetaData);
      assert(success && "FIXME: Unhandled solver failure");
      if (result) {
        targets.push_back(d);
        expressions.push_back(e);
      }
    }
    // check errorCase feasibility
    bool result;
    bool success __attribute__((unused)) = solver->mayBeTrue(
        state.constraints.cs(), errorCase, result, state.queryMetaData);
    assert(success && "FIXME: Unhandled solver failure");
    if (result) {
      expressions.push_back(errorCase);
    }

    // fork states
    std::vector<ExecutionState *> branches;
    branch(state, expressions, branches, BranchType::Indirect);

    // terminate error state
    if (result) {
      terminateStateOnExecError(*branches.back(),
                                "indirectbr: illegal label address");
      branches.pop_back();
    }

    // branch states to resp. target blocks
    assert(targets.size() == branches.size());
    for (std::vector<ExecutionState *>::size_type k = 0; k < branches.size();
         ++k) {
      if (branches[k]) {
        transferToBasicBlock(targets[k], bi->getParent(), *branches[k]);
      }
    }

    break;
  }
  case Instruction::Switch: {
    SwitchInst *si = cast<SwitchInst>(i);
    ref<Expr> cond = eval(ki, 0, state).value;
    BasicBlock *bb = si->getParent();

    cond = toUnique(state, cond);
    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(cond)) {
      // Somewhat gross to create these all the time, but fine till we
      // switch to an internal rep.
      llvm::IntegerType *Ty = cast<IntegerType>(si->getCondition()->getType());
      ConstantInt *ci = ConstantInt::get(Ty, CE->getZExtValue());
      unsigned index = si->findCaseValue(ci)->getSuccessorIndex();
      transferToBasicBlock(si->getSuccessor(index), si->getParent(), state);
    } else {
      // Handle possible different branch targets

      // We have the following assumptions:
      // - each case value is mutual exclusive to all other values
      // - order of case branches is based on the order of the expressions of
      //   the case values, still default is handled last
      std::vector<BasicBlock *> bbOrder;
      std::map<BasicBlock *, ref<Expr>> branchTargets;

      std::map<ref<Expr>, BasicBlock *> expressionOrder;

      // Iterate through all non-default cases and order them by expressions
      for (auto i : si->cases()) {
        ref<Expr> value = evalConstant(i.getCaseValue(), state.roundingMode);

        BasicBlock *caseSuccessor = i.getCaseSuccessor();
        expressionOrder.insert(std::make_pair(value, caseSuccessor));
      }

      std::vector<ref<Expr>> notMatches;

      KFunction *kf = state.stack.callStack().back().kf;

      // iterate through all non-default cases but in order of the expressions
      for (std::map<ref<Expr>, BasicBlock *>::iterator
               it = expressionOrder.begin(),
               itE = expressionOrder.end();
           it != itE; ++it) {
        ref<Expr> match = EqExpr::create(cond, it->first);
        BasicBlock *caseSuccessor = it->second;

        // skip if case has same successor basic block as default case
        // (should work even with phi nodes as a switch is a single
        // terminating instruction)
        if (it->second == si->getDefaultDest())
          continue;

        notMatches.push_back(Expr::createIsZero(match));

        if (!canReachSomeTargetFromBlock(state, kf->blockMap[caseSuccessor]))
          continue;

        // Check if control flow could take this case
        bool result;
        match = optimizer.optimizeExpr(match, false);
        bool success = solver->mayBeTrue(state.constraints.cs(), match, result,
                                         state.queryMetaData);
        assert(success && "FIXME: Unhandled solver failure");
        (void)success;
        if (result) {
          // Handle the case that a basic block might be the target of
          // multiple switch cases. Currently we generate an expression
          // containing all switch-case values for the same target basic
          // block. We spare us forking too many times but we generate more
          // complex condition expressions
          // TODO Add option to allow to choose between those behaviors
          std::pair<std::map<BasicBlock *, ref<Expr>>::iterator, bool> res =
              branchTargets.insert(std::make_pair(
                  caseSuccessor, ConstantExpr::alloc(0, Expr::Bool)));

          res.first->second = OrExpr::create(match, res.first->second);

          // Only add basic blocks which have not been target of a branch yet
          if (res.second) {
            bbOrder.push_back(caseSuccessor);
          }
        }
      }

      auto defaultDest = si->getDefaultDest();

      // Make sure that the default value does not contain this target's
      // value
      while (notMatches.size() > 1) {
        std::vector<ref<Expr>> tmp;
        std::swap(notMatches, tmp);
        for (unsigned i = 0; i * 2 < tmp.size(); ++i) {
          ref<Expr> notMatch = tmp.at(i * 2);
          if (i * 2 + 1 < tmp.size()) {
            notMatch = AndExpr::create(notMatch, tmp.at(i * 2 + 1));
          }
          notMatches.push_back(notMatch);
        }
      }
      assert(notMatches.size() <= 1);

      // Track default branch values
      ref<Expr> defaultValue = ConstantExpr::alloc(1, Expr::Bool);
      if (notMatches.size() == 1) {
        defaultValue = notMatches.back();
      }

      if (canReachSomeTargetFromBlock(state, kf->blockMap[defaultDest])) {
        // Check if control could take the default case
        defaultValue = optimizer.optimizeExpr(defaultValue, false);
        bool res;
        bool success = solver->mayBeTrue(state.constraints.cs(), defaultValue,
                                         res, state.queryMetaData);
        assert(success && "FIXME: Unhandled solver failure");
        (void)success;
        if (res) {
          std::pair<std::map<BasicBlock *, ref<Expr>>::iterator, bool> ret =
              branchTargets.insert(std::make_pair(defaultDest, defaultValue));
          if (ret.second) {
            bbOrder.push_back(defaultDest);
          }
        }
      }

      // Fork the current state with each state having one of the possible
      // successors of this switch
      std::vector<ref<Expr>> conditions;
      for (std::vector<BasicBlock *>::iterator it = bbOrder.begin(),
                                               ie = bbOrder.end();
           it != ie; ++it) {
        conditions.push_back(branchTargets[*it]);
      }
      std::vector<ExecutionState *> branches;
      branch(state, conditions, branches, BranchType::Switch);

      std::vector<ExecutionState *>::iterator bit = branches.begin();
      for (std::vector<BasicBlock *>::iterator it = bbOrder.begin(),
                                               ie = bbOrder.end();
           it != ie; ++it) {
        ExecutionState *es = *bit;
        if (es)
          transferToBasicBlock(*it, bb, *es);
        ++bit;
      }
    }
    break;
  }
  case Instruction::Unreachable:
    // Note that this is not necessarily an internal bug, llvm will
    // generate unreachable instructions in cases where it knows the
    // program will crash. So it is effectively a SEGV or internal
    // error.
    terminateStateOnExecError(state, "reached \"unreachable\" instruction");
    break;

  case Instruction::Invoke:
  case Instruction::Call: {
    // Ignore debug intrinsic calls
    if (isa<DbgInfoIntrinsic>(i))
      break;

    const CallBase &cb = cast<CallBase>(*i);
    Value *fp = cb.getCalledOperand();
    unsigned numArgs = cb.arg_size();
    Function *f = getTargetFunction(fp);

    // evaluate arguments
    std::vector<ref<Expr>> arguments;
    arguments.reserve(numArgs);

    for (unsigned j = 0; j < numArgs; ++j)
      arguments.push_back(eval(ki, j + 1, state).value);

    if (auto *asmValue =
            dyn_cast<InlineAsm>(fp)) { // TODO: move to `executeCall`
      if (ExternalCalls != ExternalCallPolicy::None) {
        KInlineAsm callable(asmValue);
        callExternalFunction(state, ki, &callable, arguments);
      } else {
        terminateStateOnExecError(
            state, "external calls disallowed (in particular inline asm)");
      }
      break;
    }

    if (f) {
      const FunctionType *fType = f->getFunctionType();
      const FunctionType *fpType =
          dyn_cast<FunctionType>(fp->getType()->getPointerElementType());

      // special case the call with a bitcast case
      if (fType != fpType) {
        assert(fType && fpType && "unable to get function type");

        // XXX check result coercion

        // XXX this really needs thought and validation
        unsigned i = 0;
        for (std::vector<ref<Expr>>::iterator ai = arguments.begin(),
                                              ie = arguments.end();
             ai != ie; ++ai) {
          Expr::Width to, from = (*ai)->getWidth();

          if (i < fType->getNumParams()) {
            to = getWidthForLLVMType(fType->getParamType(i));

            if (from != to) {
              // XXX need to check other param attrs ?
              bool isSExt = cb.paramHasAttr(i, llvm::Attribute::SExt);
              if (isSExt) {
                arguments[i] = SExtExpr::create(arguments[i], to);
              } else {
                arguments[i] = ZExtExpr::create(arguments[i], to);
              }
            }
          }

          i++;
        }
      }

      executeCall(state, ki, f, arguments);
    } else {
      ref<Expr> v = eval(ki, 0, state).value;

      ExecutionState *free = &state;
      bool hasInvalid = false, first = true;

      /* XXX This is wasteful, no need to do a full evaluate since we
          have already got a value. But in the end the caches should
          handle it for us, albeit with some overhead. */
      do {
        if (!first && interpreterOpts.Mock == MockPolicy::Failed) {
          free = nullptr;
          if (ki->inst()->getType()->isSized()) {
            prepareMockValue(state, "mockExternResult", ki->inst()->getType(),
                             ki);
          }
        } else {
          v = optimizer.optimizeExpr(v, true);
          ref<Expr> possiblePointer;
          bool success = solver->getValue(free->constraints.cs(), v,
                                          possiblePointer, free->queryMetaData);
          assert(success && "FIXME: Unhandled solver failure");
          (void)success;
          ref<ConstantExpr> value = possiblePointer->getValue();
          StatePair res =
              forkInternal(*free, EqExpr::create(v, value), BranchType::Call);
          if (res.first) {
            uint64_t addr = value->getZExtValue();
            auto it = legalFunctions.find(addr);
            if (it != legalFunctions.end()) {
              f = it->second;

              // Don't give warning on unique resolution
              if (res.second || !first)
                klee_warning_once(reinterpret_cast<void *>(addr),
                                  "resolved symbolic function pointer to: %s",
                                  f->getName().data());

              executeCall(*res.first, ki, f, arguments);
            } else {
              if (!hasInvalid) {
                terminateStateOnExecError(state, "invalid function pointer");
                hasInvalid = true;
              }
            }
          }

          first = false;
          free = res.second;
          timers.invoke();
        }
      } while (free && !haltExecution);
    }
    break;
  }
  case Instruction::PHI: {
    if (state.incomingBBIndex == -1)
      prepareSymbolicValue(state, ki);
    else {
      ref<Expr> result = eval(ki, state.incomingBBIndex, state).value;
      bindLocal(ki, state, result);
    }
    break;
  }

    // Special instructions
  case Instruction::Select: {
    // NOTE: It is not required that operands 1 and 2 be of scalar type.
    ref<Expr> cond = eval(ki, 0, state).value;
    ref<Expr> tExpr = eval(ki, 1, state).value;
    ref<Expr> fExpr = eval(ki, 2, state).value;
    ref<Expr> result = SelectExpr::create(cond, tExpr, fExpr);
    bindLocal(ki, state, result);
    break;
  }

  case Instruction::VAArg:
    terminateStateOnExecError(state, "unexpected VAArg instruction");
    break;

    // Arithmetic / logical

  case Instruction::Add: {
    ref<Expr> left = eval(ki, 0, state).value;
    ref<Expr> right = eval(ki, 1, state).value;
    bindLocal(ki, state, AddExpr::create(left, right));
    break;
  }

  case Instruction::Sub: {
    ref<Expr> left = eval(ki, 0, state).value;
    ref<Expr> right = eval(ki, 1, state).value;
    bindLocal(ki, state, SubExpr::create(left, right));
    break;
  }

  case Instruction::Mul: {
    ref<Expr> left = eval(ki, 0, state).value;
    ref<Expr> right = eval(ki, 1, state).value;
    bindLocal(ki, state, MulExpr::create(left, right));
    break;
  }

  case Instruction::UDiv: {
    ref<Expr> left = eval(ki, 0, state).value;
    ref<Expr> right = eval(ki, 1, state).value;
    ref<Expr> result = UDivExpr::create(left, right);
    bindLocal(ki, state, result);
    break;
  }

  case Instruction::SDiv: {
    ref<Expr> left = eval(ki, 0, state).value;
    ref<Expr> right = eval(ki, 1, state).value;
    ref<Expr> result = SDivExpr::create(left, right);
    bindLocal(ki, state, result);
    break;
  }

  case Instruction::URem: {
    ref<Expr> left = eval(ki, 0, state).value;
    ref<Expr> right = eval(ki, 1, state).value;
    ref<Expr> result = URemExpr::create(left, right);
    bindLocal(ki, state, result);
    break;
  }

  case Instruction::SRem: {
    ref<Expr> left = eval(ki, 0, state).value;
    ref<Expr> right = eval(ki, 1, state).value;
    ref<Expr> result = SRemExpr::create(left, right);
    bindLocal(ki, state, result);
    break;
  }

  case Instruction::And: {
    ref<Expr> left = eval(ki, 0, state).value;
    ref<Expr> right = eval(ki, 1, state).value;
    ref<Expr> result = AndExpr::create(left, right);
    bindLocal(ki, state, result);
    break;
  }

  case Instruction::Or: {
    ref<Expr> left = eval(ki, 0, state).value;
    ref<Expr> right = eval(ki, 1, state).value;
    ref<Expr> result = OrExpr::create(left, right);
    bindLocal(ki, state, result);
    break;
  }

  case Instruction::Xor: {
    ref<Expr> left = eval(ki, 0, state).value;
    ref<Expr> right = eval(ki, 1, state).value;
    ref<Expr> result = XorExpr::create(left, right);
    bindLocal(ki, state, result);
    break;
  }

  case Instruction::Shl: {
    ref<Expr> left = eval(ki, 0, state).value;
    ref<Expr> right = eval(ki, 1, state).value;
    ref<Expr> result = ShlExpr::create(left, right);
    bindLocal(ki, state, result);
    break;
  }

  case Instruction::LShr: {
    ref<Expr> left = eval(ki, 0, state).value;
    ref<Expr> right = eval(ki, 1, state).value;
    ref<Expr> result = LShrExpr::create(left, right);
    bindLocal(ki, state, result);
    break;
  }

  case Instruction::AShr: {
    ref<Expr> left = eval(ki, 0, state).value;
    ref<Expr> right = eval(ki, 1, state).value;
    ref<Expr> result = AShrExpr::create(left, right);
    bindLocal(ki, state, result);
    break;
  }

    // Compare

  case Instruction::ICmp: {
    CmpInst *ci = cast<CmpInst>(i);
    ICmpInst *ii = cast<ICmpInst>(ci);

    switch (ii->getPredicate()) {
    case ICmpInst::ICMP_EQ: {
      ref<Expr> left = eval(ki, 0, state).value;
      ref<Expr> right = eval(ki, 1, state).value;
      ref<Expr> result = EqExpr::create(left, right);
      bindLocal(ki, state, result);
      break;
    }

    case ICmpInst::ICMP_NE: {
      ref<Expr> left = eval(ki, 0, state).value;
      ref<Expr> right = eval(ki, 1, state).value;
      ref<Expr> result = NeExpr::create(left, right);
      bindLocal(ki, state, result);
      break;
    }

    case ICmpInst::ICMP_UGT: {
      ref<Expr> left = eval(ki, 0, state).value;
      ref<Expr> right = eval(ki, 1, state).value;
      ref<Expr> result = UgtExpr::create(left, right);
      bindLocal(ki, state, result);
      break;
    }

    case ICmpInst::ICMP_UGE: {
      ref<Expr> left = eval(ki, 0, state).value;
      ref<Expr> right = eval(ki, 1, state).value;
      ref<Expr> result = UgeExpr::create(left, right);
      bindLocal(ki, state, result);
      break;
    }

    case ICmpInst::ICMP_ULT: {
      ref<Expr> left = eval(ki, 0, state).value;
      ref<Expr> right = eval(ki, 1, state).value;
      ref<Expr> result = UltExpr::create(left, right);
      bindLocal(ki, state, result);
      break;
    }

    case ICmpInst::ICMP_ULE: {
      ref<Expr> left = eval(ki, 0, state).value;
      ref<Expr> right = eval(ki, 1, state).value;
      ref<Expr> result = UleExpr::create(left, right);
      bindLocal(ki, state, result);
      break;
    }

    case ICmpInst::ICMP_SGT: {
      ref<Expr> left = eval(ki, 0, state).value;
      ref<Expr> right = eval(ki, 1, state).value;
      ref<Expr> result = SgtExpr::create(left, right);
      bindLocal(ki, state, result);
      break;
    }

    case ICmpInst::ICMP_SGE: {
      ref<Expr> left = eval(ki, 0, state).value;
      ref<Expr> right = eval(ki, 1, state).value;
      ref<Expr> result = SgeExpr::create(left, right);
      bindLocal(ki, state, result);
      break;
    }

    case ICmpInst::ICMP_SLT: {
      ref<Expr> left = eval(ki, 0, state).value;
      ref<Expr> right = eval(ki, 1, state).value;
      ref<Expr> result = SltExpr::create(left, right);
      bindLocal(ki, state, result);
      break;
    }

    case ICmpInst::ICMP_SLE: {
      ref<Expr> left = eval(ki, 0, state).value;
      ref<Expr> right = eval(ki, 1, state).value;
      ref<Expr> result = SleExpr::create(left, right);
      bindLocal(ki, state, result);
      break;
    }

    default:
      terminateStateOnExecError(state, "invalid ICmp predicate");
    }
    break;
  }

    // Memory instructions...
  case Instruction::Alloca: {
    // FIXME: Should we provide a way to switch between
    //  getTypeStoreSize and GetTypeAllocSize()? The first way we
    //  could treat reading outside the memory of a type as an out
    //  of bounds access. Unfortunately the first way is not always desirable
    //  as for long double has storeSize == 80 but allocSize == 128 on x86_64
    //  and it is legitimate to access the padding using sizeof(long double)
    AllocaInst *ai = cast<AllocaInst>(i);
    unsigned elementSize =
        kmodule->targetData->getTypeAllocSize(ai->getAllocatedType());
    ref<Expr> size = Expr::createPointer(elementSize);
    if (ai->isArrayAllocation()) {
      ref<Expr> count = eval(ki, 0, state).value;
      count = Expr::createZExtToPointerWidth(count);
      size = MulExpr::create(size, count);
    }
    executeAlloc(state, size, true, ki,
                 typeSystemManager->getWrappedType(ai->getType()));
    break;
  }

  case Instruction::Load: {
    ref<Expr> base = eval(ki, 0, state).value;
    executeMemoryOperation(
        state, false,
        typeSystemManager->getWrappedType(
            cast<llvm::LoadInst>(ki->inst())->getPointerOperandType()),
        makePointer(base), 0, ki);
    break;
  }
  case Instruction::Store: {
    ref<Expr> base = eval(ki, 1, state).value;
    ref<Expr> value = eval(ki, 0, state).value;
    executeMemoryOperation(
        state, true,
        typeSystemManager->getWrappedType(
            cast<llvm::StoreInst>(ki->inst())->getPointerOperandType()),
        makePointer(base), value, ki);
    break;
  }

  case Instruction::GetElementPtr: {
    KGEPInstruction *kgepi = static_cast<KGEPInstruction *>(ki);
    GetElementPtrInst *gepInst =
        static_cast<GetElementPtrInst *>(kgepi->inst());

    ref<Expr> base = eval(ki, 0, state).value;
    ref<PointerExpr> pointer = makePointer(base);
    base = pointer->getBase();
    ref<Expr> offset = pointer->getOffset();

    for (std::vector<std::pair<unsigned, uint64_t>>::iterator
             it = kgepi->indices.begin(),
             ie = kgepi->indices.end();
         it != ie; ++it) {
      uint64_t elementSize = it->second;
      ref<Expr> index = eval(ki, it->first, state).value;
      offset = AddExpr::create(
          offset, MulExpr::create(Expr::createSExtToPointerWidth(index),
                                  Expr::createPointer(elementSize)));
    }
    if (kgepi->offset)
      offset = AddExpr::create(offset, Expr::createPointer(kgepi->offset));
    ref<Expr> address;
    if (ref<PointerExpr> pointerOffset = dyn_cast<PointerExpr>(offset)) {
      address = cast<PointerExpr>(PointerExpr::create(base, base))
                    ->Add(pointerOffset);
    } else {
      address = PointerExpr::create(base, AddExpr::create(base, offset));
    }

    state.gepExprBases[base] = {gepInst->getSourceElementType()};

    bindLocal(ki, state, address);
    break;
  }

    // Conversion
  case Instruction::Trunc: {
    CastInst *ci = cast<CastInst>(i);
    ref<Expr> result = ExtractExpr::create(eval(ki, 0, state).value, 0,
                                           getWidthForLLVMType(ci->getType()));
    bindLocal(ki, state, result);
    break;
  }
  case Instruction::ZExt: {
    CastInst *ci = cast<CastInst>(i);
    ref<Expr> result = ZExtExpr::create(eval(ki, 0, state).value,
                                        getWidthForLLVMType(ci->getType()));
    bindLocal(ki, state, result);
    break;
  }
  case Instruction::SExt: {
    CastInst *ci = cast<CastInst>(i);
    ref<Expr> result = SExtExpr::create(eval(ki, 0, state).value,
                                        getWidthForLLVMType(ci->getType()));
    bindLocal(ki, state, result);
    break;
  }

  case Instruction::IntToPtr: {
    CastInst *ci = cast<CastInst>(i);
    Expr::Width pType = getWidthForLLVMType(ci->getType());
    ref<Expr> arg = eval(ki, 0, state).value;
    bindLocal(ki, state, PointerExpr::create(ZExtExpr::create(arg, pType)));
    break;
  }
  case Instruction::PtrToInt: {
    CastInst *ci = cast<CastInst>(i);
    Expr::Width iType = getWidthForLLVMType(ci->getType());
    ref<Expr> arg = eval(ki, 0, state).value;
    bindLocal(ki, state, ZExtExpr::create(arg, iType));
    break;
  }

  case Instruction::BitCast: {
    ref<Expr> result = eval(ki, 0, state).value;
    BitCastInst *bc = cast<BitCastInst>(ki->inst());

    llvm::Type *castToType = bc->getType();

    if (X86FPAsX87FP80 && result->getWidth() == Expr::Fl80 &&
        !castToType->isFloatingPointTy() &&
        Context::get().getPointerWidth() == 32) {
      result = X87FP80ToFPTrunc(
          result, getWidthForLLVMType(bc->getOperand(0)->getType()),
          state.roundingMode);
    }

    if (X86FPAsX87FP80 && castToType->isFloatingPointTy() &&
        Context::get().getPointerWidth() == 32) {
      result = FPToX87FP80Ext(result);
    }

    if (castToType->isPointerTy()) {
      castToType = castToType->getPointerElementType();
      if (ref<PointerExpr> pointer = cast<PointerExpr>(makePointer(result))) {
        ref<Expr> base = pointer->getBase();
        if (state.isGEPExpr(base)) {
          state.gepExprBases[base] = castToType;
        }
      }
    }

    bindLocal(ki, state, result);
    break;
  }

  // Floating point instructions
#ifndef ENABLE_FP
  case Instruction::FNeg: {
    ref<ConstantExpr> arg =
        toConstant(state, eval(ki, 0, state).value, "floating point");
    if (!fpWidthToSemantics(arg->getWidth()))
      return terminateStateOnExecError(state, "Unsupported FNeg operation");

    llvm::APFloat Res(*fpWidthToSemantics(arg->getWidth()), arg->getAPValue());
    Res = llvm::neg(Res);
    bindLocal(ki, state, ConstantExpr::alloc(Res.bitcastToAPInt()));
    break;
  }

  case Instruction::FAdd: {
    ref<ConstantExpr> left =
        toConstant(state, eval(ki, 0, state).value, "floating point");
    ref<ConstantExpr> right =
        toConstant(state, eval(ki, 1, state).value, "floating point");
    if (!fpWidthToSemantics(left->getWidth()) ||
        !fpWidthToSemantics(right->getWidth()))
      return terminateStateOnExecError(state, "Unsupported FAdd operation");

    llvm::APFloat Res(*fpWidthToSemantics(left->getWidth()),
                      left->getAPValue());
    Res.add(
        APFloat(*fpWidthToSemantics(right->getWidth()), right->getAPValue()),
        APFloat::rmNearestTiesToEven);
    bindLocal(ki, state, ConstantExpr::alloc(Res.bitcastToAPInt()));
    break;
  }

  case Instruction::FSub: {
    ref<ConstantExpr> left =
        toConstant(state, eval(ki, 0, state).value, "floating point");
    ref<ConstantExpr> right =
        toConstant(state, eval(ki, 1, state).value, "floating point");
    if (!fpWidthToSemantics(left->getWidth()) ||
        !fpWidthToSemantics(right->getWidth()))
      return terminateStateOnExecError(state, "Unsupported FSub operation");
    llvm::APFloat Res(*fpWidthToSemantics(left->getWidth()),
                      left->getAPValue());
    Res.subtract(
        APFloat(*fpWidthToSemantics(right->getWidth()), right->getAPValue()),
        APFloat::rmNearestTiesToEven);
    bindLocal(ki, state, ConstantExpr::alloc(Res.bitcastToAPInt()));
    break;
  }

  case Instruction::FMul: {
    ref<ConstantExpr> left =
        toConstant(state, eval(ki, 0, state).value, "floating point");
    ref<ConstantExpr> right =
        toConstant(state, eval(ki, 1, state).value, "floating point");
    if (!fpWidthToSemantics(left->getWidth()) ||
        !fpWidthToSemantics(right->getWidth()))
      return terminateStateOnExecError(state, "Unsupported FMul operation");

    llvm::APFloat Res(*fpWidthToSemantics(left->getWidth()),
                      left->getAPValue());
    Res.multiply(
        APFloat(*fpWidthToSemantics(right->getWidth()), right->getAPValue()),
        APFloat::rmNearestTiesToEven);
    bindLocal(ki, state, ConstantExpr::alloc(Res.bitcastToAPInt()));
    break;
  }

  case Instruction::FDiv: {
    ref<ConstantExpr> left =
        toConstant(state, eval(ki, 0, state).value, "floating point");
    ref<ConstantExpr> right =
        toConstant(state, eval(ki, 1, state).value, "floating point");
    if (!fpWidthToSemantics(left->getWidth()) ||
        !fpWidthToSemantics(right->getWidth()))
      return terminateStateOnExecError(state, "Unsupported FDiv operation");

    llvm::APFloat Res(*fpWidthToSemantics(left->getWidth()),
                      left->getAPValue());
    Res.divide(
        APFloat(*fpWidthToSemantics(right->getWidth()), right->getAPValue()),
        APFloat::rmNearestTiesToEven);
    bindLocal(ki, state, ConstantExpr::alloc(Res.bitcastToAPInt()));
    break;
  }

  case Instruction::FRem: {
    ref<ConstantExpr> left =
        toConstant(state, eval(ki, 0, state).value, "floating point");
    ref<ConstantExpr> right =
        toConstant(state, eval(ki, 1, state).value, "floating point");
    if (!fpWidthToSemantics(left->getWidth()) ||
        !fpWidthToSemantics(right->getWidth()))
      return terminateStateOnExecError(state, "Unsupported FRem operation");
    llvm::APFloat Res(*fpWidthToSemantics(left->getWidth()),
                      left->getAPValue());
    Res.mod(
        APFloat(*fpWidthToSemantics(right->getWidth()), right->getAPValue()));
    bindLocal(ki, state, ConstantExpr::alloc(Res.bitcastToAPInt()));
    break;
  }

  case Instruction::FPTrunc: {
    FPTruncInst *fi = cast<FPTruncInst>(i);
    Expr::Width resultType = getWidthForLLVMType(fi->getType());
    ref<ConstantExpr> arg =
        toConstant(state, eval(ki, 0, state).value, "floating point");
    if (!fpWidthToSemantics(arg->getWidth()) || resultType > arg->getWidth())
      return terminateStateOnExecError(state, "Unsupported FPTrunc operation");

    llvm::APFloat Res(*fpWidthToSemantics(arg->getWidth()), arg->getAPValue());
    bool losesInfo = false;
    Res.convert(*fpWidthToSemantics(resultType),
                llvm::APFloat::rmNearestTiesToEven, &losesInfo);
    bindLocal(ki, state, ConstantExpr::alloc(Res));
    break;
  }

  case Instruction::FPExt: {
    FPExtInst *fi = cast<FPExtInst>(i);
    Expr::Width resultType = getWidthForLLVMType(fi->getType());
    ref<ConstantExpr> arg =
        toConstant(state, eval(ki, 0, state).value, "floating point");
    if (!fpWidthToSemantics(arg->getWidth()) || arg->getWidth() > resultType)
      return terminateStateOnExecError(state, "Unsupported FPExt operation");
    llvm::APFloat Res(*fpWidthToSemantics(arg->getWidth()), arg->getAPValue());
    bool losesInfo = false;
    Res.convert(*fpWidthToSemantics(resultType),
                llvm::APFloat::rmNearestTiesToEven, &losesInfo);
    bindLocal(ki, state, ConstantExpr::alloc(Res));
    break;
  }

  case Instruction::FPToUI: {
    FPToUIInst *fi = cast<FPToUIInst>(i);
    Expr::Width resultType = getWidthForLLVMType(fi->getType());
    ref<ConstantExpr> arg =
        toConstant(state, eval(ki, 0, state).value, "floating point");
    if (!fpWidthToSemantics(arg->getWidth()) || resultType > 64)
      return terminateStateOnExecError(state, "Unsupported FPToUI operation");

    llvm::APFloat Arg(*fpWidthToSemantics(arg->getWidth()), arg->getAPValue());
    uint64_t value = 0;
    bool isExact = true;
    auto valueRef = makeMutableArrayRef(value);
    Arg.convertToInteger(valueRef, resultType, false,
                         llvm::APFloat::rmTowardZero, &isExact);
    bindLocal(ki, state, ConstantExpr::alloc(value, resultType));
    break;
  }

  case Instruction::FPToSI: {
    FPToSIInst *fi = cast<FPToSIInst>(i);
    Expr::Width resultType = getWidthForLLVMType(fi->getType());
    ref<ConstantExpr> arg =
        toConstant(state, eval(ki, 0, state).value, "floating point");
    if (!fpWidthToSemantics(arg->getWidth()) || resultType > 64)
      return terminateStateOnExecError(state, "Unsupported FPToSI operation");
    llvm::APFloat Arg(*fpWidthToSemantics(arg->getWidth()), arg->getAPValue());

    uint64_t value = 0;
    bool isExact = true;
    auto valueRef = makeMutableArrayRef(value);
    Arg.convertToInteger(valueRef, resultType, true,
                         llvm::APFloat::rmTowardZero, &isExact);
    bindLocal(ki, state, ConstantExpr::alloc(value, resultType));
    break;
  }

  case Instruction::UIToFP: {
    UIToFPInst *fi = cast<UIToFPInst>(i);
    Expr::Width resultType = getWidthForLLVMType(fi->getType());
    ref<ConstantExpr> arg =
        toConstant(state, eval(ki, 0, state).value, "floating point");
    const llvm::fltSemantics *semantics = fpWidthToSemantics(resultType);
    if (!semantics)
      return terminateStateOnExecError(state, "Unsupported UIToFP operation");
    llvm::APFloat f(*semantics, 0);
    f.convertFromAPInt(arg->getAPValue(), false,
                       llvm::APFloat::rmNearestTiesToEven);

    bindLocal(ki, state, ConstantExpr::alloc(f));
    break;
  }

  case Instruction::SIToFP: {
    SIToFPInst *fi = cast<SIToFPInst>(i);
    Expr::Width resultType = getWidthForLLVMType(fi->getType());
    ref<ConstantExpr> arg =
        toConstant(state, eval(ki, 0, state).value, "floating point");
    const llvm::fltSemantics *semantics = fpWidthToSemantics(resultType);
    if (!semantics)
      return terminateStateOnExecError(state, "Unsupported SIToFP operation");
    llvm::APFloat f(*semantics, 0);
    f.convertFromAPInt(arg->getAPValue(), true,
                       llvm::APFloat::rmNearestTiesToEven);

    bindLocal(ki, state, ConstantExpr::alloc(f));
    break;
  }

  case Instruction::FCmp: {
    FCmpInst *fi = cast<FCmpInst>(i);
    ref<ConstantExpr> left =
        toConstant(state, eval(ki, 0, state).value, "floating point");
    ref<ConstantExpr> right =
        toConstant(state, eval(ki, 1, state).value, "floating point");
    if (!fpWidthToSemantics(left->getWidth()) ||
        !fpWidthToSemantics(right->getWidth()))
      return terminateStateOnExecError(state, "Unsupported FCmp operation");

    APFloat LHS(*fpWidthToSemantics(left->getWidth()), left->getAPValue());
    APFloat RHS(*fpWidthToSemantics(right->getWidth()), right->getAPValue());
    APFloat::cmpResult CmpRes = LHS.compare(RHS);

    bool Result = false;
    switch (fi->getPredicate()) {
      // Predicates which only care about whether or not the operands are
      // NaNs.
    case FCmpInst::FCMP_ORD:
      Result = (CmpRes != APFloat::cmpUnordered);
      break;

    case FCmpInst::FCMP_UNO:
      Result = (CmpRes == APFloat::cmpUnordered);
      break;

      // Ordered comparisons return false if either operand is NaN.  Unordered
      // comparisons return true if either operand is NaN.
    case FCmpInst::FCMP_UEQ:
      Result = (CmpRes == APFloat::cmpUnordered || CmpRes == APFloat::cmpEqual);
      break;
    case FCmpInst::FCMP_OEQ:
      Result = (CmpRes != APFloat::cmpUnordered && CmpRes == APFloat::cmpEqual);
      break;

    case FCmpInst::FCMP_UGT:
      Result = (CmpRes == APFloat::cmpUnordered ||
                CmpRes == APFloat::cmpGreaterThan);
      break;
    case FCmpInst::FCMP_OGT:
      Result = (CmpRes != APFloat::cmpUnordered &&
                CmpRes == APFloat::cmpGreaterThan);
      break;

    case FCmpInst::FCMP_UGE:
      Result =
          (CmpRes == APFloat::cmpUnordered ||
           (CmpRes == APFloat::cmpGreaterThan || CmpRes == APFloat::cmpEqual));
      break;
    case FCmpInst::FCMP_OGE:
      Result =
          (CmpRes != APFloat::cmpUnordered &&
           (CmpRes == APFloat::cmpGreaterThan || CmpRes == APFloat::cmpEqual));
      break;

    case FCmpInst::FCMP_ULT:
      Result =
          (CmpRes == APFloat::cmpUnordered || CmpRes == APFloat::cmpLessThan);
      break;
    case FCmpInst::FCMP_OLT:
      Result =
          (CmpRes != APFloat::cmpUnordered && CmpRes == APFloat::cmpLessThan);
      break;

    case FCmpInst::FCMP_ULE:
      Result =
          (CmpRes == APFloat::cmpUnordered ||
           (CmpRes == APFloat::cmpLessThan || CmpRes == APFloat::cmpEqual));
      break;
    case FCmpInst::FCMP_OLE:
      Result =
          (CmpRes != APFloat::cmpUnordered &&
           (CmpRes == APFloat::cmpLessThan || CmpRes == APFloat::cmpEqual));
      break;

    case FCmpInst::FCMP_UNE:
      Result = (CmpRes == APFloat::cmpUnordered || CmpRes != APFloat::cmpEqual);
      break;
    case FCmpInst::FCMP_ONE:
      Result = (CmpRes != APFloat::cmpUnordered && CmpRes != APFloat::cmpEqual);
      break;

    default:
      assert(0 && "Invalid FCMP predicate!");
      break;
    case FCmpInst::FCMP_FALSE:
      Result = false;
      break;
    case FCmpInst::FCMP_TRUE:
      Result = true;
      break;
    }

    bindLocal(ki, state, ConstantExpr::alloc(Result, Expr::Bool));
    break;
  }
#else
  case Instruction::FAdd: {
    ref<Expr> left = eval(ki, 0, state).value;
    ref<Expr> right = eval(ki, 1, state).value;
    if (!fpWidthToSemantics(left->getWidth()) ||
        !fpWidthToSemantics(right->getWidth()))
      return terminateStateOnExecError(state, "Unsupported FAdd operation");
    ref<Expr> result = FAddExpr::create(left, right, state.roundingMode);
    bindLocal(ki, state, result);
    break;
  }

  case Instruction::FSub: {
    ref<Expr> left = eval(ki, 0, state).value;
    ref<Expr> right = eval(ki, 1, state).value;
    if (!fpWidthToSemantics(left->getWidth()) ||
        !fpWidthToSemantics(right->getWidth()))
      return terminateStateOnExecError(state, "Unsupported FSub operation");
    ref<Expr> result = FSubExpr::create(left, right, state.roundingMode);
    bindLocal(ki, state, result);
    break;
  }

  case Instruction::FMul: {
    ref<Expr> left = eval(ki, 0, state).value;
    ref<Expr> right = eval(ki, 1, state).value;
    if (!fpWidthToSemantics(left->getWidth()) ||
        !fpWidthToSemantics(right->getWidth()))
      return terminateStateOnExecError(state, "Unsupported FMul operation");
    ref<Expr> result = FMulExpr::create(left, right, state.roundingMode);
    bindLocal(ki, state, result);
    break;
  }

  case Instruction::FDiv: {
    ref<Expr> left = eval(ki, 0, state).value;
    ref<Expr> right = eval(ki, 1, state).value;
    if (!fpWidthToSemantics(left->getWidth()) ||
        !fpWidthToSemantics(right->getWidth()))
      return terminateStateOnExecError(state, "Unsupported FDiv operation");
    ref<Expr> result = FDivExpr::create(left, right, state.roundingMode);
    bindLocal(ki, state, result);
    break;
  }

  case Instruction::FNeg: {
    ref<Expr> expr = eval(ki, 0, state).value;
    bindLocal(ki, state, FNegExpr::create(expr));
    break;
  }

  case Instruction::FRem: {
    ref<Expr> left = eval(ki, 0, state).value;
    ref<Expr> right = eval(ki, 1, state).value;
    if (!fpWidthToSemantics(left->getWidth()) ||
        !fpWidthToSemantics(right->getWidth()))
      return terminateStateOnExecError(state, "Unsupported FRem operation");
    ref<Expr> result = FRemExpr::create(left, right, state.roundingMode);
    bindLocal(ki, state, result);
    break;
  }

  case Instruction::FPTrunc: {
    FPTruncInst *fi = cast<FPTruncInst>(i);
    Expr::Width resultType = getWidthForLLVMType(fi->getType());
    ref<Expr> arg = eval(ki, 0, state).value;
    if (!fpWidthToSemantics(arg->getWidth()) || !fpWidthToSemantics(resultType))
      return terminateStateOnExecError(state, "Unsupported FPTrunc operation");
    ref<Expr> result = arg;
    if (arg->getWidth() > resultType)
      result = FPTruncExpr::create(arg, resultType, state.roundingMode);
    if (X86FPAsX87FP80 && Context::get().getPointerWidth() == 32) {
      result = FPToX87FP80Ext(result);
    }
    bindLocal(ki, state, result);
    break;
  }

  case Instruction::FPExt: {
    FPExtInst *fi = cast<FPExtInst>(i);
    Expr::Width resultType = getWidthForLLVMType(fi->getType());
    ref<Expr> arg = eval(ki, 0, state).value;
    if (!fpWidthToSemantics(arg->getWidth()) || !fpWidthToSemantics(resultType))
      return terminateStateOnExecError(state, "Unsupported FPExt operation");
    ref<Expr> result = arg;
    if (arg->getWidth() < resultType) {
      // return terminateStateOnExecError(state, "Invalid FPExt");
      result = FPExtExpr::create(arg, resultType);
    }
    if (X86FPAsX87FP80 && Context::get().getPointerWidth() == 32) {
      result = FPToX87FP80Ext(result);
    }
    bindLocal(ki, state, result);
    break;
  }

  case Instruction::FPToUI: {
    FPToUIInst *fi = cast<FPToUIInst>(i);
    Expr::Width resultType = getWidthForLLVMType(fi->getType());
    ref<Expr> arg = eval(ki, 0, state).value;
    if (X86FPAsX87FP80 && Context::get().getPointerWidth() == 32) {
      arg = X87FP80ToFPTrunc(arg,
                             getWidthForLLVMType(fi->getOperand(0)->getType()),
                             state.roundingMode);
    }
    if (!fpWidthToSemantics(arg->getWidth()))
      return terminateStateOnExecError(state, "Unsupported FPToUI operation");
    // LLVM IR Ref manual says that it rounds toward zero
    ref<Expr> result =
        FPToUIExpr::create(arg, resultType, llvm::APFloat::rmTowardZero);
    bindLocal(ki, state, result);
    break;
  }

  case Instruction::FPToSI: {
    FPToSIInst *fi = cast<FPToSIInst>(i);
    Expr::Width resultType = getWidthForLLVMType(fi->getType());
    ref<Expr> arg = eval(ki, 0, state).value;
    if (X86FPAsX87FP80 && Context::get().getPointerWidth() == 32) {
      arg = X87FP80ToFPTrunc(arg,
                             getWidthForLLVMType(fi->getOperand(0)->getType()),
                             state.roundingMode);
    }
    if (!fpWidthToSemantics(arg->getWidth()))
      return terminateStateOnExecError(state, "Unsupported FPToSI operation");
    // LLVM IR Ref manual says that it rounds toward zero
    ref<Expr> result =
        FPToSIExpr::create(arg, resultType, llvm::APFloat::rmTowardZero);
    bindLocal(ki, state, result);
    break;
  }

  case Instruction::UIToFP: {
    UIToFPInst *fi = cast<UIToFPInst>(i);
    Expr::Width resultType = getWidthForLLVMType(fi->getType());
    if (X86FPAsX87FP80 && Context::get().getPointerWidth() == 32) {
      resultType = Expr::Fl80;
    }
    ref<Expr> arg = eval(ki, 0, state).value;
    const llvm::fltSemantics *semantics = fpWidthToSemantics(resultType);
    if (!semantics)
      return terminateStateOnExecError(state, "Unsupported UIToFP operation");
    ref<Expr> result = UIToFPExpr::create(arg, resultType, state.roundingMode);
    bindLocal(ki, state, result);
    break;
  }

  case Instruction::SIToFP: {
    SIToFPInst *fi = cast<SIToFPInst>(i);
    Expr::Width resultType = getWidthForLLVMType(fi->getType());
    if (X86FPAsX87FP80 && Context::get().getPointerWidth() == 32) {
      resultType = Expr::Fl80;
    }
    ref<Expr> arg = eval(ki, 0, state).value;
    const llvm::fltSemantics *semantics = fpWidthToSemantics(resultType);
    if (!semantics)
      return terminateStateOnExecError(state, "Unsupported SIToFP operation");
    ref<Expr> result = SIToFPExpr::create(arg, resultType, state.roundingMode);
    bindLocal(ki, state, result);
    break;
  }

  case Instruction::FCmp: {
    FCmpInst *fi = cast<FCmpInst>(i);
    ref<Expr> left = eval(ki, 0, state).value;
    ref<Expr> right = eval(ki, 1, state).value;
    if (!fpWidthToSemantics(left->getWidth()) ||
        !fpWidthToSemantics(right->getWidth()))
      return terminateStateOnExecError(state, "Unsupported FCmp operation");
    ref<Expr> result = evaluateFCmp(fi->getPredicate(), left, right);
    bindLocal(ki, state, result);
    break;
  }
#endif // ENABLE_FP

  case Instruction::InsertValue: {
    KGEPInstruction *kgepi = static_cast<KGEPInstruction *>(ki);

    ref<Expr> agg = eval(ki, 0, state).value;
    ref<Expr> val = eval(ki, 1, state).value;

    ref<Expr> l = NULL, r = NULL;
    unsigned lOffset = kgepi->offset * 8,
             rOffset = kgepi->offset * 8 + val->getWidth();

    if (lOffset > 0)
      l = ExtractExpr::create(agg, 0, lOffset);
    if (rOffset < agg->getWidth())
      r = ExtractExpr::create(agg, rOffset, agg->getWidth() - rOffset);

    ref<Expr> result;
    if (l && r)
      result = ConcatExpr::create(r, ConcatExpr::create(val, l));
    else if (l)
      result = ConcatExpr::create(val, l);
    else if (r)
      result = ConcatExpr::create(r, val);
    else
      result = val;

    bindLocal(ki, state, result);
    break;
  }
  case Instruction::ExtractValue: {
    KGEPInstruction *kgepi = static_cast<KGEPInstruction *>(ki);

    ref<Expr> agg = eval(ki, 0, state).value;

    ref<Expr> result = ExtractExpr::create(agg, kgepi->offset * 8,
                                           getWidthForLLVMType(i->getType()));

    bindLocal(ki, state, result);
    break;
  }
  case Instruction::Fence: {
    // Ignore for now
    break;
  }
  case Instruction::InsertElement: {
    InsertElementInst *iei = cast<InsertElementInst>(i);
    ref<Expr> vec = eval(ki, 0, state).value;
    ref<Expr> newElt = eval(ki, 1, state).value;
    ref<Expr> idx = eval(ki, 2, state).value;

    ConstantExpr *cIdx = dyn_cast<ConstantExpr>(idx);
    if (cIdx == NULL) {
      terminateStateOnExecError(
          state, "InsertElement, support for symbolic index not implemented");
      return;
    }
    uint64_t iIdx = cIdx->getZExtValue();
#if LLVM_VERSION_MAJOR >= 11
    const auto *vt = cast<llvm::FixedVectorType>(iei->getType());
#else
    const llvm::VectorType *vt = iei->getType();
#endif
    unsigned EltBits = getWidthForLLVMType(vt->getElementType());

    if (iIdx >= vt->getNumElements()) {
      // Out of bounds write
      terminateStateOnProgramError(
          state, new ErrorEvent(locationOf(state),
                                StateTerminationType::BadVectorAccess,
                                "Out of bounds write when inserting element"));
      return;
    }

    const unsigned elementCount = vt->getNumElements();
    llvm::SmallVector<ref<Expr>, 8> elems;
    elems.reserve(elementCount);
    for (unsigned i = elementCount; i != 0; --i) {
      auto of = i - 1;
      unsigned bitOffset = EltBits * of;
      elems.push_back(
          of == iIdx ? newElt : ExtractExpr::create(vec, bitOffset, EltBits));
    }

    assert(Context::get().isLittleEndian() && "FIXME:Broken for big endian");
    ref<Expr> Result = ConcatExpr::createN(elementCount, elems.data());
    bindLocal(ki, state, Result);
    break;
  }
  case Instruction::ExtractElement: {
    ExtractElementInst *eei = cast<ExtractElementInst>(i);
    ref<Expr> vec = eval(ki, 0, state).value;
    ref<Expr> idx = eval(ki, 1, state).value;

    ConstantExpr *cIdx = dyn_cast<ConstantExpr>(idx);
    if (cIdx == NULL) {
      terminateStateOnExecError(
          state, "ExtractElement, support for symbolic index not implemented");
      return;
    }
    uint64_t iIdx = cIdx->getZExtValue();
#if LLVM_VERSION_MAJOR >= 11
    const auto *vt = cast<llvm::FixedVectorType>(eei->getVectorOperandType());
#else
    const llvm::VectorType *vt = eei->getVectorOperandType();
#endif
    unsigned EltBits = getWidthForLLVMType(vt->getElementType());

    if (iIdx >= vt->getNumElements()) {
      // Out of bounds read
      terminateStateOnProgramError(
          state, new ErrorEvent(locationOf(state),
                                StateTerminationType::BadVectorAccess,
                                "Out of bounds read when extracting element"));
      return;
    }

    unsigned bitOffset = EltBits * iIdx;
    ref<Expr> Result = ExtractExpr::create(vec, bitOffset, EltBits);
    bindLocal(ki, state, Result);
    break;
  }
  case Instruction::ShuffleVector:
    // Should never happen due to Scalarizer pass removing ShuffleVector
    // instructions.
    terminateStateOnExecError(state, "Unexpected ShuffleVector instruction");
    break;

#ifdef SUPPORT_KLEE_EH_CXX
  case Instruction::Resume: {
    auto *cui = dyn_cast_or_null<CleanupPhaseUnwindingInformation>(
        state.unwindingInformation.get());

    if (!cui) {
      terminateStateOnExecError(
          state,
          "resume-instruction executed outside of cleanup phase unwinding");
      break;
    }

    ref<Expr> arg = eval(ki, 0, state).value;
    ref<Expr> exceptionPointer = ExtractExpr::create(arg, 0, Expr::Int64);
    ref<Expr> selectorValue =
        ExtractExpr::create(arg, Expr::Int64, Expr::Int32)->getValue();

    if (!dyn_cast<ConstantPointerExpr>(exceptionPointer) ||
        !dyn_cast<ConstantExpr>(selectorValue)) {
      terminateStateOnExecError(
          state, "resume-instruction called with non constant expression");
      break;
    }

    if (!Expr::createIsZero(selectorValue)->isTrue()) {
      klee_warning("resume-instruction called with non-0 selector value");
    }

    if (!EqExpr::create(exceptionPointer, cui->exceptionObject)->isTrue()) {
      terminateStateOnExecError(
          state, "resume-instruction called with unexpected exception pointer");
      break;
    }

    unwindToNextLandingpad(state);
    break;
  }

  case Instruction::LandingPad: {
    auto *cui = dyn_cast_or_null<CleanupPhaseUnwindingInformation>(
        state.unwindingInformation.get());

    if (!cui) {
      terminateStateOnExecError(
          state, "Executing landing pad but not in unwinding phase 2");
      break;
    }

    ref<ConstantPointerExpr> exceptionPointer = cui->exceptionObject;
    ref<ConstantExpr> selectorValue;

    // check on which frame we are currently
    if (state.stack.size() - 1 == cui->catchingStackIndex) {
      // we are in the target stack frame, return the selector value
      // that was returned by the personality fn in phase 1 and stop
      // unwinding.
      selectorValue = cui->selectorValue;

      // stop unwinding by cleaning up our unwinding information.
      state.unwindingInformation.reset();

      // this would otherwise now be a dangling pointer
      cui = nullptr;
    } else {
      // we are not yet at the target stack frame. the landingpad might have
      // a cleanup clause or not, anyway, we give it the selector value "0",
      // which represents a cleanup, and expect it to handle it.
      // This is explicitly allowed by LLVM, see
      // https://llvm.org/docs/ExceptionHandling.html#id18
      selectorValue = ConstantExpr::create(0, Expr::Int32);
    }

    // we have to return a {i8*, i32}
    ref<Expr> result = ConcatExpr::create(
        ZExtExpr::create(selectorValue, Expr::Int32), exceptionPointer);

    bindLocal(ki, state, result);

    break;
  }
#endif // SUPPORT_KLEE_EH_CXX

  case Instruction::AtomicRMW:
    terminateStateOnExecError(state, "Unexpected Atomic instruction, should be "
                                     "lowered by LowerAtomicInstructionPass");
    break;
  case Instruction::AtomicCmpXchg:
    terminateStateOnExecError(state,
                              "Unexpected AtomicCmpXchg instruction, should be "
                              "lowered by LowerAtomicInstructionPass");
    break;
  // Other instructions...
  // Unhandled
  default:
    terminateStateOnExecError(state, "illegal instruction");
    break;
  }
}

template <typename TypeIt>
void Executor::computeOffsetsSeqTy(KGEPInstruction *kgepi,
                                   ref<ConstantExpr> &constantOffset,
                                   uint64_t index, const TypeIt it) {
  assert(it->getNumContainedTypes() == 1 &&
         "Sequential type must contain one subtype");
  uint64_t elementSize =
      kmodule->targetData->getTypeStoreSize(it->getContainedType(0));
  const Value *operand = it.getOperand();
  if (const Constant *c = dyn_cast<Constant>(operand)) {
    auto expr = evalConstant(c, llvm::APFloat::rmNearestTiesToEven);
    ref<ConstantExpr> index = nullptr;
    if (expr->getKind() == Expr::Constant) {
      index = cast<ConstantExpr>(
                  evalConstant(c, llvm::APFloat::rmNearestTiesToEven))
                  ->SExt(Context::get().getPointerWidth());
    } else {
      assert(expr->getKind() == Expr::ConstantPointer);
      index = cast<ConstantExpr>(
                  cast<ConstantPointerExpr>(
                      evalConstant(c, llvm::APFloat::rmNearestTiesToEven))
                      ->getValue())
                  ->SExt(Context::get().getPointerWidth());
    }
    ref<ConstantExpr> addend = index->Mul(
        ConstantExpr::alloc(elementSize, Context::get().getPointerWidth()));
    constantOffset = constantOffset->Add(addend);
  } else {
    kgepi->indices.emplace_back(index, elementSize);
  }
}

template <typename TypeIt>
void Executor::computeOffsets(KGEPInstruction *kgepi, TypeIt ib, TypeIt ie) {
  ref<ConstantExpr> constantOffset =
      ConstantExpr::alloc(0, Context::get().getPointerWidth());
  uint64_t index = 1;
  for (TypeIt ii = ib; ii != ie; ++ii) {
    if (StructType *st = dyn_cast<StructType>(*ii)) {
      const StructLayout *sl = kmodule->targetData->getStructLayout(st);
      const ConstantInt *ci = cast<ConstantInt>(ii.getOperand());
      uint64_t addend = sl->getElementOffset((unsigned)ci->getZExtValue());
      constantOffset = constantOffset->Add(
          ConstantExpr::alloc(addend, Context::get().getPointerWidth()));
    } else if (ii->isArrayTy() || ii->isVectorTy() || ii->isPointerTy()) {
      computeOffsetsSeqTy(kgepi, constantOffset, index, ii);
    } else
      assert("invalid type" && 0);
    index++;
  }
  kgepi->offset = constantOffset->getZExtValue();
}

void Executor::bindInstructionConstants(KInstruction *KI) {
  if (GetElementPtrInst *gepi = dyn_cast<GetElementPtrInst>(KI->inst())) {
    KGEPInstruction *kgepi = static_cast<KGEPInstruction *>(KI);
    computeOffsets(kgepi, gep_type_begin(gepi), gep_type_end(gepi));
  } else if (InsertValueInst *ivi = dyn_cast<InsertValueInst>(KI->inst())) {
    KGEPInstruction *kgepi = static_cast<KGEPInstruction *>(KI);
    computeOffsets(kgepi, iv_type_begin(ivi), iv_type_end(ivi));
    assert(kgepi->indices.empty() && "InsertValue constant offset expected");
  } else if (ExtractValueInst *evi = dyn_cast<ExtractValueInst>(KI->inst())) {
    KGEPInstruction *kgepi = static_cast<KGEPInstruction *>(KI);
    computeOffsets(kgepi, ev_type_begin(evi), ev_type_end(evi));
    assert(kgepi->indices.empty() && "ExtractValue constant offset expected");
  }
}

void Executor::bindModuleConstants(llvm::APFloat::roundingMode rm) {
  for (auto &kfp : kmodule->functions) {
    KFunction *kf = kfp.get();
    for (unsigned i = 0; i < kf->numInstructions; ++i)
      bindInstructionConstants(kf->instructions[i]);
  }

  kmodule->constantTable =
      std::unique_ptr<Cell[]>(new Cell[kmodule->constants.size()]);
  for (unsigned i = 0; i < kmodule->constants.size(); ++i) {
    Cell &c = kmodule->constantTable[i];
    c.value = evalConstant(kmodule->constants[i], rm);
  }
}

bool Executor::checkMemoryUsage() {
  if (!MaxMemory)
    return true;

  // We need to avoid calling GetTotalMallocUsage() often because it
  // is O(elts on freelist). This is really bad since we start
  // to pummel the freelist once we hit the memory cap.
  // every 65536 instructions
  if ((stats::instructions & 0xFFFFU) != 0 &&
      maxNewWriteableOSSize < OSCopySizeMemoryCheckThreshold &&

      maxNewStateStackSize < StackCopySizeMemoryCheckThreshold)
    return true;

  // check memory limit
  const auto mallocUsage = util::GetTotalMallocUsage() >> 20U;
  const auto mmapUsage = memory->getUsedDeterministicSize() >> 20U;
  const auto totalUsage = mallocUsage + mmapUsage;

  if (MemoryTriggerCoverOnTheFly && totalUsage > MaxMemory * 0.75) {
    klee_warning_once(0,
                      "enabling cover-on-the-fly (close to memory cap: %luMB)",
                      totalUsage);
    coverOnTheFly = true;
  }

  atMemoryLimit = totalUsage > MaxMemory; // inhibit forking
  if (!atMemoryLimit)
    return true;

  // only terminate states when threshold (+100MB) exceeded
  if (totalUsage <= MaxMemory + 100)
    return true;

  // just guess at how many to kill
  auto states = objectManager->getStates();
  const auto numStates = states.size();
  auto toKill = std::max(1UL, numStates - numStates * MaxMemory / totalUsage);
  klee_warning("killing %lu states (over memory cap: %luMB)", toKill,
               totalUsage);

  // randomly select states for early termination
  std::vector<ExecutionState *> arr(states.begin(),
                                    states.end()); // FIXME: expensive
  for (unsigned i = 0, N = arr.size(); N && i < toKill; ++i, --N) {
    unsigned idx = theRNG.getInt32() % N;
    // Make two pulls to try and not hit a state that
    // covered new code.
    if (arr[idx]->isCoveredNew())
      idx = theRNG.getInt32() % N;

    std::swap(arr[idx], arr[N - 1]);
    terminateStateEarly(*arr[N - 1], "Memory limit exceeded.",
                        StateTerminationType::OutOfMemory);
  }

  return false;
}

void Executor::decreaseConfidenceFromStoppedStates(
    const SetOfStates &leftStates, HaltExecution::Reason reason) {
  if (targets.size() == 0) {
    return;
  }
  for (auto state : leftStates) {
    if (state->targetForest.empty())
      continue;
    hasStateWhichCanReachSomeTarget = true;
    auto realReason = reason ? reason : state->terminationReasonType.load();
    if (state->progressVelocity >= 0) {
      assert(targets.count(state->targetForest.getEntryFunction()) != 0);
      targets.at(state->targetForest.getEntryFunction())
          .subtractConfidencesFrom(state->targetForest, realReason);
    }
  }
}

void Executor::doDumpStates() {
  auto &states = objectManager->getStates();
  if (DumpStatesOnHalt == HaltExecution::Reason::NotHalt || states.empty()) {
    interpreterHandler->incPathsExplored(states.size());
    return;
  }

  klee_message("halting execution, dumping remaining states");
  for (const auto &state : objectManager->getStates()) {
    terminateStateEarly(*state, "Execution halting.",
                        StateTerminationType::Interrupted);
  }
  objectManager->updateSubscribers();
}

const KInstruction *Executor::getKInst(const llvm::Instruction *inst) const {
  const llvm::Function *caller = inst->getFunction();
  KFunction *kf = kmodule->functionMap.at(caller);
  assert(kf && kf->instructionMap.find(inst) != kf->instructionMap.end());
  return kf->instructionMap.at(inst);
}

const KBlock *Executor::getKBlock(const llvm::BasicBlock *bb) const {
  const llvm::Function *F = bb->getParent();
  assert(F && kmodule->functionMap.find(F) != kmodule->functionMap.end());
  klee::KFunction *KF = kmodule->functionMap.at(F);
  assert(KF->blockMap.find(bb) != KF->blockMap.end());
  const klee::KBlock *KB = KF->blockMap.at(bb);
  assert(KB);
  return KB;
}

const KFunction *Executor::getKFunction(const llvm::Function *f) const {
  const auto kfIt = kmodule->functionMap.find(const_cast<Function *>(f));
  return (kfIt == kmodule->functionMap.end()) ? nullptr : kfIt->second;
}

void Executor::seed(ExecutionState &initialState) {
  std::vector<SeedInfo> &v = seedMap->at(&initialState);

  for (std::vector<KTest *>::const_iterator it = usingSeeds->begin(),
                                            ie = usingSeeds->end();
       it != ie; ++it)
    v.push_back(SeedInfo(*it));

  int lastNumSeeds = usingSeeds->size() + 10;
  time::Point lastTime, startTime = lastTime = time::getWallTime();
  ExecutionState *lastState = 0;
  while (!seedMap->empty()) {
    if (haltExecution) {
      doDumpStates();
      return;
    }

    std::map<ExecutionState *, std::vector<SeedInfo>>::iterator it =
        seedMap->upper_bound(lastState);
    if (it == seedMap->end())
      it = seedMap->begin();
    lastState = it->first;
    ExecutionState &state = *lastState;
    KInstruction *ki = state.pc;
    objectManager->setCurrentState(&state);
    stepInstruction(state);

    executeInstruction(state, ki);
    timers.invoke();
    if (::dumpStates)
      dumpStates();
    if (::dumpPForest)
      dumpPForest();
    objectManager->updateSubscribers();

    if ((stats::instructions % 1000) == 0) {
      int numSeeds = 0, numStates = 0;
      for (std::map<ExecutionState *, std::vector<SeedInfo>>::iterator
               it = seedMap->begin(),
               ie = seedMap->end();
           it != ie; ++it) {
        numSeeds += it->second.size();
        numStates++;
      }
      const auto time = time::getWallTime();
      const time::Span seedTime(SeedTime);
      if (seedTime && time > startTime + seedTime) {
        klee_warning("seed time expired, %d seeds remain over %d states",
                     numSeeds, numStates);
        break;
      } else if (numSeeds <= lastNumSeeds - 10 ||
                 time - lastTime >= time::seconds(10)) {
        lastTime = time;
        lastNumSeeds = numSeeds;
        klee_message("%d seeds remaining over: %d states", numSeeds, numStates);
      }
    }
  }

  klee_message("seeding done (%d states remain)",
               (int)objectManager->getStates().size());

  if (OnlySeed) {
    doDumpStates();
    return;
  }
}

void Executor::reportProgressTowardsTargets(std::string prefix,
                                            const SetOfStates &states) const {
  klee_message("%zu %sstates remaining", states.size(), prefix.c_str());
  TargetHashMap<DistanceResult> distancesTowardsTargets;
  for (auto &state : states) {
    for (auto &p : *state->targetForest.getTopLayer()) {
      auto target = p.first;
      auto distance = targetManager->distance(*state, target);
      auto it = distancesTowardsTargets.find(target);
      if (it == distancesTowardsTargets.end())
        distancesTowardsTargets.insert(it, std::make_pair(target, distance));
      else {
        distance = std::min(distance, it->second);
        distancesTowardsTargets[target] = distance;
      }
    }
  }
  klee_message("Distances from nearest %sstates to remaining targets:",
               prefix.c_str());
  for (auto &p : distancesTowardsTargets) {
    auto target = p.first;
    auto distance = p.second;
    std::ostringstream repr;
    repr << "Target ";
    if (target->shouldFailOnThisTarget()) {
      repr << cast<ReproduceErrorTarget>(target)->getId() << ": error ";
    } else {
      repr << ": ";
    }
    repr << "in function " +
                target->getBlock()->parent->function()->getName().str();
    repr << " (lines ";
    repr << target->getBlock()->getFirstInstruction()->getLine();
    repr << " to ";
    repr << target->getBlock()->getLastInstruction()->getLine();
    repr << ")";
    std::string targetString = repr.str();
    klee_message("%s for %s", distance.toString().c_str(),
                 targetString.c_str());
  }
}

void Executor::reportProgressTowardsTargets() const {
  reportProgressTowardsTargets("", objectManager->getStates());
}

void Executor::run(ExecutionState *initialState) {
  // Delay init till now so that ticks don't accrue during optimization and
  // such.
  if (guidanceKind != GuidanceKind::ErrorGuidance)
    timers.reset();

  if (usingSeeds) {
    seed(*initialState);
  }

  searcher =
      std::make_unique<ForwardOnlySearcher>(constructUserSearcher(*this));

  if (targetManager) {
    objectManager->addSubscriber(targetManager.get());
  }

  if (targetedExecutionManager) {
    objectManager->addSubscriber(targetedExecutionManager.get());
  }

  if (targetCalculator) {
    objectManager->addSubscriber(targetCalculator.get());
  }

  objectManager->addSubscriber(searcher.get());

  objectManager->initialUpdate();

  // main interpreter loop
  while (!haltExecution && !searcher->empty()) {
    auto action = searcher->selectAction();
    executeAction(action);
    objectManager->updateSubscribers();

    if (!checkMemoryUsage()) {
      objectManager->updateSubscribers();
    }
  }

  if (guidanceKind == GuidanceKind::ErrorGuidance) {
    reportProgressTowardsTargets();
    decreaseConfidenceFromStoppedStates(objectManager->getStates(),
                                        haltExecution);

    for (auto &startBlockAndWhiteList : targets) {
      startBlockAndWhiteList.second.reportFalsePositives(
          hasStateWhichCanReachSomeTarget);
    }

    if (searcher->empty())
      haltExecution = HaltExecution::NoMoreStates;
  }

  doDumpStates();

  searcher = nullptr;
  targetManager = nullptr;

  haltExecution = HaltExecution::NotHalt;
}

void Executor::initializeTypeManager() {
  if (UseAdvancedTypeSystem) {
    typeSystemManager = new CXXTypeManager(kmodule.get());
  } else {
    typeSystemManager = new TypeManager(kmodule.get());
  }
  typeSystemManager->initModule();
}

static bool shouldWriteTest(const ExecutionState &state, bool isError = false) {
  state.updateCoveredNew();
  bool coveredNew = isError ? state.isCoveredNewError() : state.isCoveredNew();
  return !OnlyOutputStatesCoveringNew || coveredNew;
}

static std::string terminationTypeFileExtension(StateTerminationType type) {
  std::string ret;
#undef TTYPE
#undef TTMARK
#define TTYPE(N, I, S)                                                         \
  case StateTerminationType::N:                                                \
    ret = (S);                                                                 \
    break;
#define TTMARK(N, I)
  switch (type) { TERMINATION_TYPES }
  return ret;
};

static std::string terminationTypeName(StateTerminationType type) {
  std::string ret;
#undef TTYPE
#undef TTMARK
#define TTYPE(N, I, S)                                                         \
  case StateTerminationType::N:                                                \
    ret = #N;                                                                  \
    break;
#define TTMARK(N, I)

  switch (type) { TERMINATION_TYPES }
  return ret;
};

void Executor::executeAction(ref<SearcherAction> action) {
  switch (action->getKind()) {
  case SearcherAction::Kind::Forward: {
    goForward(cast<ForwardAction>(action));
    break;
  }
  }
  timers.invoke();
}

void Executor::goForward(ref<ForwardAction> action) {
  ref<ForwardAction> fa = cast<ForwardAction>(action);
  objectManager->setCurrentState(fa->state);
  ExecutionState &state = *fa->state;

  if (coverOnTheFly && shouldWriteTest(state)) {
    fa->state->clearCoveredNew();
    interpreterHandler->processTestCase(
        state, nullptr,
        terminationTypeFileExtension(StateTerminationType::CoverOnTheFly)
            .c_str());
  }

  if (targetManager) {
    targetManager->pullGlobal(state);
  }

  if (targetCalculator && TrackCoverage != TrackCoverageBy::None &&
      state.multiplexKF && functionsByModule.modules.size() > 1 &&
      targetCalculator->isCovered(state.multiplexKF)) {
    terminateStateEarly(state, "Multiplex function has been covered.",
                        StateTerminationType::CoveredEntryPoint);
  } else if (targetManager && targetManager->isTargeted(state) &&
             state.targets().empty()) {
    terminateStateEarlyAlgorithm(state, "State missed all it's targets.",
                                 StateTerminationType::MissedAllTargets);
  } else if (state.isCycled(MaxCycles)) {
    terminateStateEarly(state, "max-cycles exceeded.",
                        StateTerminationType::MaxCycles);
  } else if (state.isSymbolicCycled(MaxSymbolicCycles)) {
    terminateStateEarly(state, "max-sym-cycles exceeded.",
                        StateTerminationType::MaxCycles);
  } else {
    maxNewWriteableOSSize = 0;
    maxNewStateStackSize = 0;

    KInstruction *ki = state.pc;
    stepInstruction(state);
    executeInstruction(state, ki);
  }

  timers.invoke();
  if (::dumpStates)
    dumpStates();
  if (::dumpPForest)
    dumpPForest();

  if (targetCalculator && TrackCoverage != TrackCoverageBy::None &&
      targetCalculator->isCovered(state.initPC->parent->parent)) {
    haltExecution = HaltExecution::CovCheck;
  }
}

std::string Executor::getAddressInfo(ExecutionState &state,
                                     ref<PointerExpr> address, unsigned size,
                                     const MemoryObject *mo) const {
  std::string Str;
  llvm::raw_string_ostream info(Str);
  address =
      Simplificator::simplifyExpr(state.constraints.cs(), address).simplified;
  ref<Expr> base = address->getBase();
  ref<Expr> offset = address->getOffset();
  info << "\tbase: " << base << "\n";
  info << "\toffset: " << offset << "\n";
  if (size) {
    info << "\tsize: " << size << "\n";
  }

  uint64_t example;
  if (ref<ConstantExpr> CE = dyn_cast<ConstantExpr>(address->getValue())) {
    example = CE->getZExtValue();
  } else {
    ref<ConstantExpr> value;
    bool success = solver->getValue(state.constraints.cs(), address->getValue(),
                                    value, state.queryMetaData);
    assert(success && "FIXME: Unhandled solver failure");
    (void)success;
    example = value->getZExtValue();
    info << "\texample: " << example << "\n";
    if (mo) {
      std::pair<ref<Expr>, ref<Expr>> res =
          std::make_pair(mo->getBaseExpr(),
                         AddExpr::create(mo->getBaseExpr(), mo->getSizeExpr()));
      info << "\trange: [" << res.first << ", " << res.second << "]\n";
    }
  }

  MemoryObject hack((unsigned)example);
  MemoryMap::iterator lower = state.addressSpace.objects.upper_bound(&hack);
  info << "\tnext: ";
  if (lower == state.addressSpace.objects.end()) {
    info << "none\n";
  } else {
    const MemoryObject *mo = lower->first;
    std::string alloc_info = mo->getAllocInfo();
    info << "object at ";
    mo->getBaseExpr()->print(info);
    info << " of size ";
    mo->getSizeExpr()->print(info);
    info << "\n"
         << "\t\t" << alloc_info << "\n";
  }
  if (lower != state.addressSpace.objects.begin()) {
    --lower;
    info << "\tprev: ";
    if (lower == state.addressSpace.objects.end()) {
      info << "none\n";
    } else {
      const MemoryObject *mo = lower->first;
      std::string alloc_info = mo->getAllocInfo();
      info << "object at ";
      mo->getBaseExpr()->print(info);
      info << " of size ";
      mo->getSizeExpr()->print(info);
      info << "\n"
           << "\t\t" << alloc_info << "\n";
    }
  }

  info.flush();
  return Str;
}

HaltExecution::Reason fromStateTerminationType(StateTerminationType t) {
  switch (t) {
  case StateTerminationType::MaxDepth:
    return HaltExecution::MaxDepth;
  case StateTerminationType::OutOfStackMemory:
    return HaltExecution::MaxStackFrames;
  case StateTerminationType::Solver:
    return HaltExecution::MaxSolverTime;
  case StateTerminationType::MaxCycles:
    return HaltExecution::MaxCycles;
  case StateTerminationType::Interrupted:
    return HaltExecution::Interrupt;
  default:
    return HaltExecution::Unspecified;
  }
}

void Executor::terminateState(ExecutionState &state,
                              StateTerminationType terminationType) {
  state.terminationReasonType = fromStateTerminationType(terminationType);
  if (terminationType >= StateTerminationType::MaxDepth &&
      terminationType <= StateTerminationType::EARLY) {
    SetOfStates states = {&state};
    decreaseConfidenceFromStoppedStates(states, state.terminationReasonType);
  }
  if (replayKTest && replayPosition != replayKTest->numObjects) {
    klee_warning_once(replayKTest,
                      "replay did not consume all objects in test input.");
  }

  interpreterHandler->incPathsExplored();
  state.pc = state.prevPC;
  solver->notifyStateTermination(state.id);

  objectManager->removeState(&state);
}

void Executor::terminateStateOnExit(ExecutionState &state) {
  auto terminationType = StateTerminationType::Exit;
  ++stats::terminationExit;
  if (shouldWriteTest(state) || (AlwaysOutputSeeds && seedMap->count(&state))) {
    state.clearCoveredNew();
    interpreterHandler->processTestCase(
        state, nullptr, terminationTypeFileExtension(terminationType).c_str());
  }

  interpreterHandler->incPathsCompleted();
  terminateState(state, terminationType);
}

void Executor::terminateStateEarly(ExecutionState &state, const Twine &message,
                                   StateTerminationType reason) {
  if (reason <= StateTerminationType::EARLY) {
    assert(reason > StateTerminationType::EXIT);
    ++stats::terminationEarly;
  }

  if (((reason <= StateTerminationType::EARLY ||
        reason == StateTerminationType::MissedAllTargets) &&
       shouldWriteTest(state)) ||
      (AlwaysOutputSeeds && seedMap->count(&state))) {
    state.clearCoveredNew();
    interpreterHandler->processTestCase(
        state, (message + "\n").str().c_str(),
        terminationTypeFileExtension(reason).c_str(),
        reason > StateTerminationType::EARLY &&
            reason <= StateTerminationType::EXECERR);
  }
  terminateState(state, reason);
}

void Executor::terminateStateEarlyAlgorithm(ExecutionState &state,
                                            const llvm::Twine &message,
                                            StateTerminationType reason) {
  assert(reason > StateTerminationType::EXECERR &&
         reason <= StateTerminationType::EARLYALGORITHM);
  ++stats::terminationEarlyAlgorithm;
  terminateStateEarly(state, message, reason);
}

void Executor::terminateStateEarlyUser(ExecutionState &state,
                                       const llvm::Twine &message) {
  ++stats::terminationEarlyUser;
  terminateStateEarly(state, message, StateTerminationType::SilentExit);
}

const KInstruction *
Executor::getLastNonKleeInternalInstruction(const ExecutionState &state) const {
  // unroll the stack of the applications state and find
  // the last instruction which is not inside a KLEE internal function
  auto it = state.stack.callStack().rbegin();
  auto itE = state.stack.callStack().rend();

  // don't check beyond the outermost function (i.e. main())
  itE--;

  const KInstruction *ki = nullptr;
  if (kmodule->internalFunctions.count(it->kf->function()) == 0) {
    ki = state.prevPC;
    //  Cannot return yet because even though
    //  it->function() is not an internal function it might of
    //  been called from an internal function.
  }

  // Wind up the stack and check if we are in a KLEE internal function.
  // We visit the entire stack because we want to return a CallInstruction
  // that was not reached via any KLEE internal functions.
  for (; it != itE; ++it) {
    // check calling instruction and if it is contained in a KLEE internal
    // function
    const Function *f = (*it->caller).inst()->getParent()->getParent();
    if (kmodule->internalFunctions.count(f)) {
      ki = nullptr;
      continue;
    }
    if (!ki) {
      ki = it->caller;
    }
  }

  if (!ki) {
    // something went wrong, play safe and return the current instruction info
    return state.prevPC;
  }
  return ki;
}

bool shouldExitOn(StateTerminationType reason) {
  auto it = std::find(ExitOnErrorType.begin(), ExitOnErrorType.end(), reason);
  return it != ExitOnErrorType.end();
}

void Executor::reportStateOnTargetError(ExecutionState &state,
                                        ReachWithError error) {
  if (guidanceKind == GuidanceKind::ErrorGuidance) {
    bool reportedTruePositive =
        targetedExecutionManager->reportTruePositive(state, error);
    if (!reportedTruePositive) {
      targetedExecutionManager->reportFalseNegative(state, error);
    }
  }
}

void Executor::terminateStateOnTargetError(ExecutionState &state,
                                           ReachWithError error) {
  reportStateOnTargetError(state, error);

  // Proceed with normal `terminateStateOnError` call
  std::string messaget;
  StateTerminationType terminationType;
  switch (error) {
  case ReachWithError::MayBeNullPointerException:
  case ReachWithError::MustBeNullPointerException:
    messaget = "memory error: null pointer exception";
    terminationType = StateTerminationType::Ptr;
    break;
  case ReachWithError::DoubleFree:
    messaget = "double free error";
    terminationType = StateTerminationType::Ptr;
    break;
  case ReachWithError::UseAfterFree:
    messaget = "use after free error";
    terminationType = StateTerminationType::Ptr;
    break;
  case ReachWithError::Reachable:
    messaget = "";
    terminationType = StateTerminationType::Reachable;
    break;
  case ReachWithError::None:
  default:
    messaget = "unspecified error";
    terminationType = StateTerminationType::User;
  }
  terminateStateOnProgramError(
      state, new ErrorEvent(locationOf(state), terminationType, messaget));
}

void Executor::terminateStateOnError(ExecutionState &state,
                                     const llvm::Twine &messaget,
                                     StateTerminationType terminationType,
                                     const llvm::Twine &info,
                                     const char *suffix) {
  std::string message = messaget.str();
  static std::set<std::pair<Instruction *, std::string>> emittedErrors;
  const KInstruction *ki = getLastNonKleeInternalInstruction(state);
  Instruction *lastInst = ki->inst();

  if ((EmitAllErrors ||
       emittedErrors.insert(std::make_pair(lastInst, message)).second) &&
      shouldWriteTest(state, true)) {
    std::string filepath = ki->getSourceFilepath();
    if (!filepath.empty()) {
      klee_message("ERROR: %s:%zu: %s", filepath.c_str(), ki->getLine(),
                   message.c_str());
    } else {
      klee_message("ERROR: (location information missing) %s", message.c_str());
    }
    if (!EmitAllErrors)
      klee_message("NOTE: now ignoring this error at this location");

    std::string MsgString;
    llvm::raw_string_ostream msg(MsgString);
    msg << "Error: " << message << '\n';
    if (!filepath.empty()) {
      msg << "File: " << filepath << '\n' << "Line: " << ki->getLine() << '\n';
      {
        auto asmLine = ki->getKModule()->getAsmLine(ki->inst());
        if (asmLine.has_value()) {
          msg << "assembly.ll line: " << asmLine.value() << '\n';
        }
      }
      msg << "State: " << state.getID() << '\n';
    }
    msg << "ExecutionStack: \n";
    state.dumpStack(msg);

    std::string info_str = info.str();
    if (!info_str.empty())
      msg << "Info: \n" << info_str;

    state.clearCoveredNewError();

    const std::string ext = terminationTypeFileExtension(terminationType);
    // use user provided suffix from klee_report_error()
    const char *file_suffix = suffix ? suffix : ext.c_str();
    interpreterHandler->processTestCase(state, msg.str().c_str(), file_suffix,
                                        true);
  }

  terminateState(state, terminationType);

  if (shouldExitOn(terminationType))
    haltExecution = HaltExecution::ErrorOnWhichShouldExit;
}

void Executor::terminateStateOnExecError(ExecutionState &state,
                                         const llvm::Twine &message,
                                         StateTerminationType reason) {
  assert(reason > StateTerminationType::USERERR &&
         reason <= StateTerminationType::EXECERR);
  ++stats::terminationExecutionError;
  terminateStateOnError(state, message, reason, "");
}

void Executor::terminateStateOnProgramError(ExecutionState &state,
                                            const ref<ErrorEvent> &reason,
                                            const llvm::Twine &info,
                                            const char *suffix) {
  assert(reason->ruleID > StateTerminationType::SOLVERERR &&
         reason->ruleID <= StateTerminationType::PROGERR);
  ++stats::terminationProgramError;

  if (reason->source.has_value()) {
    const ref<CodeEvent> &sourceEvent = reason->source.value();
    if (sourceEvent->location) {
      EventRecorder traceRecorder;

      traceRecorder.record(sourceEvent);
      traceRecorder.append(
          state.eventsRecorder.tail(sourceEvent->location->pathIndex));

      state.eventsRecorder = traceRecorder;
    }
  }
  state.eventsRecorder.record(reason);

  terminateStateOnError(state, reason->message, reason->ruleID, info, suffix);
}

void Executor::terminateStateOnSolverError(ExecutionState &state,
                                           const llvm::Twine &message) {
  ++stats::terminationSolverError;
  terminateStateOnError(state, message, StateTerminationType::Solver, "");
}

void Executor::terminateStateOnUserError(ExecutionState &state,
                                         const llvm::Twine &message) {
  ++stats::terminationUserError;
  terminateStateOnError(state, message, StateTerminationType::User, "");
}

void Executor::callExternalFunction(ExecutionState &state, KInstruction *target,
                                    KCallable *callable,
                                    std::vector<ref<Expr>> &arguments) {
  // check if specialFunctionHandler wants it
  if (const auto *func = dyn_cast<KFunction>(callable)) {
    if (func->getName() == "raise") {
      terminateStateOnError(state, "Call to \"raise\" is unmodelled",
                            StateTerminationType::Model);
      return;
    }

    if (specialFunctionHandler->handle(state, func->function(), target,
                                       arguments))
      return;
  }

  if (ExternalCalls == ExternalCallPolicy::None &&
      !okExternals.count(callable->getName().str())) {
    klee_warning("Disallowed call to external function: %s\n",
                 callable->getName().str().c_str());
    terminateStateOnUserError(state, "external calls disallowed");
    return;
  }

  // normal external function handling path
  // allocate 512 bits for each argument (+return value) to support
  // fp80's and SIMD vectors as parameters for external calls;
  // we could iterate through all the arguments first and determine the exact
  // size we need, but this is faster, and the memory usage isn't significant.
  size_t allocatedBytes = Expr::MaxWidth / 8 * (arguments.size() + 1);
  uint64_t *args = (uint64_t *)alloca(allocatedBytes);
  memset(args, 0, allocatedBytes);
  unsigned wordIndex = 2;

  /* To check types we iterate over types of parameteres in function
  signature. Notice, that number of them can differ from passed,
  as function can have variadic arguments. */
  llvm::FunctionType *functionType = callable->getFunctionType();

  if (arguments.size() > 0) {
    ref<SolverResponse> response;
    bool success =
        solver->getResponse(state.constraints.cs(), Expr::createFalse(),
                            response, state.queryMetaData);
    assert(success && isa<InvalidResponse>(response) &&
           "FIXME: Unhandled solver failure");
    (void)success;
    Assignment model = cast<InvalidResponse>(response)->initialValues();
    AssignmentEvaluator evaluator(model, false);
    llvm::FunctionType::param_iterator ati = functionType->param_begin();
    for (std::vector<ref<Expr>>::iterator ai = arguments.begin(),
                                          ae = arguments.end();
         ai != ae; ++ai) {
      if (ExternalCalls ==
          ExternalCallPolicy::All) { // don't bother checking uniqueness
        ref<Expr> arg = *ai;
        if (auto pointer = dyn_cast<PointerExpr>(arg)) {
          ref<ConstantExpr> base = evaluator.visit(pointer->getBase());
          ref<ConstantExpr> value = evaluator.visit(pointer->getValue());
          value->toMemory(&args[wordIndex]);
          ref<ConstantPointerExpr> const_pointer =
              ConstantPointerExpr::create(base, value);
          ObjectPair op;
          if (state.addressSpace.resolveOne(const_pointer, nullptr, op) &&
              !op.second->readOnly) {
            auto *os = state.addressSpace.getWriteable(op.first, op.second);
            os->flushToConcreteStore(model);
          }
          addConstraint(state, EqExpr::create(const_pointer->getValue(), arg));
          wordIndex += (value->getWidth() + 63) / 64;
        } else {
          arg = optimizer.optimizeExpr(arg, true);
          ref<ConstantExpr> ce = evaluator.visit(arg);
          ce->toMemory(&args[wordIndex]);
          addConstraint(state, EqExpr::create(ce, arg));
          wordIndex += (ce->getWidth() + 63) / 64;
        }
      } else {
        ref<Expr> arg = toUnique(state, *ai);
        if (ConstantExpr *ce = dyn_cast<ConstantExpr>(arg)) {
          // fp80 must be aligned to 16 according to the System V AMD 64 ABI
          if (ce->getWidth() == Expr::Fl80 && wordIndex & 0x01)
            wordIndex++;

          // XXX kick toMemory functions from here
          ce->toMemory(&args[wordIndex]);
          wordIndex += (ce->getWidth() + 63) / 64;
        } else if (ConstantPointerExpr *cpe =
                       dyn_cast<ConstantPointerExpr>(arg)) {
          ref<ConstantExpr> ce = cpe->getConstantValue();
          // XXX kick toMemory functions from here
          ce->toMemory(&args[wordIndex]);
          wordIndex += (ce->getWidth() + 63) / 64;
        } else {
          terminateStateOnExecError(state,
                                    "external call with symbolic argument: " +
                                        callable->getName());
          return;
        }
      }
      if (ati != functionType->param_end()) {
        ++ati;
      }
    }
  }

  // Prepare external memory for invoking the function
  auto arrays = state.constraints.cs().gatherArrays();
  std::vector<SparseStorageImpl<unsigned char>> values;
  solver->getInitialValues(state.constraints.cs(), arrays, values,
                           state.queryMetaData);
  Assignment assignment(arrays, values);
  state.addressSpace.copyOutConcretes(assignment);
#ifndef WINDOWS
  // Update external errno state with local state value
  ObjectPair result;

  llvm::Type *pointerErrnoAddr = llvm::PointerType::get(
      llvm::IntegerType::get(kmodule->module->getContext(),
                             sizeof(*errno_addr) * CHAR_BIT),
      kmodule->targetData->getAllocaAddrSpace());

  bool resolved = state.addressSpace.resolveOne(
      ConstantPointerExpr::create(Expr::createPointer((uint64_t)errno_addr),
                                  Expr::createPointer((uint64_t)errno_addr)),
      typeSystemManager->getWrappedType(pointerErrnoAddr), result);
  if (!resolved)
    klee_error("Could not resolve memory object for errno");
  ref<Expr> errValueExpr = result.second->read(0, sizeof(*errno_addr) * 8);
  errValueExpr = toUnique(state, errValueExpr);
  ConstantExpr *errnoValue =
      dyn_cast<ConstantExpr>(makePointer(errValueExpr)->getValue());
  if (!errnoValue) {
    terminateStateOnExecError(state,
                              "external call with errno value symbolic: " +
                                  callable->getName());
    return;
  }

  externalDispatcher->setLastErrno(
      errnoValue->getZExtValue(sizeof(*errno_addr) * 8));
#endif

  if (!SuppressExternalWarnings) {

    std::string TmpStr;
    llvm::raw_string_ostream os(TmpStr);
    os << "calling external: " << callable->getName().str() << "(";
    for (unsigned i = 0; i < arguments.size(); i++) {
      os << arguments[i];
      if (i != arguments.size() - 1)
        os << ", ";
    }
    os << ") at " << state.pc->getSourceLocationString();

    if (AllExternalWarnings)
      klee_warning("%s", os.str().c_str());
    else
      klee_warning_once(callable->unwrap(), "%s", os.str().c_str());
  }

  int roundingMode = LLVMRoundingModeToCRoundingMode(state.roundingMode);
  if (roundingMode == -1) {
    std::string msg("Cannot set rounding mode for external call to ");
    msg += LLVMRoundingModeToString(state.roundingMode);
    terminateStateOnExecError(state, msg, StateTerminationType::External);
    return;
  }

  bool success = externalDispatcher->executeCall(callable, target->inst(), args,
                                                 roundingMode);

  if (!success) {
    if (interpreterOpts.Mock == MockPolicy::Failed) {
      if (target->inst()->getType()->isSized()) {
        prepareMockValue(state, "mockExternResult", target->inst()->getType(),
                         target);
      }
    } else {
      terminateStateOnExecError(state,
                                "failed external call: " + callable->getName(),
                                StateTerminationType::External);
    }
    return;
  }

  if (!state.addressSpace.copyInConcretes(assignment)) {
    terminateStateOnExecError(state, "external modified read-only object",
                              StateTerminationType::External);
    return;
  }

#ifndef WINDOWS
  // Update errno memory object with the errno value from the call
  int error = externalDispatcher->getLastErrno();
  state.addressSpace.copyInConcrete(result.first, result.second,
                                    (uint64_t)&error, assignment);
#endif

  Type *resultType = target->inst()->getType();
  if (resultType != Type::getVoidTy(kmodule->module->getContext())) {
    ref<Expr> e =
        ConstantExpr::fromMemory((void *)args, getWidthForLLVMType(resultType));
    if (e->getWidth() == Context::get().getPointerWidth() &&
        resultType->isPointerTy()) {
      e = PointerExpr::create(e, e);
    }
    if (ExternCallsCanReturnNull &&
        e->getWidth() == Context::get().getPointerWidth() &&
        resultType->isPointerTy()) {
      ref<Expr> symExternCallsCanReturnNullExpr =
          makeMockValue("symExternCallsCanReturnNull", Expr::Bool);
      e = SelectExpr::create(
          symExternCallsCanReturnNullExpr,
          PointerExpr::create(Expr::createPointer(0), Expr::createPointer(0)),
          e);
    }
    bindLocal(target, state, e);
  }
}

/***/

ref<Expr> Executor::replaceReadWithSymbolic(ExecutionState &state,
                                            ref<Expr> e) {
  unsigned n = interpreterOpts.MakeConcreteSymbolic;
  if (!n || replayKTest || replayPath)
    return e;

  // right now, we don't replace symbolics (is there any reason to?)
  if (!isa<ConstantExpr>(e))
    return e;

  if (n != 1 && random() % n)
    return e;

  // create a new fresh location, assert it is equal to concrete value in e
  // and return it.

  const Array *array =
      makeArray(Expr::createPointer(Expr::getMinBytesForWidth(e->getWidth())),
                SourceBuilder::makeSymbolic(
                    "rrws_arr", updateNameVersion(state, "rrws_arr")));
  ref<Expr> res = Expr::createTempRead(array, e->getWidth());
  ref<Expr> eq = NotOptimizedExpr::create(EqExpr::create(e, res));
  llvm::errs() << "Making symbolic: " << eq << "\n";
  addConstraint(state, eq);
  return res;
}

ref<Expr> Executor::makeMockValue(const std::string &name, Expr::Width width) {
  const Array *array =
      makeArray(Expr::createPointer(Expr::getMinBytesForWidth(width)),
                SourceBuilder::irreproducible(name));
  ref<Expr> result = Expr::createTempRead(array, width);
  return result;
}

ObjectState *Executor::bindObjectInState(ExecutionState &state,
                                         const MemoryObject *mo,
                                         KType *dynamicType, bool isLocal,
                                         const Array *array) {
  ObjectState *os = array ? new ObjectState(mo, array, dynamicType)
                          : new ObjectState(mo, dynamicType);
  state.addressSpace.bindObject(mo, os);

  // Its possible that multiple bindings of the same mo in the state
  // will put multiple copies on this list, but it doesn't really
  // matter because all we use this list for is to unbind the object
  // on function return.
  if (isLocal && state.stack.size() > 0) {
    state.stack.valueStack().back().allocas.push_back(mo);
  }
  return os;
}

void Executor::executeAlloc(ExecutionState &state, ref<Expr> size, bool isLocal,
                            KInstruction *target, KType *type, bool zeroMemory,
                            const ObjectState *reallocFrom,
                            size_t allocationAlignment, bool checkOutOfMemory) {
  static unsigned allocations = 0;
  if (allocationAlignment == 0) {
    allocationAlignment = getAllocationAlignment(state.prevPC->inst());
  }

  if (!isa<ConstantExpr>(size) && !UseSymbolicSizeAllocation) {
    concretizeSize(state, size, isLocal, target, type, zeroMemory, reallocFrom,
                   allocationAlignment, checkOutOfMemory);
    return;
  }

  ref<Expr> upperBoundSizeConstraint = Expr::createTrue();
  if (!isa<ConstantExpr>(size) && MaxSymbolicAllocationSize) {
    upperBoundSizeConstraint = UleExpr::create(
        ZExtExpr::create(size, Context::get().getPointerWidth()),
        Expr::createPointer(MaxSymbolicAllocationSize));
  }

  /* If size greater then upper bound for size, then we will follow
  the malloc semantic and return NULL. Otherwise continue execution. */
  PartialValidity inBounds;
  solver->setTimeout(coreSolverTimeout);
  bool success =
      solver->evaluate(state.constraints.cs(), upperBoundSizeConstraint,
                       inBounds, state.queryMetaData);
  solver->setTimeout(time::Span());
  if (!success) {
    terminateStateOnSolverError(state, "Query timed out (resolve)");
    return;
  }

  if (inBounds != PValidity::MustBeFalse && inBounds != PValidity::MayBeFalse) {
    ref<Expr> conditionExpr = Expr::createTrue();
    if (inBounds != PValidity::MustBeTrue) {
      addConstraint(state, upperBoundSizeConstraint);
    }

    MemoryObject *mo =
        allocate(state, size, isLocal, /*isGlobal=*/false, locationOf(state),
                 allocationAlignment, type, conditionExpr);
    if (!mo) {
      bindLocal(
          target, state,
          PointerExpr::create(Expr::createPointer(0), Expr::createPointer(0)));
    } else {
      ref<SymbolicSource> source = nullptr;
      if (zeroMemory) {
        source = SourceBuilder::constant(
            constructStorage(size, ConstantExpr::create(0, Expr::Int8)));
      } else {
        source = SourceBuilder::uninitialized(allocations++, target);
      }
      auto array = makeArray(size, source);
      ObjectState *os = bindObjectInState(state, mo, type, isLocal, array);

      ref<Expr> address = mo->getBaseExpr();

      if (isLocal) {
        addConstraint(state, Expr::createIsZero(EqExpr::create(
                                 address, Expr::createPointer(0))));
      }

      if (checkOutOfMemory && !isLocal) {
        ref<Expr> symCheckOutOfMemoryExpr =
            makeMockValue("symCheckOutOfMemory", Expr::Bool);
        address = SelectExpr::create(symCheckOutOfMemoryExpr,
                                     Expr::createPointer(0), address);
      }

      state.addPointerResolution(PointerExpr::create(address, address), mo);
      bindLocal(target, state, PointerExpr::create(address, address));

      if (reallocFrom) {
        os->write(reallocFrom);
        state.removePointerResolutions(reallocFrom->getObject());
        state.addressSpace.unbindObject(reallocFrom->getObject());
      }
    }
  } else {
    terminateStateEarly(state, "", StateTerminationType::SilentExit);
  }
}

void Executor::executeFree(ExecutionState &state, ref<PointerExpr> address,
                           KInstruction *target) {
  address = optimizer.optimizeExpr(address, true);
  ref<Expr> isNullPointer = Expr::createIsZero(address->getValue());
  StatePair zeroPointer = forkInternal(state, isNullPointer, BranchType::Free);
  if (zeroPointer.first) {
    if (target)
      bindLocal(
          target, *zeroPointer.first,
          PointerExpr::create(Expr::createPointer(0), Expr::createPointer(0)));
  }
  if (zeroPointer.second) { // address != 0
    ExactResolutionList rl;
    if (!resolveExact(*zeroPointer.second, address,
                      typeSystemManager->getUnknownType(), rl, "free") &&
        guidanceKind == GuidanceKind::ErrorGuidance) {
      terminateStateOnTargetError(*zeroPointer.second,
                                  ReachWithError::DoubleFree);
      return;
    }

    for (Executor::ExactResolutionList::iterator it = rl.begin(), ie = rl.end();
         it != ie; ++it) {
      const MemoryObject *mo = it->first;

      if (mo->isLocal) {
        terminateStateOnProgramError(
            *it->second,
            new ErrorEvent(new AllocEvent(mo->allocSite),
                           locationOf(*it->second), StateTerminationType::Free,
                           "free of alloca"),
            getAddressInfo(*it->second, address));
      } else if (mo->isGlobal) {
        terminateStateOnProgramError(
            *it->second,
            new ErrorEvent(new AllocEvent(mo->allocSite),
                           locationOf(*it->second), StateTerminationType::Free,
                           "free of global"),
            getAddressInfo(*it->second, address));
      } else {
        it->second->removePointerResolutions(mo);
        it->second->addressSpace.unbindObject(mo);
        if (target)
          bindLocal(target, *it->second,
                    PointerExpr::create(Expr::createPointer(0),
                                        Expr::createPointer(0)));
      }
    }
  }
}

bool Executor::resolveExact(ExecutionState &estate, ref<Expr> address,
                            KType *type, ExactResolutionList &results,
                            const std::string &name) {
  ref<PointerExpr> pointer =
      PointerExpr::create(address->getValue(), address->getValue());
  address = pointer->getValue();
  ref<Expr> base = pointer->getBase();
  ref<PointerExpr> basePointer = PointerExpr::create(base, base);
  ref<Expr> zeroPointer = PointerExpr::create(Expr::createPointer(0));

  if (SimplifySymIndices) {
    if (!isa<ConstantExpr>(address))
      address = Simplificator::simplifyExpr(estate.constraints.cs(), address)
                    .simplified;
    if (!isa<ConstantExpr>(base))
      base =
          Simplificator::simplifyExpr(estate.constraints.cs(), base).simplified;
  }

  address = optimizer.optimizeExpr(address, true);
  base = optimizer.optimizeExpr(base, true);

  ref<Expr> uniqueBase =
      Simplificator::simplifyExpr(estate.constraints.cs(), base).simplified;
  uniqueBase = toUnique(estate, uniqueBase);

  ref<Expr> isNullPointer = Expr::createIsZero(base);
  StatePair branches = forkInternal(estate, isNullPointer, BranchType::MemOp);
  ExecutionState *bound = branches.first;
  if (bound) {
    auto error = isReadFromSymbolicArray(uniqueBase)
                     ? ReachWithError::MayBeNullPointerException
                     : ReachWithError::MustBeNullPointerException;
    terminateStateOnTargetError(*bound, error);
  }
  if (!branches.second) {
    address =
        PointerExpr::create(Expr::createPointer(0), Expr::createPointer(0));
    return true;
  }

  ExecutionState &state = *branches.second;

  ObjectResolutionList rl;
  bool mayBeOutOfBound = true;
  bool hasLazyInitialized = false;
  bool incomplete = false;

  /* We do not need this variable here, just a placeholder for resolve */
  [[maybe_unused]] bool success = resolveMemoryObjects(
      state, pointer, type, state.prevPC, 0, rl, mayBeOutOfBound,
      hasLazyInitialized, incomplete,
      LazyInitialization == LazyInitializationPolicy::Only);
  assert(success);

  ExecutionState *unbound = &state;
  for (unsigned i = 0; i < rl.size(); ++i) {
    const MemoryObject *mo = rl.at(i).get();
    ref<Expr> inBounds;
    inBounds = EqExpr::create(pointer->getValue(), mo->getBaseExpr());
    StatePair branches =
        forkInternal(*unbound, inBounds, BranchType::ResolvePointer);

    if (branches.first)
      results.push_back(std::make_pair(rl.at(i).get(), branches.first));

    unbound = branches.second;
    if (!unbound) // Fork failure
      break;
  }

  if (guidanceKind == GuidanceKind::ErrorGuidance && rl.size() == 0) {
    return false;
  }

  if (unbound) {
    if (incomplete) {
      terminateStateOnSolverError(*unbound, "Query timed out (resolve).");
    } else {
      terminateStateOnProgramError(
          *unbound,
          new ErrorEvent(locationOf(*unbound), StateTerminationType::Ptr,
                         "memory error: invalid pointer: " + name),
          getAddressInfo(*unbound, pointer));
    }
  }
  return true;
}

void Executor::concretizeSize(ExecutionState &state, ref<Expr> size,
                              bool isLocal, KInstruction *target, KType *type,
                              bool zeroMemory, const ObjectState *reallocFrom,
                              size_t allocationAlignment,
                              bool checkOutOfMemory) {
  // XXX For now we just pick a size. Ideally we would support
  // symbolic sizes fully but even if we don't it would be better to
  // "smartly" pick a value, for example we could fork and pick the
  // min and max values and perhaps some intermediate (reasonable
  // value).
  //
  // It would also be nice to recognize the case when size has
  // exactly two values and just fork (but we need to get rid of
  // return argument first). This shows up in pcre when llvm
  // collapses the size expression with a select.

  size = optimizer.optimizeExpr(size, true);

  ref<ConstantExpr> example;
  bool success = solver->getValue(state.constraints.cs(), size, example,
                                  state.queryMetaData);
  assert(success && "FIXME: Unhandled solver failure");
  (void)success;

  // Try and start with a small example.
  Expr::Width W = example->getWidth();
  while (example->Ugt(ConstantExpr::alloc(128, W))->isTrue()) {
    ref<ConstantExpr> tmp = example->LShr(ConstantExpr::alloc(1, W));
    bool res;
    bool success =
        solver->mayBeTrue(state.constraints.cs(), EqExpr::create(tmp, size),
                          res, state.queryMetaData);
    assert(success && "FIXME: Unhandled solver failure");
    (void)success;
    if (!res)
      break;
    example = tmp;
  }

  StatePair fixedSize =
      forkInternal(state, EqExpr::create(example, size), BranchType::Alloc);

  if (fixedSize.second) {
    // Check for exactly two values
    ref<ConstantExpr> tmp;
    bool success = solver->getValue(fixedSize.second->constraints.cs(), size,
                                    tmp, fixedSize.second->queryMetaData);
    assert(success && "FIXME: Unhandled solver failure");
    (void)success;
    bool res;
    success = solver->mustBeTrue(fixedSize.second->constraints.cs(),
                                 EqExpr::create(tmp, size), res,
                                 fixedSize.second->queryMetaData);
    assert(success && "FIXME: Unhandled solver failure");
    (void)success;
    if (res) {
      executeAlloc(*fixedSize.second, tmp, isLocal, target, type, zeroMemory,
                   reallocFrom);
    } else {
      // See if a *really* big value is possible. If so assume
      // malloc will fail for it, so lets fork and return 0.
      StatePair hugeSize =
          forkInternal(*fixedSize.second,
                       UltExpr::create(ConstantExpr::alloc(1U << 31, W), size),
                       BranchType::Alloc);
      if (hugeSize.first) {
        klee_message("NOTE: found huge malloc, returning 0");
        bindLocal(target, *hugeSize.first,
                  PointerExpr::create(Expr::createPointer(0),
                                      Expr::createPointer(0)));
      }

      if (hugeSize.second) {
        std::string Str;
        llvm::raw_string_ostream info(Str);
        ExprPPrinter::printOne(info, "  size expr", size);
        info << "  concretization : " << example << "\n";
        info << "  unbound example: " << tmp << "\n";
        terminateStateOnProgramError(
            *hugeSize.second,
            new ErrorEvent(locationOf(*hugeSize.second),
                           StateTerminationType::Model,
                           "concretized symbolic size"),
            info.str());
      }
    }
  }

  if (fixedSize.first) // can be zero when fork fails
    executeAlloc(*fixedSize.first, example, isLocal, target, type, zeroMemory,
                 reallocFrom, allocationAlignment, checkOutOfMemory);
}

bool Executor::computeSizes(
    const ConstraintSet &constraints, ref<Expr> symbolicSizesSum,
    std::vector<const Array *> &objects,
    std::vector<SparseStorageImpl<unsigned char>> &values,
    SolverQueryMetaData &metaData) {
  ref<ConstantExpr> minimalSumValue;
  ref<SolverResponse> response;

  /* Compute assignment for symcretes. */
  objects = constraints.gatherSymcretizedArrays();
  findObjects(symbolicSizesSum, objects);

  solver->setTimeout(coreSolverTimeout);
  bool success = solver->getResponse(
      constraints,
      UgtExpr::create(symbolicSizesSum,
                      ConstantExpr::create(SymbolicAllocationThreshold,
                                           symbolicSizesSum->getWidth())),
      response, metaData);
  solver->setTimeout(time::Span());

  if (!response->tryGetInitialValuesFor(objects, values)) {
    /* Receive model with a smallest sum as possible. */
    solver->setTimeout(coreSolverTimeout);
    success = solver->getMinimalUnsignedValue(constraints, symbolicSizesSum,
                                              minimalSumValue, metaData);
    solver->setTimeout(time::Span());
    assert(success);

    /* We can simply query the solver to get value of size, but
    to optimize the number of queries we will ask it one time with assignment
    for symcretes. */

    ConstraintSet minimized = constraints;
    minimized.addConstraint(EqExpr::create(symbolicSizesSum, minimalSumValue));

    solver->setTimeout(coreSolverTimeout);
    success = solver->getInitialValues(minimized, objects, values, metaData);
    solver->setTimeout(time::Span());
  }
  return success;
}

MemoryObject *Executor::allocate(ExecutionState &state, ref<Expr> size,
                                 bool isLocal, bool isGlobal,
                                 ref<CodeLocation> allocSite,
                                 size_t allocationAlignment, KType *type,
                                 ref<Expr> conditionExpr,
                                 ref<Expr> lazyInitializationSource,
                                 unsigned timestamp) {
  size = ZExtExpr::create(size, Context::get().getPointerWidth());

  /* Try to find existing solution. */
  ref<Expr> uniqueSize = toUnique(state, size);

  ref<ConstantExpr> arrayConstantSize =
      dyn_cast<ConstantExpr>(optimizer.optimizeExpr(uniqueSize, true));

  /* Constant solution exists. Just return it. */
  if (arrayConstantSize && lazyInitializationSource.isNull()) {
    MemoryObject *mo =
        memory->allocate(arrayConstantSize, isLocal, isGlobal, false, allocSite,
                         allocationAlignment, type);
    if (mo && state.isGEPExpr(mo->getBaseExpr())) {
      state.gepExprBases.erase(mo->getBaseExpr());
    }
    return mo;
  }

  Expr::Width pointerWidthInBits = Context::get().getPointerWidth();

  /// Determine source for address array:
  /// * LI source if allocate occures on lazy initialization
  /// * Otherwise choose source depending on the allocation site
  ref<SymbolicSource> sourceAddressArray;
  if (!lazyInitializationSource) {
    if (const auto inst =
            dyn_cast<const llvm::Instruction>(allocSite->source->unwrap())) {
      KInstruction *ki =
          kmodule->getKBlock(inst->getParent())->parent->instructionMap[inst];
      sourceAddressArray = SourceBuilder::symbolicSizeConstantAddress(
          updateNameVersion(state, "const_arr"), ki, size);
    } else if (const auto global = dyn_cast<const llvm::GlobalVariable>(
                   allocSite->source->unwrap())) {
      KGlobalVariable *kgb = kmodule->globalMap[global].get();
      sourceAddressArray = SourceBuilder::symbolicSizeConstantAddress(
          updateNameVersion(state, "const_arr"), kgb, size);
    }
  } else {
    sourceAddressArray =
        lazyInitializationSource->hasOrderedReads()->updates.root->source;
  }

  if (lazyInitializationSource) {
    lazyInitializationSource =
        Simplificator::simplifyExpr(state.constraints.cs(),
                                    lazyInitializationSource)
            .simplified;
  }

  const Array *addressArray = makeArray(
      Expr::createPointer(pointerWidthInBits / CHAR_BIT), sourceAddressArray);
  ref<Expr> addressExpr =
      Expr::createTempRead(addressArray, pointerWidthInBits);

  if (lazyInitializationSource && isLocal) {
    conditionExpr = AndExpr::create(
        conditionExpr, NotExpr::create(Expr::createIsZero(addressExpr)));
    conditionExpr = AndExpr::create(
        conditionExpr, EqExpr::create(addressExpr, lazyInitializationSource));
  }

  /* Create symcretes for array and size */
  ref<AddressSymcrete> addressSymcrete =
      lazyInitializationSource
          ? cast<AddressSymcrete>(
                new LazyInitializedAddressSymcrete(addressExpr))
          : cast<AddressSymcrete>(new AllocAddressSymcrete(addressExpr));
  ref<SizeSymcrete> sizeSymcrete =
      lazyInitializationSource
          ? cast<SizeSymcrete>(new LazyInitializedSizeSymcrete(
                size, cast<LazyInitializedAddressSymcrete>(addressSymcrete)))
          : cast<SizeSymcrete>(new AllocSizeSymcrete(
                size, cast<AllocAddressSymcrete>(addressSymcrete)));

  sizeSymcrete->addDependentSymcrete(addressSymcrete);
  addressSymcrete->addDependentSymcrete(sizeSymcrete);

  const Array *contentArray =
      lazyInitializationSource
          ? makeArray(size, SourceBuilder::lazyInitializationContent(
                                lazyInitializationSource))
          : nullptr;

  /* Allocate corresponding memory object. */
  MemoryObject *mo = memory->allocate(
      size, isLocal, isGlobal, !lazyInitializationSource.isNull(), allocSite,
      allocationAlignment, type, conditionExpr, addressExpr, timestamp,
      contentArray);

  if (!mo) {
    return nullptr;
  }

  if (lazyInitializationSource.isNull()) {
    state.addPointerResolution(PointerExpr::create(addressExpr, addressExpr),
                               mo);
  }

  state.constraints.addSymcrete(sizeSymcrete);
  state.constraints.addSymcrete(addressSymcrete);
  if (lazyInitializationSource) {
    state.addSymbolic(mo, contentArray, type);
  }
  return mo;
}

bool Executor::resolveMemoryObjects(
    ExecutionState &state, ref<PointerExpr> address, KType *targetType,
    KInstruction *target, unsigned bytes,
    ObjectResolutionList &mayBeResolvedMemoryObjects, bool &mayBeOutOfBound,
    bool &mayLazyInitialize, bool &incomplete, bool onlyLazyInitialize) {
  mayLazyInitialize = false;
  mayBeOutOfBound = true;
  incomplete = false;

  ref<Expr> base = address->getBase();
  unsigned size = bytes;
  KType *baseTargetType = targetType;

  if (state.isGEPExpr(base)) {
    size = kmodule->targetData->getTypeStoreSize(state.gepExprBases[base]);
    baseTargetType = typeSystemManager->getWrappedType(llvm::PointerType::get(
        state.gepExprBases[base], kmodule->targetData->getAllocaAddrSpace()));
  }

  base = Simplificator::simplifyExpr(state.constraints.cs(), base).simplified;
  ref<PointerExpr> basePointer = PointerExpr::create(base, base);

  auto mso = MemorySubobject(address, bytes);
  if (state.resolvedSubobjects.count(mso)) {
    for (auto resolution : state.resolvedSubobjects.at(mso)) {
      mayBeResolvedMemoryObjects.push_back(resolution);
    }
    mayBeOutOfBound = false;
  } else if (state.resolvedPointers.count(base)) {
    for (auto resolution : state.resolvedPointers.at(base)) {
      mayBeResolvedMemoryObjects.push_back(resolution);
    }
  } else {
    // we are on an error path (no resolution, multiple resolution, one
    // resolution with out of bounds)

    base = optimizer.optimizeExpr(base, true);
    ref<Expr> checkOutOfBounds = Expr::createTrue();

    ref<ReadExpr> readBase = base->hasOrderedReads();
    bool checkAddress = readBase && readBase->updates.getSize() == 0 &&
                        readBase->updates.root->isSymbolicArray();
    if (!checkAddress && isa<SelectExpr>(base)) {
      std::vector<ref<Expr>> alternatives;
      ArrayExprHelper::collectAlternatives(*cast<SelectExpr>(base),
                                           alternatives);
      checkAddress = std::find_if(alternatives.begin(), alternatives.end(),
                                  [](ref<Expr> expr) {
                                    return !isa<ConstantExpr>(expr);
                                  }) != alternatives.end();

      if (checkAddress) {
        for (auto alt : alternatives) {
          if (isa<ConstantExpr>(alt)) {
            continue;
          }
          readBase = alt->hasOrderedReads();
          checkAddress &= readBase && readBase->updates.getSize() == 0 &&
                          readBase->updates.root->isSymbolicArray();
        }
      }
    }

    mayLazyInitialize =
        LazyInitialization != LazyInitializationPolicy::None && checkAddress;

    if (!onlyLazyInitialize || !mayLazyInitialize) {
      ResolutionList rl;

      solver->setTimeout(coreSolverTimeout);
      incomplete =
          state.addressSpace.resolve(state, solver.get(), basePointer,
                                     targetType, rl, 0, coreSolverTimeout);
      solver->setTimeout(time::Span());

      for (ResolutionList::iterator i = rl.begin(), ie = rl.end(); i != ie;
           ++i) {
        const MemoryObject *mo = i->first;
        ref<Expr> inBounds = mo->getBoundsCheckPointer(address, bytes);
        ref<Expr> notInBounds = Expr::createIsZero(inBounds);
        ref<Expr> addressNotInBounds =
            Expr::createIsZero(mo->getBoundsCheckAddress(address->getValue()));
        mayBeResolvedMemoryObjects.push_back(mo);
        checkOutOfBounds = AndExpr::create(checkOutOfBounds, notInBounds);
      }
    }

    if (mayLazyInitialize) {
      solver->setTimeout(coreSolverTimeout);
      bool success = solver->mayBeTrue(state.constraints.cs(), checkOutOfBounds,
                                       mayLazyInitialize, state.queryMetaData);
      solver->setTimeout(time::Span());
      if (!success) {
        return false;
      } else if (mayLazyInitialize) {
        uint64_t minObjectSize = 0;
        // minObjectSize = MinNumberElementsLazyInit * MinElementSizeLazyInit;
        minObjectSize = MinNumberElementsLazyInit * size;

        const Array *lazyInstantiationSize = makeArray(
            Expr::createPointer(Context::get().getPointerWidth() / CHAR_BIT),
            SourceBuilder::lazyInitializationSize(base));
        auto sizeExpr = Expr::createTempRead(lazyInstantiationSize,
                                             Context::get().getPointerWidth());
        ref<const MemoryObject> idLazyInitialization = lazyInitializeObject(
            state, basePointer, target, baseTargetType, minObjectSize, sizeExpr,
            false, checkOutOfBounds, UseSymbolicSizeLazyInit);
        RefObjectPair op = state.addressSpace.findOrLazyInitializeObject(
            idLazyInitialization.get());
        state.addressSpace.bindObject(op.first, op.second.get());
        mayBeResolvedMemoryObjects.push_back(idLazyInitialization);
      }
    }
  }
  return true;
}

bool Executor::checkResolvedMemoryObjects(
    ExecutionState &state, ref<PointerExpr> address, unsigned bytes,
    const ObjectResolutionList &mayBeResolvedMemoryObjects,
    bool hasLazyInitialized, ObjectResolutionList &resolvedMemoryObjects,
    std::vector<ref<Expr>> &resolveConditions,
    std::vector<ref<Expr>> &unboundConditions, ref<Expr> &checkOutOfBounds,
    bool &mayBeOutOfBound) {
  ref<Expr> base = address->getBase();
  ref<PointerExpr> basePointer = PointerExpr::create(base, base);
  unsigned size = bytes;

  if (state.isGEPExpr(base)) {
    size = kmodule->targetData->getTypeStoreSize(state.gepExprBases.at(base));
  }

  checkOutOfBounds = Expr::createTrue();
  if (mayBeResolvedMemoryObjects.size() == 1) {
    const MemoryObject *mo = mayBeResolvedMemoryObjects.begin()->get();

    state.addPointerResolution(address, mo);

    ref<Expr> inBounds = mo->getBoundsCheckPointer(address, bytes);
    ref<Expr> baseInBounds = Expr::createTrue();
    ref<Expr> notInBounds = Expr::createIsZero(inBounds);
    ref<Expr> addressNotInBounds =
        Expr::createIsZero(mo->getBoundsCheckAddress(address->getValue()));

    inBounds = AndExpr::create(inBounds, baseInBounds);
    inBounds = optimizer.optimizeExpr(inBounds, true);
    inBounds = Simplificator::simplifyExpr(state.constraints.cs(), inBounds)
                   .simplified;
    notInBounds =
        Simplificator::simplifyExpr(state.constraints.cs(), notInBounds)
            .simplified;
    addressNotInBounds =
        Simplificator::simplifyExpr(state.constraints.cs(), addressNotInBounds)
            .simplified;

    PartialValidity result;
    solver->setTimeout(coreSolverTimeout);
    bool success = solver->evaluate(state.constraints.cs(), inBounds, result,
                                    state.queryMetaData);
    solver->setTimeout(time::Span());
    if (!success) {
      return false;
    }

    mayBeOutOfBound = PValidity::MustBeFalse == result ||
                      PValidity::MayBeFalse == result ||
                      PValidity::TrueOrFalse == result;
    bool mayBeInBound = PValidity::MustBeTrue == result ||
                        PValidity::MayBeTrue == result ||
                        PValidity::TrueOrFalse == result;
    bool mustBeInBounds = PValidity::MustBeTrue == result;
    bool mustBeOutOfBound = PValidity::MustBeFalse == result;

    if (mayBeInBound) {
      state.addPointerResolution(address, mo, bytes);
      state.addPointerResolution(basePointer, mo, size);
      resolvedMemoryObjects.push_back(mo);
      if (mustBeInBounds) {
        resolveConditions.push_back(Expr::createTrue());
        unboundConditions.push_back(Expr::createFalse());
        checkOutOfBounds = Expr::createFalse();
      } else {
        resolveConditions.push_back(inBounds);
        unboundConditions.push_back(addressNotInBounds);
        checkOutOfBounds = notInBounds;
      }
    } else if (mayBeOutOfBound) {
      if (mustBeOutOfBound) {
        checkOutOfBounds = Expr::createTrue();
      } else {
        checkOutOfBounds = notInBounds;
      }
    }
  } else {
    for (unsigned int i = 0; i < mayBeResolvedMemoryObjects.size(); ++i) {
      const MemoryObject *mo = mayBeResolvedMemoryObjects.at(i).get();

      state.addPointerResolution(address, mo);

      ref<Expr> inBounds = mo->getBoundsCheckPointer(address, bytes);
      ref<Expr> baseInBounds = Expr::createTrue();
      ref<Expr> notInBounds = Expr::createIsZero(inBounds);
      ref<Expr> addressNotInBounds =
          Expr::createIsZero(mo->getBoundsCheckAddress(address->getValue()));

      if (hasLazyInitialized && i == mayBeResolvedMemoryObjects.size() - 1) {
        inBounds = AndExpr::create(inBounds, checkOutOfBounds);
      }

      inBounds = AndExpr::create(inBounds, baseInBounds);
      inBounds = Simplificator::simplifyExpr(state.constraints.cs(), inBounds)
                     .simplified;
      notInBounds =
          Simplificator::simplifyExpr(state.constraints.cs(), notInBounds)
              .simplified;
      addressNotInBounds = Simplificator::simplifyExpr(state.constraints.cs(),
                                                       addressNotInBounds)
                               .simplified;

      bool mayBeInBounds;
      solver->setTimeout(coreSolverTimeout);
      bool success = solver->mayBeTrue(state.constraints.cs(), inBounds,
                                       mayBeInBounds, state.queryMetaData);
      solver->setTimeout(time::Span());
      if (!success) {
        return false;
      }
      if (!mayBeInBounds) {
        continue;
      }

      state.addPointerResolution(address, mo, bytes);
      state.addPointerResolution(basePointer, mo, size);

      resolveConditions.push_back(inBounds);
      resolvedMemoryObjects.push_back(mo);
      unboundConditions.push_back(addressNotInBounds);

      if (mayBeOutOfBound) {
        checkOutOfBounds = AndExpr::create(checkOutOfBounds, notInBounds);
      }
    }
  }

  if (mayBeOutOfBound) {
    solver->setTimeout(coreSolverTimeout);
    bool success = solver->mayBeTrue(state.constraints.cs(), checkOutOfBounds,
                                     mayBeOutOfBound, state.queryMetaData);
    solver->setTimeout(time::Span());
    if (!success) {
      return false;
    }
  }

  if (!mayBeOutOfBound) {
    checkOutOfBounds = Expr::createFalse();
  }

  return true;
}

bool Executor::makeGuard(ExecutionState &state,
                         const std::vector<ref<Expr>> &resolveConditions,
                         ref<Expr> &guard, bool &mayBeInBounds) {
  guard = Expr::createFalse();

  if (resolveConditions.size() > 0) {
    guard = resolveConditions.back();
    for (unsigned int i = 0; i < resolveConditions.size() - 1; ++i) {
      guard = SelectExpr::create(
          resolveConditions.at(resolveConditions.size() - 2 - i),
          Expr::createTrue(), guard);
    }
  }

  solver->setTimeout(coreSolverTimeout);
  bool success = solver->mayBeTrue(state.constraints.cs(), guard, mayBeInBounds,
                                   state.queryMetaData);
  solver->setTimeout(time::Span());
  if (!success) {
    return false;
  }
  if (!mayBeInBounds) {
    guard = Expr::createFalse();
  }
  return true;
}

void Executor::collectReads(ExecutionState &state, ref<PointerExpr> address,
                            Expr::Width type,
                            const ObjectResolutionList &resolvedMemoryObjects,
                            std::vector<ref<Expr>> &results) {
  for (unsigned int i = 0; i < resolvedMemoryObjects.size(); ++i) {
    RefObjectPair op = state.addressSpace.findOrLazyInitializeObject(
        resolvedMemoryObjects.at(i).get());
    const MemoryObject *mo = op.first;
    const ObjectState *os = op.second.get();

    ref<Expr> offset = mo->getOffsetExpr(address);
    if (SimplifySymIndices) {
      if (!isa<ConstantExpr>(offset)) {
        offset = Simplificator::simplifyExpr(state.constraints.cs(), offset)
                     .simplified;
      }
    }

    ref<Expr> result = os->read(offset, type);

    if (X86FPAsX87FP80 &&
        state.prevPC->inst()->getType()->isFloatingPointTy() &&
        Context::get().getPointerWidth() == 32) {
      result = FPToX87FP80Ext(result);
    }

    results.push_back(result);
  }
}

void Executor::executeMemoryOperation(
    ExecutionState &estate, bool isWrite, KType *targetType,
    ref<PointerExpr> address, ref<Expr> value /* undef if read */,
    KInstruction *target /* undef if write */) {
  KInstruction *ki = estate.prevPC;
  if (isWrite && isa<const llvm::StoreInst>(ki->inst())) {
    if (X86FPAsX87FP80) {
      auto valueOperand =
          cast<const llvm::StoreInst>(ki->inst())->getValueOperand();
      if (valueOperand->getType()->isFloatingPointTy() &&
          Context::get().getPointerWidth() == 32) {
        value = X87FP80ToFPTrunc(value,
                                 getWidthForLLVMType(valueOperand->getType()),
                                 estate.roundingMode);
      }
    }
    auto valueOperand =
        cast<const llvm::StoreInst>(ki->inst())->getValueOperand();
    if (valueOperand->getType()->isPointerTy()) {
      value = PointerExpr::create(value);
    }
  }
  Expr::Width type = (isWrite ? value->getWidth()
                              : getWidthForLLVMType(target->inst()->getType()));
  unsigned bytes = Expr::getMinBytesForWidth(type);

  ref<Expr> base = address->getBase();
  ref<PointerExpr> basePointer = PointerExpr::create(base, base);
  ref<Expr> zeroPointer = PointerExpr::create(Expr::createPointer(0));
  unsigned size = bytes;
  KType *baseTargetType = targetType;

  if (estate.isGEPExpr(base)) {
    baseTargetType = typeSystemManager->getWrappedType(llvm::PointerType::get(
        estate.gepExprBases[base], kmodule->targetData->getAllocaAddrSpace()));
  }

  if (SimplifySymIndices) {
    if (!isa<ConstantExpr>(base)) {
      base =
          Simplificator::simplifyExpr(estate.constraints.cs(), base).simplified;
    }
    if (isWrite && !isa<ConstantExpr>(value))
      value = Simplificator::simplifyExpr(estate.constraints.cs(), value)
                  .simplified;
  }

  base = optimizer.optimizeExpr(base, true);

  ref<Expr> isNullPointer = Expr::createIsZero(base);
  StatePair branches = forkInternal(estate, isNullPointer, BranchType::MemOp);
  ExecutionState *bound = branches.first;
  if (bound) {
    auto error = (isReadFromSymbolicArray(base) && branches.second)
                     ? ReachWithError::MayBeNullPointerException
                     : ReachWithError::MustBeNullPointerException;
    terminateStateOnTargetError(*bound, error);
  }
  if (!branches.second)
    return;
  ExecutionState *state = branches.second;

  // fast path: single in-bounds resolution
  ref<const MemoryObject> idFastResult;
  bool success = false;

  if (state->resolvedPointers.count(base) &&
      state->resolvedPointers.at(base).size() == 1) {
    success = true;
    idFastResult = *state->resolvedPointers[base].begin();
  } else {
    ObjectPair idFastOp;
    solver->setTimeout(coreSolverTimeout);

    if (!state->addressSpace.resolveOne(*state, solver.get(), address,
                                        targetType, idFastOp, success,
                                        haltExecution)) {
      address = toConstantPointer(*state, address, "resolveOne failure");
      success = state->addressSpace.resolveOne(
          cast<ConstantPointerExpr>(address), targetType, idFastOp);
    }

    solver->setTimeout(time::Span());

    if (success) {
      idFastResult = idFastOp.first;
    }
  }

  if (success) {
    RefObjectPair op =
        state->addressSpace.findOrLazyInitializeObject(idFastResult.get());
    const MemoryObject *mo = op.first;

    ref<ConstantExpr> sizeExpr = dyn_cast<ConstantExpr>(mo->getSizeExpr());
    if (MaxSymArraySize && sizeExpr &&
        sizeExpr->getZExtValue() >= MaxSymArraySize) {
      base = toConstant(*state, base, "max-sym-array-size");
    }

    ref<Expr> inBounds = mo->getBoundsCheckPointer(address, bytes);

    inBounds = optimizer.optimizeExpr(inBounds, true);
    inBounds = Simplificator::simplifyExpr(state->constraints.cs(), inBounds)
                   .simplified;

    ref<SolverResponse> response;
    solver->setTimeout(coreSolverTimeout);
    bool success = solver->getResponse(state->constraints.cs(), inBounds,
                                       response, state->queryMetaData);
    solver->setTimeout(time::Span());
    if (!success) {
      state->pc = state->prevPC;
      terminateStateOnSolverError(*state, "Query timed out (bounds check).");
      return;
    }

    bool mustBeInBounds = !isa<InvalidResponse>(response);
    if (mustBeInBounds) {
      ref<Expr> result;
      op = state->addressSpace.findOrLazyInitializeObject(idFastResult.get());
      const ObjectState *os = op.second.get();
      state->addPointerResolution(basePointer, mo);
      state->addPointerResolution(address, mo);
      ref<Expr> offset = mo->getOffsetExpr(address);
      if (SimplifySymIndices) {
        if (!isa<ConstantExpr>(offset)) {
          offset = Simplificator::simplifyExpr(state->constraints.cs(), offset)
                       .simplified;
        }
      }
      if (isWrite) {
        ObjectState *wos = state->addressSpace.getWriteable(mo, os);
        maxNewWriteableOSSize =
            std::max(maxNewWriteableOSSize, wos->getSparseStorageEntries());
        wos->getDynamicType()->handleMemoryAccess(
            targetType, offset,
            ConstantExpr::alloc(size, Context::get().getPointerWidth()), true);
        if (wos->readOnly) {
          terminateStateOnProgramError(
              *state,
              new ErrorEvent(new AllocEvent(mo->allocSite), locationOf(*state),
                             StateTerminationType::ReadOnly,
                             "memory error: object read only"));
        } else {
          wos->write(offset, value);
        }
      } else {
        result = os->read(offset, type);

        if (X86FPAsX87FP80 && ki->inst()->getType()->isFloatingPointTy() &&
            Context::get().getPointerWidth() == 32) {
          result = FPToX87FP80Ext(result);
        }

        if (interpreterOpts.MakeConcreteSymbolic)
          result = replaceReadWithSymbolic(*state, result);

        if (target->inst()->getType()->isPointerTy()) {
          result = PointerExpr::create(result);
        }

        bindLocal(target, *state, result);
      }

      return;
    }
  } else if (guidanceKind == GuidanceKind::ErrorGuidance &&
             allLeafsAreConstant(address)) {
    ObjectPair idFastOp;

    solver->setTimeout(coreSolverTimeout);
    state->addressSpace.resolveOne(*state, solver.get(), basePointer,
                                   baseTargetType, idFastOp, success,
                                   haltExecution);
    solver->setTimeout(time::Span());

    if (!success) {
      terminateStateOnTargetError(*state, ReachWithError::UseAfterFree);
      return;
    }
  }

  bool mayBeOutOfBound = true;
  bool hasLazyInitialized = false;
  bool incomplete = false;
  ObjectResolutionList mayBeResolvedMemoryObjects;

  if (!resolveMemoryObjects(
          *state, address, targetType, target, bytes,
          mayBeResolvedMemoryObjects, mayBeOutOfBound, hasLazyInitialized,
          incomplete, LazyInitialization == LazyInitializationPolicy::Only)) {
    terminateStateOnSolverError(*state,
                                "Query timed out (executeMemoryOperation)");
    return;
  }

  ref<Expr> checkOutOfBounds;
  std::vector<ref<Expr>> resolveConditions;
  std::vector<ref<Expr>> unboundConditions;
  ObjectResolutionList resolvedMemoryObjects;

  if (!checkResolvedMemoryObjects(
          *state, address, bytes, mayBeResolvedMemoryObjects,
          hasLazyInitialized, resolvedMemoryObjects, resolveConditions,
          unboundConditions, checkOutOfBounds, mayBeOutOfBound)) {
    terminateStateOnSolverError(*state,
                                "Query timed out (executeMemoryOperation)");
    return;
  }

  ExecutionState *unbound = nullptr;
  if (MergedPointerDereference) {
    ref<Expr> guard;
    bool mayBeInBounds;

    assert(resolveConditions.size() == unboundConditions.size());
    if (!makeGuard(*state, resolveConditions, guard, mayBeInBounds)) {
      terminateStateOnSolverError(*state, "Query timed out (resolve)");
      return;
    }

    std::vector<ExecutionState *> branchStates;
    std::vector<ref<Expr>> branchConditions;
    if (mayBeInBounds) {
      branchConditions.push_back(guard);
    }
    if (!checkOutOfBounds->isFalse()) {
      branchConditions.push_back(checkOutOfBounds);
    }
    assert(branchConditions.size() > 0);
    branch(*state, branchConditions, branchStates, BranchType::MemOp);
    state = mayBeInBounds ? branchStates[0] : nullptr;
    unbound = mayBeInBounds && mayBeOutOfBound
                  ? branchStates[1]
                  : (mayBeOutOfBound ? branchStates[0] : nullptr);

    if (state) {
      std::vector<ref<Expr>> results;
      collectReads(*state, address, type, resolvedMemoryObjects, results);

      if (isWrite) {
        for (unsigned int i = 0; i < resolvedMemoryObjects.size(); ++i) {
          RefObjectPair op = state->addressSpace.findOrLazyInitializeObject(
              resolvedMemoryObjects.at(i).get());
          const MemoryObject *mo = op.first;
          const ObjectState *os = op.second.get();

          ObjectState *wos = state->addressSpace.getWriteable(mo, os);
          maxNewWriteableOSSize =
              std::max(maxNewWriteableOSSize, wos->getSparseStorageEntries());
          if (wos->readOnly) {
            branches =
                forkInternal(*state, resolveConditions[i], BranchType::MemOp);
            assert(branches.first);
            terminateStateOnProgramError(
                *branches.first,
                new ErrorEvent(new AllocEvent(mo->allocSite),
                               locationOf(*branches.first),
                               StateTerminationType::ReadOnly,
                               "memory error: object read only"));
            state = branches.second;
          } else {
            ref<Expr> offset = mo->getOffsetExpr(address);
            if (SimplifySymIndices) {
              if (!isa<ConstantExpr>(offset)) {
                offset =
                    Simplificator::simplifyExpr(state->constraints.cs(), offset)
                        .simplified;
              }
            }
            ref<Expr> result =
                SelectExpr::create(resolveConditions[i], value, results[i]);
            wos->write(offset, result);
          }
        }
      } else {
        ref<Expr> result = results[unboundConditions.size() - 1];
        for (unsigned int i = 0; i < unboundConditions.size(); ++i) {
          unsigned int index = unboundConditions.size() - 1 - i;
          result = SelectExpr::create(resolveConditions[index], results[index],
                                      result);
        }
        if (target->inst()->getType()->isPointerTy()) {
          result = PointerExpr::create(result);
        }
        bindLocal(target, *state, result);
      }
    }

    assert(unbound || !resolveConditions.empty());
  } else {
    std::vector<ExecutionState *> statesForMemoryOperation;
    if (mayBeOutOfBound) {
      resolveConditions.push_back(checkOutOfBounds);
    }
    assert(resolveConditions.size() > 0);
    branch(*state, resolveConditions, statesForMemoryOperation,
           BranchType::MemOp);

    for (unsigned int i = 0; i < resolvedMemoryObjects.size(); ++i) {
      ExecutionState *bound = statesForMemoryOperation[i];
      if (!bound) {
        continue;
      }
      RefObjectPair op = bound->addressSpace.findOrLazyInitializeObject(
          resolvedMemoryObjects.at(i).get());
      const MemoryObject *mo = op.first;
      const ObjectState *os = op.second.get();

      if (hasLazyInitialized && i + 1 != resolvedMemoryObjects.size()) {
        const MemoryObject *liMO =
            resolvedMemoryObjects.at(resolvedMemoryObjects.size() - 1).get();
        bound->removePointerResolutions(liMO);
        bound->addressSpace.unbindObject(liMO);
      }

      ref<Expr> offset = mo->getOffsetExpr(address);
      if (SimplifySymIndices) {
        if (!isa<ConstantExpr>(offset)) {
          offset = Simplificator::simplifyExpr(bound->constraints.cs(), offset)
                       .simplified;
        }
      }
      bound->addUniquePointerResolution(address, mo);

      /* FIXME: Notice, that here we are creating a new instance of object
      for every memory operation in order to handle type changes. This might
      waste too much memory as we do now always modify something. To fix this
      we can ask memory if we will make anything, and create a copy if
      required. */

      if (isWrite) {
        ObjectState *wos = bound->addressSpace.getWriteable(mo, os);
        maxNewWriteableOSSize =
            std::max(maxNewWriteableOSSize, wos->getSparseStorageEntries());
        wos->getDynamicType()->handleMemoryAccess(
            targetType, offset,
            ConstantExpr::alloc(size, Context::get().getPointerWidth()), true);
        if (wos->readOnly) {
          terminateStateOnProgramError(
              *bound,
              new ErrorEvent(new AllocEvent(mo->allocSite), locationOf(*bound),
                             StateTerminationType::ReadOnly,
                             "memory error: object read only"));
        } else {
          wos->write(offset, value);
        }
      } else {
        ref<Expr> result = os->read(offset, type);

        if (X86FPAsX87FP80 && ki->inst()->getType()->isFloatingPointTy() &&
            Context::get().getPointerWidth() == 32) {
          result = FPToX87FP80Ext(result);
        }

        if (target->inst()->getType()->isPointerTy()) {
          result = PointerExpr::create(result);
        }

        bindLocal(target, *bound, result);
      }
    }
    if (mayBeOutOfBound) {
      unbound = statesForMemoryOperation.back();
    }
  }

  // XXX should we distinguish out of bounds and overlapped cases?
  if (unbound) {
    if (incomplete) {
      terminateStateOnSolverError(*unbound, "Query timed out (resolve).");
      return;
    }
    assert(mayBeOutOfBound && "must be true since unbound is not null");

    /* If base may point to some object then we may provide additional
    information about object allocations site.*/

    ref<const MemoryObject> idFastResult;
    bool uniqueBaseResolved = false;

    if (unbound->resolvedPointers.count(base) &&
        unbound->resolvedPointers.at(base).size() == 1) {
      uniqueBaseResolved = true;
      idFastResult = *unbound->resolvedPointers[base].begin();
    } else if (auto constBasePointer =
                   dyn_cast<ConstantPointerExpr>(basePointer)) {
      ObjectPair contantResult;
      uniqueBaseResolved = unbound->addressSpace.resolveOne(
          constBasePointer, baseTargetType, contantResult);
      if (uniqueBaseResolved) {
        idFastResult = contantResult.first;
      }
    }

    if (uniqueBaseResolved) {
      RefObjectPair baseObjectPair =
          unbound->addressSpace.findOrLazyInitializeObject(idFastResult.get());
      if (!baseObjectPair.first->isLazyInitialized) {
        // Termiante with source event
        terminateStateOnProgramError(
            *unbound,
            new ErrorEvent(new AllocEvent(baseObjectPair.first->allocSite),
                           locationOf(*unbound), StateTerminationType::Ptr,
                           "memory error: out of bound pointer"),
            getAddressInfo(*unbound, address));
        return;
      }
    }

    terminateStateOnProgramError(
        *unbound,
        new ErrorEvent(locationOf(*unbound), StateTerminationType::Ptr,
                       "memory error: out of bound pointer"),
        getAddressInfo(*unbound, address));
  }
}

ref<const MemoryObject> Executor::lazyInitializeObject(
    ExecutionState &state, ref<PointerExpr> address, const KInstruction *target,
    KType *targetType, uint64_t concreteSize, ref<Expr> size, bool isLocal,
    ref<Expr> conditionExpr, bool isSymbolic) {
  assert(!isa<ConstantPointerExpr>(address));
  std::pair<ref<const MemoryObject>, ref<Expr>> moBasePair;
  unsigned timestamp = 0;
  ref<Expr> base = address->getBase();
  if (state.getBase(base, moBasePair)) {
    timestamp = moBasePair.first->timestamp;
  }

  ref<Expr> sizeExpr;
  if (!isa<ConstantExpr>(size) &&
      (!MaxSymbolicAllocationSize ||
       concreteSize < MaxSymbolicAllocationSize) &&
      isSymbolic) {
    sizeExpr = size;
    ref<Expr> lowerBound =
        UgeExpr::create(sizeExpr, Expr::createPointer(concreteSize));
    ref<Expr> upperBound = Expr::createTrue();
    if (MaxSymbolicAllocationSize) {
      upperBound = UleExpr::create(
          sizeExpr, Expr::createPointer(MaxSymbolicAllocationSize));
    }
    bool mayBeInBounds;
    solver->setTimeout(coreSolverTimeout);
    bool success = solver->mayBeTrue(state.constraints.cs(),
                                     AndExpr::create(lowerBound, upperBound),
                                     mayBeInBounds, state.queryMetaData);
    solver->setTimeout(time::Span());
    if (!success) {
      return nullptr;
    }
    assert(mayBeInBounds);

    addConstraint(state, AndExpr::create(lowerBound, upperBound));
    conditionExpr =
        AddExpr::create(conditionExpr, AndExpr::create(lowerBound, upperBound));
  } else {
    sizeExpr = Expr::createPointer(concreteSize);
  }

  ref<Expr> addressExpr = address->getBase();
  MemoryObject *mo =
      allocate(state, sizeExpr, isLocal,
               /*isGlobal=*/false, CodeLocation::create(target, "", 0, {}),
               /*allocationAlignment=*/8, targetType, conditionExpr,
               addressExpr, timestamp);

  return mo;
}

void Executor::lazyInitializeLocalObject(ExecutionState &state, StackFrame &sf,
                                         ref<Expr> address,
                                         const KInstruction *target) {
  AllocaInst *ai = cast<AllocaInst>(target->inst());
  unsigned elementSize =
      kmodule->targetData->getTypeStoreSize(ai->getAllocatedType());
  ref<Expr> size = Expr::createPointer(elementSize);
  if (ai->isArrayAllocation()) {
    ref<Expr> count = eval(target, 0, state, sf).value;
    count = Expr::createZExtToPointerWidth(count);
    size = MulExpr::create(size, count);
    if (isa<ConstantExpr>(size)) {
      elementSize = cast<ConstantExpr>(size)->getZExtValue();
    } else {
      const Array *lazyInstantiationSize = makeArray(
          Expr::createPointer(Context::get().getPointerWidth() / CHAR_BIT),
          SourceBuilder::lazyInitializationSize(address));
      size = Expr::createTempRead(lazyInstantiationSize,
                                  Context::get().getPointerWidth());
    }
  }
  ref<Expr> conditionExpr = Expr::createTrue();
  ref<PointerExpr> pointer = PointerExpr::create(address, address);
  ref<const MemoryObject> id = lazyInitializeObject(
      state, pointer, target, typeSystemManager->getWrappedType(ai->getType()),
      elementSize, size, true, conditionExpr, UseSymbolicSizeLazyInit);
  state.addPointerResolution(pointer, id.get());
  state.addConstraint(EqExpr::create(address, id->getBaseExpr()));
  state.addConstraint(
      Expr::createIsZero(EqExpr::create(address, Expr::createPointer(0))));
  if (isa<ConstantExpr>(size)) {
    addConstraint(state, EqExpr::create(size, id->getSizeExpr()));
  }
  RefObjectPair op = state.addressSpace.findOrLazyInitializeObject(id.get());
  state.addressSpace.bindObject(op.first, op.second.get());
}

void Executor::lazyInitializeLocalObject(ExecutionState &state,
                                         ref<Expr> address,
                                         const KInstruction *target) {
  lazyInitializeLocalObject(state, state.stack.valueStack().back(), address,
                            target);
}

uint64_t Executor::updateNameVersion(ExecutionState &state,
                                     const std::string &name) {
  uint64_t id = 0;
  if (state.arrayNames.count(name)) {
    id = state.arrayNames[name];
  }
  state.arrayNames[name] = id + 1;
  return id;
}

const Array *Executor::makeArray(ref<Expr> size,
                                 ref<SymbolicSource> source) const {
  const Array *array = Array::create(size, source);
  return array;
}

ref<PointerExpr> Executor::makePointer(ref<Expr> expr) const {
  return PointerExpr::create(expr);
}

void Executor::executeMakeSymbolic(ExecutionState &state,
                                   const MemoryObject *mo, KType *type,
                                   const ref<SymbolicSource> source,
                                   bool isLocal) {
  // Create a new object state for the memory object (instead of a copy).
  if (!replayKTest) {
    const Array *array = makeArray(mo->getSizeExpr(), source);
    ObjectState *os = bindObjectInState(state, mo, type, isLocal, array);

    ref<ConstantExpr> sizeExpr =
        dyn_cast<ConstantExpr>(os->getObject()->getSizeExpr());
    if (AlignSymbolicPointers && sizeExpr) {
      if (ref<Expr> alignmentRestrictions = type->getContentRestrictions(
              os->read(0, sizeExpr->getZExtValue() * CHAR_BIT))) {
        addConstraint(state, alignmentRestrictions);
      }
    }
    state.addSymbolic(mo, array, type);

    std::map<ExecutionState *, std::vector<SeedInfo>>::iterator it =
        seedMap->find(&state);
    if (it != seedMap->end()) { // In seed mode we need to add this as a
                                // binding.
      for (std::vector<SeedInfo>::iterator siit = it->second.begin(),
                                           siie = it->second.end();
           siit != siie; ++siit) {
        SeedInfo &si = *siit;
        KTestObject *obj = si.getNextInput(mo, NamedSeedMatching);

        if (!obj) {
          if (ZeroSeedExtension) {
            si.assignment.bindings.replace(
                {array, SparseStorageImpl<unsigned char>(0)});
          } else if (!AllowSeedExtension) {
            terminateStateOnUserError(state,
                                      "ran out of inputs during seeding");
            break;
          }
        } else {
          ref<ConstantExpr> sizeExpr =
              dyn_cast<ConstantExpr>(mo->getSizeExpr());
          if (sizeExpr) {
            unsigned moSize = sizeExpr->getZExtValue();
            if (obj->numBytes != moSize &&
                ((!(AllowSeedExtension || ZeroSeedExtension) &&
                  obj->numBytes < moSize) ||
                 (!AllowSeedTruncation && obj->numBytes > moSize))) {
              std::stringstream msg;
              msg << "replace size mismatch: " << mo->name << "[" << moSize
                  << "]"
                  << " vs " << obj->name << "[" << obj->numBytes << "]"
                  << " in test\n";

              terminateStateOnUserError(state, msg.str());
              break;
            } else {
              SparseStorageImpl<unsigned char> values;
              if (si.assignment.bindings.find(array) !=
                  si.assignment.bindings.end()) {
                values = si.assignment.bindings.at(array);
              }
              values.store(0, obj->bytes,
                           obj->bytes + std::min(obj->numBytes, moSize));
              si.assignment.bindings.replace({array, values});
            }
          }
        }
      }
    }
  } else {
    ObjectState *os = bindObjectInState(state, mo, type, false);
    if (replayPosition >= replayKTest->numObjects) {
      terminateStateOnUserError(state, "replay count mismatch");
    } else {
      KTestObject *obj = &replayKTest->objects[replayPosition++];
      ref<ConstantExpr> sizeExpr = dyn_cast<ConstantExpr>(mo->getSizeExpr());
      if (sizeExpr && obj->numBytes != sizeExpr->getZExtValue()) {
        for (unsigned i = 0; i < sizeExpr->getZExtValue(); i++)
          os->write8(i, obj->bytes[i]);
      } else {
        terminateStateOnUserError(state, "replay size mismatch");
      }
    }
  }
}

/***/

ExecutionState *Executor::formState(Function *f) {
  ExecutionState *state = new ExecutionState(
      kmodule->functionMap[f], kmodule->functionMap[f]->blockMap[&*f->begin()]);
  initializeGlobals(*state);
  return state;
}

ExecutionState *Executor::formState(Function *f, int argc, char **argv,
                                    char **envp) {
  std::vector<ref<Expr>> arguments;

  // force deterministic initialization of memory objects
  srand(1);
  srandom(1);

  MemoryObject *argvMO = 0;

  // In order to make uclibc happy and be closer to what the system is
  // doing we lay out the environments at the end of the argv array
  // (both are terminated by a null). There is also a final terminating
  // null that uclibc seems to expect, possibly the ELF header?

  int envc;
  for (envc = 0; envp[envc]; ++envc)
    ;

  unsigned NumPtrBytes = Context::get().getPointerWidth() / 8;
  KFunction *kf = kmodule->functionMap[f];
  assert(kf);
  ExecutionState *state = new ExecutionState(kmodule->functionMap[f]);
  Function::arg_iterator ai = f->arg_begin(), ae = f->arg_end();
  if (ai != ae) {
    arguments.push_back(ConstantExpr::alloc(argc, Expr::Int32));
    if (++ai != ae) {
      // For the entry state assume that arguments allocated
      // on line with function signature.
      ref<CodeLocation> parameterLocation =
          CodeLocation::create(kf, kf->getSourceFilepath(), kf->getLine(), {});

      llvm::Type *argumentType = llvm::PointerType::get(
          llvm::IntegerType::get(kmodule->module->getContext(), 8),
          kmodule->targetData->getAllocaAddrSpace());

      llvm::Type *argvType = llvm::ArrayType::get(argumentType, 1);

      argvMO = allocate(
          *state, Expr::createPointer((argc + 1 + envc + 1 + 1) * NumPtrBytes),
          /*isLocal=*/false, /*isGlobal=*/true,
          /*allocSite=*/parameterLocation, /*alignment=*/8,
          typeSystemManager->getWrappedType(argvType));

      if (!argvMO)
        klee_error("Could not allocate memory for function arguments");

      arguments.push_back(
          PointerExpr::create(argvMO->getBaseExpr(), argvMO->getBaseExpr()));

      if (++ai != ae) {
        uint64_t envp_start = (argc + 1) * NumPtrBytes;
        arguments.push_back(AddExpr::create(
            PointerExpr::create(argvMO->getBaseExpr(), argvMO->getBaseExpr()),
            Expr::createPointer(envp_start)));

        if (++ai != ae)
          klee_error("invalid main function (expect 0-3 arguments)");
      }
    }
  }

  assert(arguments.size() == f->arg_size() && "wrong number of arguments");
  for (unsigned i = 0, e = f->arg_size(); i != e; ++i)
    bindArgument(kf, i, *state, arguments[i]);

  if (argvMO) {
    llvm::Type *argumentType = llvm::PointerType::get(
        llvm::IntegerType::get(kmodule->module->getContext(), 8),
        kmodule->targetData->getAllocaAddrSpace());

    llvm::Type *argvType = llvm::ArrayType::get(argumentType, 1);

    ObjectState *argvOS = bindObjectInState(
        *state, argvMO, typeSystemManager->getWrappedType(argvType), false);

    for (int i = 0; i < argc + 1 + envc + 1 + 1; i++) {
      if (i == argc || i >= argc + 1 + envc) {
        // Write NULL pointer
        argvOS->write(i * NumPtrBytes,
                      PointerExpr::create(Expr::createPointer(0),
                                          Expr::createPointer(0)));
      } else {
        char *s = i < argc ? argv[i] : envp[i - (argc + 1)];
        int j, len = strlen(s);

        ref<CodeLocation> parameterLocation = CodeLocation::create(
            kf, kf->getSourceFilepath(), kf->getLine(), {});

        MemoryObject *arg =
            allocate(*state, Expr::createPointer(len + 1), /*isLocal=*/false,
                     /*isGlobal=*/true,
                     /*allocSite=*/parameterLocation, /*alignment=*/8,
                     typeSystemManager->getWrappedType(argumentType));
        if (!arg)
          klee_error("Could not allocate memory for function arguments");

        ObjectState *os = bindObjectInState(
            *state, arg, typeSystemManager->getWrappedType(argumentType),
            false);
        for (j = 0; j < len + 1; j++)
          os->write8(j, s[j]);

        // Write pointer to newly allocated and initialised argv/envp c-string
        argvOS->write(i * NumPtrBytes, PointerExpr::create(arg->getBaseExpr(),
                                                           arg->getBaseExpr()));
      }
    }
  }

  initializeGlobals(*state);
  return state;
}

void Executor::clearGlobal() {
  globalObjects.clear();
  globalAddresses.clear();
}

void Executor::clearMemory() {
  // hack to clear memory objects
  memory = nullptr;
}

void Executor::runFunctionAsMain(Function *f, int argc, char **argv,
                                 char **envp) {
  if (FunctionCallReproduce == "" &&
      guidanceKind == GuidanceKind::ErrorGuidance &&
      (!interpreterOpts.Paths.has_value() || interpreterOpts.Paths->empty())) {
    klee_warning("No targets found in error-guided mode");
    return;
  }

  processForest = std::make_unique<PForest>();
  objectManager->addProcessForest(processForest.get());

  ExecutionState *state = formState(f, argc, argv, envp);
  bindModuleConstants(llvm::APFloat::rmNearestTiesToEven);

  KFunction *kEntryFunction = kmodule->functionMap.at(f);
  if (guidanceKind == GuidanceKind::ErrorGuidance) {
    ref<TargetForest> forest;
    if (FunctionCallReproduce == "") {
      auto &paths = interpreterOpts.Paths.value();
      forest = targetedExecutionManager->prepareTargets(
          kmodule.get(), kEntryFunction, std::move(paths));
    } else {
      /* Find all calls to function specified in .prp file
       * and combine them to single target forest */
      auto kfIt = kmodule->functionNameMap.find(FunctionCallReproduce);
      if (kfIt == kmodule->functionNameMap.end()) {
        klee_warning("%s was eliminated by LLVM passes, so it is unreachable",
                     FunctionCallReproduce.c_str());
      } else {
        auto kCallBlock = kfIt->second->entryKBlock;
        forest = new TargetForest(kEntryFunction);
        forest->add(ReproduceErrorTarget::create(
            {ReachWithError::Reachable}, "",
            ErrorLocation(kCallBlock->getFirstInstruction()), kCallBlock));
      }
    }

    if (forest->empty()) {
      klee_warning(
          "No targets found in error-guided mode after prepare targets");
      delete state;
      return;
    }

    targets.emplace(kEntryFunction, TargetedHaltsOnTraces(forest));
    prepareTargetedExecution(*state, forest);
  }

  objectManager->addInitialState(state);

  TreeOStream pathOS;
  TreeOStream symPathOS;
  if (pathWriter) {
    pathOS = pathWriter->open();
  }

  if (symPathWriter) {
    symPathOS = symPathWriter->open();
  }

  if (statsTracker)
    statsTracker->framePushed(*state, 0);

  if (pathWriter)
    state->pathOS = pathOS;
  if (symPathWriter)
    state->symPathOS = symPathOS;

  run(state);

  if (statsTracker)
    statsTracker->done();

  clearMemory();
  clearGlobal();

  if (statsTracker)
    statsTracker->done();
}

void Executor::prepareTargetedExecution(ExecutionState &initialState,
                                        ref<TargetForest> whitelist) {
  initialState.targetForest = *whitelist->deepCopy();
  initialState.setTargeted(true);
  initialState.setHistory(initialState.targetForest.getHistory());
  initialState.setTargets(initialState.targetForest.getTargets());
}

bool isReturnValueFromInitBlock(const ExecutionState &state,
                                const llvm::Value *value) {
  return state.initPC->parent->getKBlockType() == KBlockType::Call &&
         state.initPC == state.initPC->parent->getLastInstruction() &&
         state.initPC->parent->getFirstInstruction()->inst() == value;
}

ref<Expr> Executor::makeSymbolicValue(llvm::Value *value,
                                      ExecutionState &state) {
  assert(value && "Attempted to make symbolic value from nullptr Value");
  auto size = kmodule->targetData->getTypeStoreSize(value->getType());
  auto width = kmodule->targetData->getTypeSizeInBits(value->getType());
  bool argument = (isa<Argument>(value) ? true : false);
  assert(argument || isa<Instruction>(value));
  int index = isReturnValueFromInitBlock(state, value) ? -1 : 0;
  auto source = argument
                    ? SourceBuilder::argument(*dyn_cast<Argument>(value), index,
                                              kmodule.get())
                    : SourceBuilder::instruction(*dyn_cast<Instruction>(value),
                                                 index, kmodule.get());
  auto array = makeArray(Expr::createPointer(size), source);
  ref<Expr> result = Expr::createTempRead(array, width);
  if (X86FPAsX87FP80 && value->getType()->isFloatingPointTy() &&
      Context::get().getPointerWidth() == 32) {
    result = FPToX87FP80Ext(result);
  }
  return result;
}

void Executor::prepareSymbolicValue(ExecutionState &state, StackFrame &frame,
                                    KInstruction *target) {
  ref<Expr> result = makeSymbolicValue(target->inst(), state);
  if (isa<AllocaInst>(target->inst())) {
    lazyInitializeLocalObject(state, frame, result, target);
  }
  bindLocal(target, frame, result);
}

void Executor::prepareMockValue(ExecutionState &state, StackFrame &frame,
                                const std::string &name, Expr::Width width,
                                KInstruction *target) {
  ref<Expr> result = makeMockValue(name, width);
  bindLocal(target, frame, result);
  if (isa<AllocaInst>(target->inst())) {
    lazyInitializeLocalObject(state, frame, result, target);
  }
}

void Executor::prepareMockValue(ExecutionState &state, const std::string &name,
                                llvm::Type *type, KInstruction *target) {
  Expr::Width width = kmodule->targetData->getTypeSizeInBits(type);
  prepareMockValue(state, state.stack.valueStack().back(), name, width, target);
}

void Executor::prepareSymbolicValue(ExecutionState &state,
                                    KInstruction *target) {
  prepareSymbolicValue(state, state.stack.valueStack().back(), target);
}

void Executor::prepareSymbolicRegister(ExecutionState &state, StackFrame &sf,
                                       unsigned regNum) {
  KInstruction *allocInst = sf.kf->getInstructionByRegister(regNum);
  prepareSymbolicValue(state, sf, allocInst);
}

void Executor::prepareSymbolicArg(ExecutionState &state, StackFrame &frame,
                                  unsigned index) {
  KFunction *kf = frame.kf;
#if LLVM_VERSION_CODE >= LLVM_VERSION(10, 0)
  Argument *arg = kf->function()->getArg(index);
#else
  Argument *arg = &kf->function()->arg_begin()[index];
#endif
  ref<Expr> result = makeSymbolicValue(arg, state);
  bindArgument(kf, index, frame, result);
}

void Executor::prepareSymbolicArgs(ExecutionState &state, StackFrame &frame) {
  KFunction *kf = frame.kf;
  unsigned argSize = kf->function()->arg_size();
  for (unsigned argNo = 0; argNo < argSize; ++argNo) {
    prepareSymbolicArg(state, frame, argNo);
  }
}

void Executor::prepareSymbolicArgs(ExecutionState &state) {
  prepareSymbolicArgs(state, state.stack.valueStack().back());
}

unsigned Executor::getPathStreamID(const ExecutionState &state) {
  assert(pathWriter);
  return state.pathOS.getID();
}

unsigned Executor::getSymbolicPathStreamID(const ExecutionState &state) {
  assert(symPathWriter);
  return state.symPathOS.getID();
}

void Executor::getConstraintLog(const ExecutionState &state, std::string &res,
                                Interpreter::LogType logFormat) {
  switch (logFormat) {
  case STP: {
    auto query = state.toQuery();
    char *log = solver->getConstraintLog(query);
    res = std::string(log);
    free(log);
  } break;

  case KQUERY: {
    std::string Str;
    llvm::raw_string_ostream info(Str);
    ExprPPrinter::printConstraints(info, state.constraints.cs());
    res = info.str();
  } break;

  case SMTLIB2: {
    std::string Str;
    llvm::raw_string_ostream info(Str);
    ExprSMTLIBPrinter printer;
    printer.setOutput(info);
    auto query = state.toQuery();
    printer.setQuery(query);
    printer.generateOutput();
    res = info.str();
  } break;

  default:
    klee_warning("Executor::getConstraintLog() : Log format not supported!");
  }
}

void Executor::addSARIFReport(const ExecutionState &state) {
  ResultJson result{};

  CodeFlowJson codeFlow = state.eventsRecorder.serialize();

  if (ref<ErrorEvent> lastEvent =
          llvm::dyn_cast<ErrorEvent>(state.eventsRecorder.last())) {
    result.locations.push_back(lastEvent->serialize());
    result.message = {Message{lastEvent->message}};
    result.ruleId = {terminationTypeName(lastEvent->ruleID)};
    result.level = {"error"};
  }

  result.codeFlows.push_back(std::move(codeFlow));

  sarifReport.runs.back().results.push_back(std::move(result));
}

SarifReportJson Executor::getSARIFReport() const { return sarifReport; }

void Executor::logState(const ExecutionState &state, int id,
                        std::unique_ptr<llvm::raw_fd_ostream> &f) {
  *f << "State number " << state.id << ". Test number: " << id << "\n\n";
  for (auto &object : state.addressSpace.objects) {
    *f << "ID memory object: " << object.first->id << "\n";
    *f << "Address memory object: ";
    object.first->getBaseExpr()->print(*f);
    *f << "\n";
    *f << "Memory object size: ";
    object.first->getSizeExpr()->print(*f);
    *f << "\n";
  }
  *f << state.symbolics.size() << " symbolics total. "
     << "Symbolics:\n";
  size_t sc = 0;
  for (const auto &symbolic : state.symbolics) {
    *f << "Symbolic number " << sc++ << "\n";
    *f << "Associated memory object: " << symbolic.memoryObject.get()->id
       << "\n";
    *f << "Memory object size: ";
    symbolic.memoryObject.get()->getSizeExpr()->print(*f);
    *f << "\n";
  }
  *f << "\n";
  *f << "State constraints:\n";
  for (auto constraint : state.constraints.cs().cs()) {
    constraint->print(*f);
    *f << "\n";
  }
}

bool resolveOnSymbolics(const std::vector<klee::Symbolic> &symbolics,
                        const Assignment &assn,
                        const ref<klee::ConstantPointerExpr> &addr,
                        ref<const MemoryObject> &result) {
  uint64_t base = addr->getConstantBase()->getZExtValue();
  AssignmentEvaluator evaluator(assn, true);

  for (const auto &res : symbolics) {
    const auto &mo = res.memoryObject;
    // Check if the provided address is between start and end of the object
    // [mo->address, mo->address + mo->size) or the object is a 0-sized object.
    ref<klee::ConstantExpr> moSize =
        cast<klee::ConstantExpr>(evaluator.visit(mo->getSizeExpr()));
    ref<klee::ConstantExpr> moAddress =
        cast<klee::ConstantExpr>(evaluator.visit(mo->getBaseExpr()));
    ref<klee::ConstantExpr> moCondition =
        cast<klee::ConstantExpr>(evaluator.visit(mo->getConditionExpr()));
    if (((base == moAddress->getZExtValue())) && moCondition->getZExtValue(1)) {
      result = mo;
      return true;
    }
  }

  return false;
}

void resolvePointer(const ExecutionState &state,
                    const std::vector<klee::Symbolic> &symbolics,
                    const Assignment &model,
                    ExprHashMap<std::pair<ref<const MemoryObject>, ref<Expr>>>
                        &resolvedPointers,
                    ref<PointerExpr> pointer) {
  ref<Expr> pointerInModel = model.evaluate(pointer, false);
  if (!isa<ConstantPointerExpr>(pointerInModel)) {
    return;
  }
  ref<ConstantPointerExpr> constantPointer =
      cast<ConstantPointerExpr>(pointerInModel);
  ref<const MemoryObject> mo;

  if (resolveOnSymbolics(symbolics, model, constantPointer, mo)) {
    ref<Expr> moOffset = mo->getOffsetExpr(pointer);
    if (SimplifySymIndices) {
      if (!isa<klee::ConstantExpr>(moOffset)) {
        moOffset = Simplificator::simplifyExpr(state.constraints.cs(), moOffset)
                       .simplified;
      }
    }
    resolvedPointers[pointer] = std::make_pair(mo, moOffset);
  }
}

void Executor::setInitializationGraph(
    const ExecutionState &state, const std::vector<klee::Symbolic> &symbolics,
    const Assignment &model, KTest &ktest) {
  std::map<size_t, std::vector<Pointer>> pointers;
  std::map<size_t, std::map<uint64_t, std::pair<uint64_t, uint64_t>>> s;
  ExprHashMap<std::pair<ref<const MemoryObject>, ref<Expr>>> resolvedPointers;

  ExprHashSet symbolicPointers;
  for (auto &constraint : state.constraints.cs().cs()) {
    klee::findSymbolicPointers(constraint, true, symbolicPointers,
                               Context::get().getPointerWidth());
  }

  for (auto &symbolicPointer : symbolicPointers) {
    ref<ReadExpr> read = symbolicPointer->hasOrderedReads();
    auto source = read->updates.root->source;
    if (isa<MakeSymbolicSource>(source) ||
        isa<LazyInitializationContentSource>(source)) {
      resolvePointer(state, symbolics, model, resolvedPointers,
                     PointerExpr::create(symbolicPointer));
    }
  }

  for (const auto &pointer : resolvedPointers) {

    if (!isa<PointerExpr>(pointer.first)) {
      continue;
    }
    ref<PointerExpr> pointerExpr = cast<PointerExpr>(pointer.first);
    ref<Expr> updateCheck;
    if (auto e = dyn_cast<ConcatExpr>(pointerExpr->getBase())) {
      updateCheck = e->getLeft();
    } else {
      updateCheck = e;
    }

    if (auto e = dyn_cast<ReadExpr>(updateCheck)) {
      if (e->updates.getSize() != 0) {
        continue;
      }
    }

    std::pair<ref<const MemoryObject>, ref<Expr>> pointerResolution;
    auto resolved = state.getBase(pointerExpr->getBase(), pointerResolution);

    if (resolved) {
      // The objects have to be symbolic
      bool pointerFound = false, pointeeFound = false;
      size_t pointerIndex = 0, pointeeIndex = 0;
      size_t i = 0;
      for (auto &symbolic : symbolics) {
        if (symbolic.memoryObject == pointerResolution.first) {
          pointerIndex = i;
          pointerFound = true;
        }
        if (symbolic.memoryObject->id == pointer.second.first->id) {
          pointeeIndex = i;
          pointeeFound = true;
        }
        ++i;
      }
      if (pointerFound && pointeeFound) {
        ref<ConstantExpr> offset =
            model.evaluate(pointerResolution.second, false);
        ref<ConstantExpr> indexOffset =
            model.evaluate(pointer.second.second, false);
        Pointer o;
        o.offset = cast<ConstantExpr>(offset)->getZExtValue();
        o.index = pointeeIndex;
        o.indexOffset = cast<ConstantExpr>(indexOffset)->getZExtValue();
        if (s[pointerIndex].count(o.offset) &&
            s[pointerIndex][o.offset] !=
                std::make_pair(o.index, o.indexOffset)) {
          assert(0 && "unreachable");
        }
        if (!s[pointerIndex].count(o.offset)) {
          pointers[pointerIndex].push_back(o);
          s[pointerIndex][o.offset] = std::make_pair(o.index, o.indexOffset);
        }
      }
    }
  }
  for (auto i : pointers) {
    ktest.objects[i.first].numPointers = i.second.size();
    ktest.objects[i.first].pointers = new Pointer[i.second.size()];
    for (size_t j = 0; j < i.second.size(); j++) {
      ktest.objects[i.first].pointers[j] = i.second[j];
    }
  }
  return;
}

bool isReproducible(const klee::Symbolic &symb) {
  auto arr = symb.array;
  bool bad = IrreproducibleSource::classof(arr->source.get());
  if (auto liSource = dyn_cast<LazyInitializationSource>(arr->source.get())) {
    std::vector<const Array *> arrays;
    findObjects(liSource->pointer, arrays);
    for (auto innerArr : arrays) {
      bad |= IrreproducibleSource::classof(innerArr->source.get());
    }
  }
  if (bad)
    klee_warning_once(arr->source.get(),
                      "A irreproducible symbolic %s reaches a test",
                      arr->getIdentifier().c_str());
  return !bad;
}

bool isUninitialized(const klee::Array *array) {
  bool bad = isa<UninitializedSource>(array->source);
  if (bad)
    klee_warning_once(array->source.get(),
                      "A uninitialized array %s reaches a test",
                      array->getIdentifier().c_str());
  return bad;
}

bool isMakeSymbolic(const klee::Symbolic &symb) {
  auto array = symb.array;
  bool good = isa<MakeSymbolicSource>(array->source);
  if (!good)
    klee_warning_once(array->source.get(),
                      "A not make_symbolic array %s reaches a test",
                      array->getIdentifier().c_str());
  return good;
}

bool Executor::getSymbolicSolution(const ExecutionState &state, KTest &res) {
  solver->setTimeout(coreSolverTimeout);

  PathConstraints extendedConstraints(state.constraints);

  // Go through each byte in every test case and attempt to restrict
  // it to the constraints contained in cexPreferences.  (Note:
  // usually this means trying to make it an ASCII character (0-127)
  // and therefore human readable. It is also possible to customize
  // the preferred constraints.  See test/Features/PreferCex.c for
  // an example) While this process can be very expensive, it can
  // also make understanding individual test cases much easier.
  const size_t cexPreferencesBound = 16;
  if (state.cexPreferences.size() > cexPreferencesBound) {
    klee_warning_once(0, "skipping cex preffering (size of restrictons > %zu).",
                      cexPreferencesBound);
  } else {
    for (auto &pi : state.cexPreferences) {
      bool mustBeTrue;
      // Attempt to bound byte to constraints held in cexPreferences
      bool success =
          solver->mustBeTrue(extendedConstraints.cs(), Expr::createIsZero(pi),
                             mustBeTrue, state.queryMetaData);
      // If it isn't possible to add the condition without making the entire
      // list UNSAT, then just continue to the next condition
      if (!success)
        break;
      // If the particular constraint operated on in this iteration through
      // the loop isn't implied then add it to the list of constraints.
      if (!mustBeTrue) {
        extendedConstraints.addConstraint(pi);
      }
    }
  }

  std::vector<const Array *> allObjects;
  findSymbolicObjects(state.constraints.cs().cs().begin(),
                      state.constraints.cs().cs().end(), allObjects);
  std::vector<const Array *> uninitObjects;
  std::copy_if(allObjects.begin(), allObjects.end(),
               std::back_inserter(uninitObjects), isUninitialized);

  std::vector<klee::Symbolic> symbolics;

  if (OnlyOutputMakeSymbolicArrays) {
    std::copy_if(state.symbolics.begin(), state.symbolics.end(),
                 std::back_inserter(symbolics), isMakeSymbolic);
  } else {
    std::copy_if(state.symbolics.begin(), state.symbolics.end(),
                 std::back_inserter(symbolics), isReproducible);
  }

  // we cannot be sure that an irreproducible state proves the presence of an
  // error
  if (uninitObjects.size() > 0 || state.symbolics.size() != symbolics.size()) {
    state.error = ReachWithError::None;
  } else if (FunctionCallReproduce != "" &&
             state.error == ReachWithError::Reachable) {
    setHaltExecution(HaltExecution::ReachedTarget);
  }

  std::unordered_set<const Array *> objectSet;
  for (auto &symbolic : symbolics) {
    objectSet.insert(symbolic.array);
  }
  for (auto &array : allObjects) {
    objectSet.insert(array);
  }

  std::vector<const Array *> objects(objectSet.begin(), objectSet.end());

  ref<SolverResponse> response;
  solver->setTimeout(coreSolverTimeout);
  bool success =
      solver->getResponse(extendedConstraints.cs(), Expr::createFalse(),
                          response, state.queryMetaData);
  solver->setTimeout(time::Span());
  if (!success || !isa<InvalidResponse>(response)) {
    klee_warning("unable to compute initial values (invalid constraints?)!");
    ExprPPrinter::printQuery(llvm::errs(), state.constraints.cs(),
                             ConstantExpr::alloc(0, Expr::Bool));
    return false;
  }
  assert(isa<InvalidResponse>(response));

  Assignment model = cast<InvalidResponse>(response)->initialValuesFor(objects);
  AssignmentEvaluator evaluator(model, false);

  Expr::Width pointerWidthInBits = Context::get().getPointerWidth();

  std::vector<ref<Expr>> symbolicSizesTerms;

  /* Collect all size symcretes. */
  for (ref<Symcrete> symcrete : extendedConstraints.cs().symcretes()) {
    if (isa<SizeSymcrete>(symcrete)) {
      symbolicSizesTerms.push_back(
          ZExtExpr::create(symcrete->symcretized, pointerWidthInBits));
    }
  }

  if (symbolicSizesTerms.size() > 0) {
    ref<Expr> symbolicSizesSum =
        createNonOverflowingSumExpr(symbolicSizesTerms);

    std::vector<const Array *> symcreteObjects;
    std::vector<SparseStorageImpl<unsigned char>> symcreteValues;
    bool success =
        computeSizes(extendedConstraints.cs(), symbolicSizesSum,
                     symcreteObjects, symcreteValues, state.queryMetaData);
    if (success) {
      Assignment symcreteModel = Assignment(symcreteObjects, symcreteValues);

      for (auto &i : model.diffWith(symcreteModel).bindings) {
        model.bindings.replace({i.first, i.second});
      }
    }

    ExprHashMap<ref<Expr>> concretizations;

    for (ref<Symcrete> symcrete : extendedConstraints.cs().symcretes()) {
      concretizations[symcrete->symcretized] =
          evaluator.visit(symcrete->symcretized);
    }

    std::vector<void *> addresses;

    for (const ref<Symcrete> &symcrete : extendedConstraints.cs().symcretes()) {
      ref<SizeSymcrete> sizeSymcrete = dyn_cast<SizeSymcrete>(symcrete);

      if (!sizeSymcrete) {
        continue;
      }

      ref<Expr> symcretizedAddress = sizeSymcrete->addressSymcrete.symcretized;

      /* Receive address array linked with this size array to request address
       * concretization. */
      ref<Expr> condcretized = concretizations.at(symcrete->symcretized);

      uint64_t newSize = cast<ConstantExpr>(condcretized)->getZExtValue();

      void *address = malloc(newSize);
      addresses.push_back(address);
      unsigned char *charAddressIterator =
          reinterpret_cast<unsigned char *>(&address);
      SparseStorageImpl<unsigned char> storage(0);
      storage.store(0, charAddressIterator,
                    charAddressIterator + sizeof(address));
      ref<ConstantExpr> constantAddress = ConstantExpr::create(
          reinterpret_cast<uint64_t>(address), symcretizedAddress->getWidth());

      concretizations[symcretizedAddress] = constantAddress;
    }

    ref<Expr> concretizationCondition = Expr::createFalse();
    for (const auto &concretization : concretizations) {
      solver->setTimeout(coreSolverTimeout);
      success = solver->getResponse(
          extendedConstraints.cs(),
          OrExpr::create(Expr::createIsZero(EqExpr::create(
                             concretization.first, concretization.second)),
                         concretizationCondition),
          response, state.queryMetaData);
      solver->setTimeout(time::Span());

      if (auto invalidResponse = dyn_cast<InvalidResponse>(response)) {
        concretizationCondition =
            OrExpr::create(Expr::createIsZero(EqExpr::create(
                               concretization.first, concretization.second)),
                           concretizationCondition);
        model = invalidResponse->initialValuesFor(objects);
      } else {
        llvm::errs();
      }
    }

    for (auto addr : addresses) {
      free(addr);
    }
  }

  res.numObjects = symbolics.size();
  res.objects = new KTestObject[res.numObjects];
  res.uninitCoeff = uninitObjects.size() * UninitMemoryTestMultiplier;

  {
    size_t i = 0;
    // Remove mo->size, evaluate size expr in array
    for (auto &symbolic : symbolics) {
      auto mo = symbolic.memoryObject;
      auto &values = model.bindings.at(symbolic.array);
      KTestObject *o = &res.objects[i];
      o->name = const_cast<char *>(mo->name.c_str());
      o->address = cast<ConstantExpr>(evaluator.visit(mo->getBaseExpr()))
                       ->getZExtValue();
      o->numBytes = cast<ConstantExpr>(evaluator.visit(mo->getSizeExpr()))
                        ->getZExtValue();
      o->bytes = new unsigned char[o->numBytes];
      for (size_t j = 0; j < o->numBytes; j++) {
        o->bytes[j] = values.load(j);
      }
      o->numPointers = 0;
      o->pointers = nullptr;
      i++;
    }
  }

  setInitializationGraph(state, symbolics, model, res);

  return true;
}

void Executor::getCoveredLines(const ExecutionState &state,
                               std::map<std::string, std::set<unsigned>> &res) {
  res = state.coveredLines;
}

void Executor::getBlockPath(const ExecutionState &state,
                            std::string &blockPath) {
  blockPath = state.constraints.path().toString();
}

void Executor::doImpliedValueConcretization(ExecutionState &state, ref<Expr> e,
                                            ref<ConstantExpr> value) {
  abort(); // FIXME: Broken until we sort out how to do the write back.

  if (DebugCheckForImpliedValues)
    ImpliedValue::checkForImpliedValues(solver->solver.get(), e, value);

  ImpliedValueList results;
  ImpliedValue::getImpliedValues(e, value, results);
  for (ImpliedValueList::iterator it = results.begin(), ie = results.end();
       it != ie; ++it) {
    ReadExpr *re = it->first.get();

    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(re->index)) {
      // FIXME: This is the sole remaining usage of the Array object
      // variable. Kill me.
      const MemoryObject *mo = 0; // re->updates.root->object;
      const ObjectState *os = state.addressSpace.findObject(mo).second;

      if (!os) {
        // object has been free'd, no need to concretize (although as
        // in other cases we would like to concretize the outstanding
        // reads, but we have no facility for that yet)
      } else {
        assert(!os->readOnly &&
               "not possible? read only object with static read?");
        ObjectState *wos = state.addressSpace.getWriteable(mo, os);
        maxNewWriteableOSSize =
            std::max(maxNewWriteableOSSize, wos->getSparseStorageEntries());
        wos->write(CE, it->second);
      }
    }
  }
}

Expr::Width Executor::getWidthForLLVMType(llvm::Type *type) const {
  return kmodule->targetData->getTypeSizeInBits(type);
}

size_t Executor::getAllocationAlignment(const llvm::Value *allocSite) const {
  // FIXME: 8 was the previous default. We shouldn't hard code this
  // and should fetch the default from elsewhere.
  const size_t forcedAlignment = 8;
  size_t alignment = 0;
  llvm::Type *type = NULL;
  std::string allocationSiteName(allocSite->getName().str());
  if (const GlobalObject *GO = dyn_cast<GlobalObject>(allocSite)) {
    alignment = GO->getAlignment();
    if (const GlobalVariable *globalVar = dyn_cast<GlobalVariable>(GO)) {
      // All GlobalVariables's have pointer type
      assert(globalVar->getType()->isPointerTy() &&
             "globalVar's type is not a pointer");
      type = globalVar->getValueType();
    } else {
      type = GO->getType();
    }
  } else if (const AllocaInst *AI = dyn_cast<AllocaInst>(allocSite)) {
    alignment = AI->getAlignment();
    type = AI->getAllocatedType();
  } else if (isa<InvokeInst>(allocSite) || isa<CallInst>(allocSite)) {
    // FIXME: Model the semantics of the call to use the right alignment
    const CallBase &cb = cast<CallBase>(*allocSite);
    llvm::Function *fn =
        klee::getDirectCallTarget(cb, /*moduleIsFullyLinked=*/true);
    if (fn)
      allocationSiteName = fn->getName().str();

    klee_warning_once(fn != NULL ? fn : allocSite,
                      "Alignment of memory from call \"%s\" is not "
                      "modelled. Using alignment of %zu.",
                      allocationSiteName.c_str(), forcedAlignment);
    alignment = forcedAlignment;
  } else {
    llvm_unreachable("Unhandled allocation site");
  }

  if (alignment == 0) {
    assert(type != NULL);
    // No specified alignment. Get the alignment for the type.
    if (type->isSized()) {
      alignment = kmodule->targetData->getPrefTypeAlignment(type);
    } else {
      klee_warning_once(allocSite,
                        "Cannot determine memory alignment for "
                        "\"%s\". Using alignment of %zu.",
                        allocationSiteName.c_str(), forcedAlignment);
      alignment = forcedAlignment;
    }
  }

  // Currently we require alignment be a power of 2
  if (!bits64::isPowerOfTwo(alignment)) {
    klee_warning_once(allocSite,
                      "Alignment of %zu requested for %s but this "
                      "not supported. Using alignment of %zu",
                      alignment, allocSite->getName().str().c_str(),
                      forcedAlignment);
    alignment = forcedAlignment;
  }
  assert(bits64::isPowerOfTwo(alignment) &&
         "Returned alignment must be a power of two");
  return alignment;
}

void Executor::prepareForEarlyExit() {
  if (statsTracker) {
    // Make sure stats get flushed out
    statsTracker->done();
  }
}

/// Returns the errno location in memory
int *Executor::getErrnoLocation() const {
#if !defined(__APPLE__) && !defined(__FreeBSD__)
  /* From /usr/include/errno.h: it [errno] is a per-thread variable. */
  return __errno_location();
#else
  return __error();
#endif
}

void Executor::dumpPForest() {
  if (!::dumpPForest)
    return;

  char name[32];
  snprintf(name, sizeof(name), "pforest%08d.dot", (int)stats::instructions);
  auto os = interpreterHandler->openOutputFile(name);
  if (os) {
    processForest->dump(*os);
  }

  ::dumpPForest = 0;
}

void Executor::dumpStates() {
  if (!::dumpStates)
    return;

  auto os = interpreterHandler->openOutputFile("states.txt");

  if (os) {
    for (ExecutionState *es : objectManager->getStates()) {
      *os << "(" << es << ",";
      *os << "[";
      auto next = es->stack.callStack().begin();
      ++next;
      for (auto sfIt = es->stack.callStack().begin(),
                sf_ie = es->stack.callStack().end();
           sfIt != sf_ie; ++sfIt) {
        *os << "('" << sfIt->kf->function()->getName().str() << "',";
        if (next == es->stack.callStack().end()) {
          *os << es->prevPC->getLine() << "), ";
        } else {
          *os << next->caller->getLine() << "), ";
          ++next;
        }
      }
      *os << "], ";

      InfoStackFrame &sf = es->stack.infoStack().back();
      uint64_t md2u =
          computeMinDistToUncovered(es->pc, sf.minDistToUncoveredOnReturn);
      uint64_t icnt = theStatisticManager->getIndexedValue(
          stats::instructions, es->pc->getGlobalIndex());
      uint64_t cpicnt =
          sf.callPathNode->statistics.getValue(stats::instructions);

      *os << "{";
      *os << "'depth' : " << es->depth << ", ";
      *os << "'queryCost' : " << es->queryMetaData.queryCost << ", ";
      *os << "'coveredNew' : " << es->isCoveredNew() << ", ";
      *os << "'instsSinceCovNew' : " << es->instsSinceCovNew << ", ";
      *os << "'md2u' : " << md2u << ", ";
      *os << "'icnt' : " << icnt << ", ";
      *os << "'CPicnt' : " << cpicnt << ", ";
      *os << "}";
      *os << ")\n";
    }
  }

  ::dumpStates = 0;
}

///

/// @brief Determines current code location for given state.
/// @param state given state.
/// @return constructed code location for given state.
///
/// Note: location tied with state as source code location
/// depends on current call stack.
ref<CodeLocation> Executor::locationOf(const ExecutionState &state) const {
  // Iterate over call stack frames until find function from
  // main module.

  // Either caller from call stack or executed instruction
  // in case of last stack frame.
  const KInstruction *kinst = &*state.prevPC;

  for (auto stackFrameIt = state.stack.callStack().rbegin(),
            stackBottom = state.stack.callStack().rend();
       stackFrameIt != stackBottom && !kmodule->inMainModule(*kinst->inst());
       kinst = stackFrameIt++->caller) {
  }

  if (!kinst) {
    return nullptr;
  }

  if (llvm::isa<llvm::ReturnInst>(kinst->inst())) {
    std::optional<ref<CodeLocation>> returnLocation =
        state.stack.forcedReturnLocation();
    if (returnLocation.has_value()) {
      return returnLocation.value();
    }
  }

  Path::PathIndex callPathIndex = state.constraints.path().getCurrentIndex();
  return CodeLocation::create(callPathIndex, kinst, kinst->getSourceFilepath(),
                              kinst->getLine(), kinst->getColumn());
}

Interpreter *Interpreter::create(LLVMContext &ctx,
                                 const InterpreterOptions &opts,
                                 InterpreterHandler *ih) {
  return new Executor(ctx, opts, ih);
}
