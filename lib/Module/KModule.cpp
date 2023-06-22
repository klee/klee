//===-- KModule.cpp -------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "KModule"

#include "Passes.h"

#include "klee/Config/Version.h"
#include "klee/Core/Interpreter.h"
#include "klee/Module/Cell.h"
#include "klee/Module/InstructionInfoTable.h"
#include "klee/Module/KInstruction.h"
#include "klee/Module/KModule.h"
#include "klee/Support/Debug.h"
#include "klee/Support/ErrorHandling.h"
#include "klee/Support/ModuleUtil.h"
#include "klee/Support/OptionCategories.h"

#include "llvm/Bitcode/BitcodeWriter.h"
#if LLVM_VERSION_CODE < LLVM_VERSION(8, 0)
#include "llvm/IR/CallSite.h"
#endif
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/ValueSymbolTable.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Linker/Linker.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_os_ostream.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Scalar.h"
#if LLVM_VERSION_CODE >= LLVM_VERSION(8, 0)
#include "llvm/Transforms/Scalar/Scalarizer.h"
#endif
#include "llvm/Transforms/Utils/Cloning.h"
#if LLVM_VERSION_CODE >= LLVM_VERSION(7, 0)
#include "llvm/Transforms/Utils.h"
#endif

#include <sstream>

using namespace llvm;
using namespace klee;

namespace klee {
cl::OptionCategory
    ModuleCat("Module-related options",
              "These options affect the compile-time processing of the code.");
}

namespace {
enum SwitchImplType { eSwitchTypeSimple, eSwitchTypeLLVM, eSwitchTypeInternal };

cl::opt<bool> OutputSource(
    "output-source",
    cl::desc(
        "Write the assembly for the final transformed source (default=true)"),
    cl::init(true), cl::cat(ModuleCat));

cl::opt<bool>
    OutputModule("output-module",
                 cl::desc("Write the bitcode for the final transformed module"),
                 cl::init(false), cl::cat(ModuleCat));

cl::opt<SwitchImplType> SwitchType(
    "switch-type",
    cl::desc("Select the implementation of switch (default=internal)"),
    cl::values(clEnumValN(eSwitchTypeSimple, "simple",
                          "lower to ordered branches"),
               clEnumValN(eSwitchTypeLLVM, "llvm", "lower using LLVM"),
               clEnumValN(eSwitchTypeInternal, "internal",
                          "execute switch internally")),
    cl::init(eSwitchTypeInternal), cl::cat(ModuleCat));

cl::opt<bool> DebugPrintEscapingFunctions(
    "debug-print-escaping-functions",
    cl::desc("Print functions whose address is taken (default=false)"),
    cl::cat(ModuleCat));

// For testing rounding mode only
cl::opt<bool> UseKleeFloatInternals(
    "float-internals",
    cl::desc("Use KLEE internal functions for floating-point"), cl::init(true));

// Don't run VerifierPass when checking module
cl::opt<bool>
    DontVerify("disable-verify",
               cl::desc("Do not verify the module integrity (default=false)"),
               cl::init(false), cl::cat(klee::ModuleCat));

cl::opt<bool> UseKleeFERoundInternals(
    "feround-internals",
    cl::desc("USE KLEE internal functions for passing rounding mode to "
             "external calls"),
    cl::init(true));

cl::opt<bool> OptimiseKLEECall("klee-call-optimisation",
                               cl::desc("Allow optimization of functions that "
                                        "contain KLEE calls (default=true)"),
                               cl::init(true), cl::cat(ModuleCat));

cl::opt<bool>
    SplitCalls("split-calls",
               cl::desc("Split each call in own basic block (default=true)"),
               cl::init(true), cl::cat(klee::ModuleCat));

cl::opt<bool> SplitReturns(
    "split-returns",
    cl::desc("Split each return in own basic block (default=true)"),
    cl::init(true), cl::cat(klee::ModuleCat));
} // namespace

/***/

namespace llvm {
extern void Optimize(Module *, llvm::ArrayRef<const char *> preservedFunctions);
}

// what a hack
static Function *getStubFunctionForCtorList(Module *m, GlobalVariable *gv,
                                            std::string name) {
  assert(!gv->isDeclaration() && !gv->hasInternalLinkage() &&
         "do not support old LLVM style constructor/destructor lists");

  std::vector<Type *> nullary;

  Function *fn = Function::Create(
      FunctionType::get(Type::getVoidTy(m->getContext()), nullary, false),
      GlobalVariable::InternalLinkage, name, m);
  BasicBlock *bb = BasicBlock::Create(m->getContext(), "entry", fn);
  llvm::IRBuilder<> Builder(bb);

  // From lli:
  // Should be an array of '{ int, void ()* }' structs.  The first value is
  // the init priority, which we ignore.
  auto arr = dyn_cast<ConstantArray>(gv->getInitializer());
  if (arr) {
    for (unsigned i = 0; i < arr->getNumOperands(); i++) {
      auto cs = cast<ConstantStruct>(arr->getOperand(i));
      // There is a third element in global_ctor elements (``i8 @data``).
#if LLVM_VERSION_CODE >= LLVM_VERSION(9, 0)
      assert(cs->getNumOperands() == 3 &&
             "unexpected element in ctor initializer list");
#else
      // before LLVM 9.0, the third operand was optional
      assert((cs->getNumOperands() == 2 || cs->getNumOperands() == 3) &&
             "unexpected element in ctor initializer list");
#endif
      auto fp = cs->getOperand(1);
      if (!fp->isNullValue()) {
        if (auto ce = dyn_cast<llvm::ConstantExpr>(fp))
          fp = ce->getOperand(0);

        if (auto f = dyn_cast<Function>(fp)) {
          Builder.CreateCall(f);
        } else {
          assert(0 &&
                 "unable to get function pointer from ctor initializer list");
        }
      }
    }
  }

  Builder.CreateRetVoid();

  return fn;
}

static void
injectStaticConstructorsAndDestructors(Module *m,
                                       llvm::StringRef entryFunction) {
  GlobalVariable *ctors = m->getNamedGlobal("llvm.global_ctors");
  GlobalVariable *dtors = m->getNamedGlobal("llvm.global_dtors");

  if ((!ctors && !dtors) || entryFunction.empty())
    return;

  Function *mainFn = m->getFunction(entryFunction);
  if (!mainFn)
    klee_error("Entry function '%s' not found in module.",
               entryFunction.str().c_str());

  if (ctors) {
    llvm::IRBuilder<> Builder(&*mainFn->begin()->begin());
    Builder.CreateCall(getStubFunctionForCtorList(m, ctors, "klee.ctor_stub"));
  }

  if (dtors) {
    Function *dtorStub = getStubFunctionForCtorList(m, dtors, "klee.dtor_stub");
    for (Function::iterator it = mainFn->begin(), ie = mainFn->end(); it != ie;
         ++it) {
      if (isa<ReturnInst>(it->getTerminator())) {
        llvm::IRBuilder<> Builder(it->getTerminator());
        Builder.CreateCall(dtorStub);
      }
    }
  }
}

void KModule::addInternalFunction(const char *functionName) {
  Function *internalFunction = module->getFunction(functionName);
  if (!internalFunction) {
    KLEE_DEBUG(klee_warning("Failed to add internal function %s. Not found.",
                            functionName));
    return;
  }
  KLEE_DEBUG(klee_message("Added function %s.", functionName));
  internalFunctions.insert(internalFunction);
}

bool KModule::link(std::vector<std::unique_ptr<llvm::Module>> &modules,
                   unsigned flags) {
  std::string error;
  if (!module) {
    module = std::move(modules.front());
  }
  if (!klee::linkModules(module.get(), modules, flags, error)) {
    klee_error("Could not link KLEE files %s", error.c_str());
    return false;
  }

  targetData = std::make_unique<llvm::DataLayout>(module.get());
  return true;
}

void KModule::replaceFunction(const std::unique_ptr<llvm::Module> &m,
                              const char *original, const char *replacement) {
  llvm::Function *originalFunc = m->getFunction(original);
  llvm::Function *replacementFunc = m->getFunction(replacement);
  if (!originalFunc)
    return;
  klee_message("Replacing function \"%s\" with \"%s\"", original, replacement);
  assert(replacementFunc && "Replacement function not found");
  assert(!(replacementFunc->isDeclaration()) && "replacement must have body");
  originalFunc->replaceAllUsesWith(replacementFunc);
  originalFunc->eraseFromParent();
}

void KModule::instrument(const Interpreter::ModuleOptions &opts) {
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
  if (opts.CheckDivZero)
    pm.add(new DivCheckPass());
  if (opts.CheckOvershift)
    pm.add(new OvershiftCheckPass());

  pm.add(new IntrinsicCleanerPass(*targetData, opts.WithFPRuntime));
  pm.run(*module);
  withPosixRuntime = opts.WithPOSIXRuntime;
}

void KModule::optimiseAndPrepare(
    const Interpreter::ModuleOptions &opts,
    llvm::ArrayRef<const char *> preservedFunctions) {
  // Preserve all functions containing klee-related function calls from being
  // optimised around
  if (!OptimiseKLEECall) {
    legacy::PassManager pm;
    pm.add(new OptNonePass());
    pm.run(*module);
  }

  if (opts.Optimize)
    Optimize(module.get(), preservedFunctions);

  // Add internal functions which are not used to check if instructions
  // have been already visited
  if (opts.CheckDivZero)
    addInternalFunction("klee_div_zero_check");
  if (opts.CheckOvershift)
    addInternalFunction("klee_overshift_check");

  // Use KLEE's internal float classification functions if requested.
  if (opts.WithFPRuntime) {
    if (UseKleeFloatInternals) {
      for (const auto &p : klee::floatReplacements) {
        replaceFunction(module, p.first.c_str(), p.second.c_str());
      }
    }
    for (const auto &p : klee::feRoundReplacements) {
      replaceFunction(module, p.first.c_str(), p.second.c_str());
    }
  }

  // Needs to happen after linking (since ctors/dtors can be modified)
  // and optimization (since global optimization can rewrite lists).
  injectStaticConstructorsAndDestructors(module.get(), opts.EntryPoint);

  // Finally, run the passes that maintain invariants we expect during
  // interpretation. We run the intrinsic cleaner just in case we
  // linked in something with intrinsics but any external calls are
  // going to be unresolved. We really need to handle the intrinsics
  // directly I think?
  legacy::PassManager pm3;
  if (opts.Simplify)
    pm3.add(createCFGSimplificationPass());
  switch (SwitchType) {
  case eSwitchTypeInternal:
    break;
  case eSwitchTypeSimple:
    pm3.add(new LowerSwitchPass());
    break;
  case eSwitchTypeLLVM:
    pm3.add(createLowerSwitchPass());
    break;
  default:
    klee_error("invalid --switch-type");
  }
  pm3.add(new IntrinsicCleanerPass(*targetData, opts.WithFPRuntime));
  pm3.add(createScalarizerPass());
  pm3.add(new PhiCleanerPass());
  pm3.add(new FunctionAliasPass());
  if (SplitCalls) {
    pm3.add(new CallSplitter());
  }
  if (SplitReturns) {
    pm3.add(new ReturnSplitter());
  }
  pm3.run(*module);
}

void KModule::manifest(InterpreterHandler *ih, bool forceSourceOutput) {

  if (OutputModule) {
    std::unique_ptr<llvm::raw_fd_ostream> f(ih->openOutputFile("final.bc"));
#if LLVM_VERSION_CODE >= LLVM_VERSION(7, 0)
    WriteBitcodeToFile(*module, *f);
#else
    WriteBitcodeToFile(module.get(), *f);
#endif
  }

  {
    /* Build shadow structures */
    std::unique_ptr<llvm::raw_fd_ostream> assemblyFS;
    if (OutputSource || forceSourceOutput) {
      assemblyFS = ih->openOutputFile("assembly.ll");
    }
    infos =
        std::make_unique<InstructionInfoTable>(*module, std::move(assemblyFS));
  }

  std::vector<Function *> declarations;

  unsigned functionID = 0;
  for (auto &Function : module->functions()) {
    if (Function.isDeclaration()) {
      declarations.push_back(&Function);
    }

    auto kf = std::unique_ptr<KFunction>(new KFunction(&Function, this));

    llvm::Function *function = &Function;
    for (auto &BasicBlock : *function) {
      unsigned numInstructions = kf->blockMap[&BasicBlock]->numInstructions;
      KBlock *kb = kf->blockMap[&BasicBlock];
      for (unsigned i = 0; i < numInstructions; ++i) {
        KInstruction *ki = kb->instructions[i];
        ki->info = &infos->getInfo(*ki->inst);
      }
    }

    functionIDMap.insert({&Function, functionID});
    kf->id = functionID;
    functionID++;
    functionNameMap.insert({kf->getName().str(), kf.get()});
    functionMap.insert(std::make_pair(&Function, kf.get()));
    functions.push_back(std::move(kf));
  }

  /* Compute various interesting properties */

  for (auto &kf : functions) {
    if (functionEscapes(kf->function))
      escapingFunctions.insert(kf->function);
  }

  for (auto &declaration : declarations) {
    if (functionEscapes(declaration))
      escapingFunctions.insert(declaration);
  }

  for (auto &kfp : functions) {
    for (auto &kcb : kfp.get()->kCallBlocks) {
      bool isInlineAsm = false;
#if LLVM_VERSION_CODE >= LLVM_VERSION(8, 0)
      const CallBase &cs = cast<CallBase>(*kcb->kcallInstruction->inst);
      if (isa<InlineAsm>(cs.getCalledOperand())) {
#else
      const CallSite cs(kcb->kcallInstruction->inst);
      if (isa<InlineAsm>(cs.getCalledValue())) {
#endif
        isInlineAsm = true;
      }
      if (kcb->calledFunctions.empty() && !isInlineAsm) {
        kcb->calledFunctions.insert(escapingFunctions.begin(),
                                    escapingFunctions.end());
      }
      for (auto &calledFunction : kcb->calledFunctions) {
        callMap[calledFunction].insert(kfp.get()->function);
      }
    }
  }

  if (DebugPrintEscapingFunctions && !escapingFunctions.empty()) {
    llvm::errs() << "KLEE: escaping functions: [";
    std::string delimiter = "";
    for (auto &Function : escapingFunctions) {
      llvm::errs() << delimiter << Function->getName();
      delimiter = ", ";
    }
    llvm::errs() << "]\n";
  }
}

void KModule::checkModule() {
  InstructionOperandTypeCheckPass *operandTypeCheckPass =
      new InstructionOperandTypeCheckPass();

  legacy::PassManager pm;
  if (!DontVerify)
    pm.add(createVerifierPass(false));
  pm.add(operandTypeCheckPass);
  pm.run(*module);

  // Enforce the operand type invariants that the Executor expects.  This
  // implicitly depends on the "Scalarizer" pass to be run in order to succeed
  // in the presence of vector instructions.
  if (!operandTypeCheckPass->checkPassed()) {
    klee_error("Unexpected instruction operand types detected");
  }
}

KBlock *KModule::getKBlock(llvm::BasicBlock *bb) {
  return functionMap[bb->getParent()]->blockMap[bb];
}

bool KModule::inMainModule(llvm::Function *f) {
  auto found = std::find_if(
      mainModuleFunctions.begin(), mainModuleFunctions.end(),
      [&f](const std::string &str) { return str == f->getName().str(); });
  return found != mainModuleFunctions.end();
}

Function *llvm::getTargetFunction(Value *calledVal) {
  SmallPtrSet<const GlobalValue *, 3> Visited;

  Constant *c = dyn_cast<Constant>(calledVal);
  if (!c)
    return 0;

  while (true) {
    if (GlobalValue *gv = dyn_cast<GlobalValue>(c)) {
      if (!Visited.insert(gv).second)
        return 0;

      if (Function *f = dyn_cast<Function>(gv))
        return f;
      else if (GlobalAlias *ga = dyn_cast<GlobalAlias>(gv))
        c = ga->getAliasee();
      else
        return 0;
    } else if (llvm::ConstantExpr *ce = dyn_cast<llvm::ConstantExpr>(c)) {
      if (ce->getOpcode() == Instruction::BitCast)
        c = ce->getOperand(0);
      else
        return 0;
    } else
      return 0;
  }
}

KConstant *KModule::getKConstant(const Constant *c) {
  auto it = constantMap.find(c);
  if (it != constantMap.end())
    return it->second.get();
  return NULL;
}

unsigned KModule::getConstantID(Constant *c, KInstruction *ki) {
  if (KConstant *kc = getKConstant(c))
    return kc->id;

  unsigned id = constants.size();
  auto kc = std::unique_ptr<KConstant>(new KConstant(c, id, ki));
  constantMap.insert(std::make_pair(c, std::move(kc)));
  constants.push_back(c);
  return id;
}

/***/

KConstant::KConstant(llvm::Constant *_ct, unsigned _id, KInstruction *_ki) {
  ct = _ct;
  id = _id;
  ki = _ki;
}

/***/

static int getOperandNum(
    Value *v,
    std::unordered_map<Instruction *, unsigned> &instructionToRegisterMap,
    KModule *km, KInstruction *ki) {
  if (Instruction *inst = dyn_cast<Instruction>(v)) {
    return instructionToRegisterMap[inst];
  } else if (Argument *a = dyn_cast<Argument>(v)) {
    return a->getArgNo();
  } else if (isa<BasicBlock>(v) || isa<InlineAsm>(v) ||
             isa<MetadataAsValue>(v)) {
    return -1;
  } else {
    assert(isa<Constant>(v));
    Constant *c = cast<Constant>(v);
    return -(km->getConstantID(c, ki) + 2);
  }
}

void KBlock::handleKInstruction(
    std::unordered_map<Instruction *, unsigned> &instructionToRegisterMap,
    llvm::Instruction *inst, KModule *km, KInstruction *ki) {
  ki->parent = this;
  ki->inst = inst;
  ki->dest = instructionToRegisterMap[inst];
  if (isa<CallInst>(inst) || isa<InvokeInst>(inst)) {
#if LLVM_VERSION_CODE >= LLVM_VERSION(8, 0)
    const CallBase &cs = cast<CallBase>(*inst);
    Value *val = cs.getCalledOperand();
#else
    const CallSite cs(inst);
    Value *val = cs.getCalledValue();
#endif
    unsigned numArgs = cs.arg_size();
    ki->operands = new int[numArgs + 1];
    ki->operands[0] = getOperandNum(val, instructionToRegisterMap, km, ki);
    for (unsigned j = 0; j < numArgs; j++) {
      Value *v = cs.getArgOperand(j);
      ki->operands[j + 1] = getOperandNum(v, instructionToRegisterMap, km, ki);
    }
  } else {
    unsigned numOperands = inst->getNumOperands();
    ki->operands = new int[numOperands];
    for (unsigned j = 0; j < numOperands; j++) {
      Value *v = inst->getOperand(j);
      ki->operands[j] = getOperandNum(v, instructionToRegisterMap, km, ki);
    }
  }
}

KFunction::KFunction(llvm::Function *_function, KModule *_km)
    : KCallable(CK_Function), parent(_km), function(_function),
      numArgs(function->arg_size()), numInstructions(0), numBlocks(0),
      entryKBlock(nullptr), trackCoverage(true) {
  for (auto &BasicBlock : *function) {
    numInstructions += BasicBlock.size();
    numBlocks++;
  }
  instructions = new KInstruction *[numInstructions];
  std::unordered_map<Instruction *, unsigned> instructionToRegisterMap;
  // Assign unique instruction IDs to each basic block
  unsigned n = 0;
  // The first arg_size() registers are reserved for formals.
  unsigned rnum = numArgs;
  for (llvm::Function::iterator bbit = function->begin(),
                                bbie = function->end();
       bbit != bbie; ++bbit) {
    for (llvm::BasicBlock::iterator it = bbit->begin(), ie = bbit->end();
         it != ie; ++it)
      instructionToRegisterMap[&*it] = rnum++;
  }
  numRegisters = rnum;

  unsigned blockID = 0;
  for (llvm::Function::iterator bbit = function->begin(),
                                bbie = function->end();
       bbit != bbie; ++bbit) {
    KBlock *kb;
    Instruction *fit = &bbit->front();
    Instruction *lit = &bbit->back();
    if (SplitCalls && (isa<CallInst>(fit) || isa<InvokeInst>(fit))) {
#if LLVM_VERSION_CODE >= LLVM_VERSION(8, 0)
      const CallBase &cs = cast<CallBase>(*fit);
      Value *fp = cs.getCalledOperand();
#else
      CallSite cs(fit);
      Value *fp = cs.getCalledValue();
#endif
      Function *f = getTargetFunction(fp);
      std::set<llvm::Function *> calledFunctions;
      if (f) {
        calledFunctions.insert(f);
      }
      KCallBlock *ckb = new KCallBlock(
          this, &*bbit, parent, instructionToRegisterMap,
          registerToInstructionMap, calledFunctions, &instructions[n]);
      kCallBlocks.push_back(ckb);
      kb = ckb;
    } else if (SplitReturns && isa<ReturnInst>(lit)) {
      kb = new KReturnBlock(this, &*bbit, parent, instructionToRegisterMap,
                            registerToInstructionMap, &instructions[n]);
      returnKBlocks.push_back(kb);
    } else
      kb = new KBlock(this, &*bbit, parent, instructionToRegisterMap,
                      registerToInstructionMap, &instructions[n]);
    for (unsigned i = 0; i < kb->numInstructions; i++, n++) {
      instructionMap[instructions[n]->inst] = instructions[n];
    }
    kb->id = blockID++;
    blockMap[&*bbit] = kb;
    blocks.push_back(std::unique_ptr<KBlock>(kb));
  }

  if (numBlocks > 0) {
    assert(function->begin() != function->end());
    entryKBlock = blockMap[&*function->begin()];
  }
}

KFunction::~KFunction() {
  for (unsigned i = 0; i < numInstructions; ++i)
    delete instructions[i];
  delete[] instructions;
}

KBlock::KBlock(
    KFunction *_kfunction, llvm::BasicBlock *block, KModule *km,
    std::unordered_map<Instruction *, unsigned> &instructionToRegisterMap,
    std::unordered_map<unsigned, KInstruction *> &registerToInstructionMap,
    KInstruction **instructionsKF)
    : parent(_kfunction), basicBlock(block), numInstructions(0),
      trackCoverage(true) {
  numInstructions += block->size();
  instructions = instructionsKF;

  unsigned i = 0;
  for (llvm::BasicBlock::iterator it = block->begin(), ie = block->end();
       it != ie; ++it) {
    KInstruction *ki;

    switch (it->getOpcode()) {
    case Instruction::GetElementPtr:
    case Instruction::InsertValue:
    case Instruction::ExtractValue:
      ki = new KGEPInstruction();
      break;
    default:
      ki = new KInstruction();
      break;
    }

    Instruction *inst = &*it;
    handleKInstruction(instructionToRegisterMap, inst, km, ki);
    ki->index = i;
    instructions[i++] = ki;
    registerToInstructionMap[instructionToRegisterMap[&*it]] = ki;
  }
}

KCallBlock::KCallBlock(
    KFunction *_kfunction, llvm::BasicBlock *block, KModule *km,
    std::unordered_map<Instruction *, unsigned> &instructionToRegisterMap,
    std::unordered_map<unsigned, KInstruction *> &registerToInstructionMap,
    std::set<llvm::Function *> _calledFunctions, KInstruction **instructionsKF)
    : KBlock::KBlock(_kfunction, block, km, instructionToRegisterMap,
                     registerToInstructionMap, instructionsKF),
      kcallInstruction(this->instructions[0]),
      calledFunctions(_calledFunctions) {}

bool KCallBlock::intrinsic() const {
  if (calledFunctions.size() != 1) {
    return false;
  }
  Function *calledFunction = *calledFunctions.begin();
  auto kf = parent->parent->functionMap[calledFunction];
  if (kf && kf->kleeHandled) {
    return true;
  }
  return calledFunction->getIntrinsicID() != llvm::Intrinsic::not_intrinsic;
}

bool KCallBlock::internal() const {
  return calledFunctions.size() == 1 &&
         parent->parent->functionMap[*calledFunctions.begin()] != nullptr;
}

KFunction *KCallBlock::getKFunction() const {
  return calledFunctions.size() == 1
             ? parent->parent->functionMap[*calledFunctions.begin()]
             : nullptr;
}

KReturnBlock::KReturnBlock(
    KFunction *_kfunction, llvm::BasicBlock *block, KModule *km,
    std::unordered_map<Instruction *, unsigned> &instructionToRegisterMap,
    std::unordered_map<unsigned, KInstruction *> &registerToInstructionMap,
    KInstruction **instructionsKF)
    : KBlock::KBlock(_kfunction, block, km, instructionToRegisterMap,
                     registerToInstructionMap, instructionsKF) {}

std::string KBlock::getLabel() const {
  std::string _label;
  llvm::raw_string_ostream label_stream(_label);
  basicBlock->printAsOperand(label_stream, false);
  label_stream << " (lines " << getFirstInstruction()->info->line << " to "
               << getLastInstruction()->info->line << ")";
  std::string label = label_stream.str();
  return label;
}

std::string KBlock::toString() const {
  return getLabel() + " in function " + parent->function->getName().str();
}
