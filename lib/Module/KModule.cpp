//===-- KModule.cpp -------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "KModule"

#include "klee/Module/KModule.h"

#include "ModuleHelper.h"
#include "Passes.h"

#include "klee/Config/Version.h"
#include "klee/Core/Interpreter.h"
#include "klee/Module/Cell.h"
#include "klee/Module/InstructionInfoTable.h"
#include "klee/Module/KInstruction.h"
#include "klee/Support/Debug.h"
#include "klee/Support/ErrorHandling.h"
#include "klee/Support/ModuleUtil.h"
#include "klee/Support/OptionCategories.h"

#include "klee/Support/CompilerWarning.h"

DISABLE_WARNING_PUSH
DISABLE_WARNING_DEPRECATED_DECLARATIONS
#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvm/IR/IRBuilder.h"
DISABLE_WARNING_POP

#include <sstream>

using namespace llvm;
using namespace klee;

namespace klee {
cl::OptionCategory
    ModuleCat("Module-related options",
              "These options affect the compile-time processing of the code.");
}

namespace {
  cl::opt<bool>
  OutputSource("output-source",
               cl::desc("Write the assembly for the final transformed source (default=true)"),
               cl::init(true),
	       cl::cat(ModuleCat));

  cl::opt<bool>
  OutputModule("output-module",
               cl::desc("Write the bitcode for the final transformed module (default=false)"),
               cl::init(false),
	       cl::cat(ModuleCat));

  
  cl::opt<bool>
  DebugPrintEscapingFunctions("debug-print-escaping-functions", 
                              cl::desc("Print functions whose address is taken (default=false)"),
			      cl::cat(ModuleCat));

  // Don't run VerifierPass when checking module
  cl::opt<bool>
  DontVerify("disable-verify",
             cl::desc("Do not verify the module integrity (default=false)"),
             cl::init(false), cl::cat(klee::ModuleCat));

  cl::opt<bool>
  OptimiseKLEECall("klee-call-optimisation",
                             cl::desc("Allow optimization of functions that "
                                      "contain KLEE calls (default=true)"),
                             cl::init(true), cl::cat(ModuleCat));
cl::opt<SwitchImplType> SwitchType(
    "switch-type",
    cl::desc("Select the implementation of switch (default=internal)"),
    cl::values(clEnumValN(SwitchImplType::eSwitchTypeSimple, "simple",
                          "lower to ordered branches"),
               clEnumValN(SwitchImplType::eSwitchTypeLLVM, "llvm",
                          "lower using LLVM"),
               clEnumValN(SwitchImplType::eSwitchTypeInternal, "internal",
                          "execute switch internally")),
    cl::init(SwitchImplType::eSwitchTypeInternal), cl::cat(ModuleCat));

} // namespace

/***/

// what a hack
static Function *getStubFunctionForCtorList(Module *m,
                                            GlobalVariable *gv, 
                                            std::string name) {
  assert(!gv->isDeclaration() && !gv->hasInternalLinkage() &&
         "do not support old LLVM style constructor/destructor lists");

  std::vector<Type *> nullary;

  Function *fn = Function::Create(FunctionType::get(Type::getVoidTy(m->getContext()),
						    nullary, false),
				  GlobalVariable::InternalLinkage, 
				  name,
                              m);
  BasicBlock *bb = BasicBlock::Create(m->getContext(), "entry", fn);
  llvm::IRBuilder<> Builder(bb);

  // From lli:
  // Should be an array of '{ int, void ()* }' structs.  The first value is
  // the init priority, which we ignore.
  auto arr = dyn_cast<ConstantArray>(gv->getInitializer());
  if (arr) {
    for (unsigned i=0; i<arr->getNumOperands(); i++) {
      auto cs = cast<ConstantStruct>(arr->getOperand(i));
      // There is a third element in global_ctor elements (``i8 @data``).
      assert(cs->getNumOperands() == 3 &&
             "unexpected element in ctor initializer list");
      auto fp = cs->getOperand(1);
      if (!fp->isNullValue()) {
        if (auto ce = dyn_cast<llvm::ConstantExpr>(fp))
          fp = ce->getOperand(0);

        if (auto f = dyn_cast<Function>(fp)) {
          Builder.CreateCall(f);
        } else {
          assert(0 && "unable to get function pointer from ctor initializer list");
        }
      }
    }
  }

  Builder.CreateRetVoid();

  return fn;
}

void klee::injectStaticConstructorsAndDestructors(
    Module *m, llvm::StringRef entryFunction) {
  GlobalVariable *ctors = m->getNamedGlobal("llvm.global_ctors");
  GlobalVariable *dtors = m->getNamedGlobal("llvm.global_dtors");

  if (!ctors && !dtors)
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

void KModule::addInternalFunction(const char* functionName){
  Function* internalFunction = module->getFunction(functionName);
  if (!internalFunction) {
    KLEE_DEBUG(klee_warning(
        "Failed to add internal function %s. Not found.", functionName));
    return ;
  }
  KLEE_DEBUG(klee_message("Added function %s.",functionName));
  internalFunctions.insert(internalFunction);
}

bool KModule::link(std::vector<std::unique_ptr<llvm::Module>> &modules,
                   const std::string &entryPoint) {
  auto numRemainingModules = modules.size();
  // Add the currently active module to the list of linkables
  modules.push_back(std::move(module));
  std::string error;
  module = std::unique_ptr<llvm::Module>(
      klee::linkModules(modules, entryPoint, error));
  if (!module)
    klee_error("Could not link KLEE files %s", error.c_str());

  targetData = std::unique_ptr<llvm::DataLayout>(new DataLayout(module.get()));

  // Check if we linked anything
  return modules.size() != numRemainingModules;
}

void KModule::instrument(const Interpreter::ModuleOptions &opts) {
  klee::instrument(opts.CheckDivZero, opts.CheckOvershift, module.get());
}

void KModule::optimiseAndPrepare(
    const Interpreter::ModuleOptions &opts,
    llvm::ArrayRef<const char *> preservedFunctions) {
  // Add internal functions which are not used to check if instructions
  // have been already visited
  if (opts.CheckDivZero)
    addInternalFunction("klee_div_zero_check");
  if (opts.CheckOvershift)
    addInternalFunction("klee_overshift_check");

  klee::optimiseAndPrepare(OptimiseKLEECall, opts.Optimize, SwitchType,
                           opts.EntryPoint, preservedFunctions, module.get());
}

void KModule::manifest(InterpreterHandler *ih, bool forceSourceOutput) {
  if (OutputSource || forceSourceOutput) {
    std::unique_ptr<llvm::raw_fd_ostream> os(ih->openOutputFile("assembly.ll"));
    assert(os && !os->has_error() && "unable to open source output");
    *os << *module;
  }

  if (OutputModule) {
    std::unique_ptr<llvm::raw_fd_ostream> f(ih->openOutputFile("final.bc"));
    llvm::WriteBitcodeToFile(*module, *f);
  }

  /* Build shadow structures */

  infos = std::unique_ptr<InstructionInfoTable>(
      new InstructionInfoTable(*module.get()));

  std::vector<Function *> declarations;

  for (auto &Function : *module) {
    if (Function.isDeclaration()) {
      declarations.push_back(&Function);
    }

    auto kf = std::unique_ptr<KFunction>(new KFunction(&Function, this));

    for (unsigned i=0; i<kf->numInstructions; ++i) {
      KInstruction *ki = kf->instructions[i];
      ki->info = &infos->getInfo(*ki->inst);
    }

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

void KModule::checkModule() { klee::checkModule(DontVerify, module.get()); }

KConstant* KModule::getKConstant(const Constant *c) {
  auto it = constantMap.find(c);
  if (it != constantMap.end())
    return it->second.get();
  return NULL;
}

unsigned KModule::getConstantID(Constant *c, KInstruction* ki) {
  if (KConstant *kc = getKConstant(c))
    return kc->id;  

  unsigned id = constants.size();
  auto kc = std::unique_ptr<KConstant>(new KConstant(c, id, ki));
  constantMap.insert(std::make_pair(c, std::move(kc)));
  constants.push_back(c);
  return id;
}

/***/

KConstant::KConstant(llvm::Constant* _ct, unsigned _id, KInstruction* _ki) {
  ct = _ct;
  id = _id;
  ki = _ki;
}

/***/

static int getOperandNum(Value *v,
                         std::map<Instruction*, unsigned> &registerMap,
                         KModule *km,
                         KInstruction *ki) {
  if (Instruction *inst = dyn_cast<Instruction>(v)) {
    return registerMap[inst];
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

KFunction::KFunction(llvm::Function *_function,
                     KModule *km) 
  : KCallable(CK_Function),
    function(_function),
    numArgs(function->arg_size()),
    numInstructions(0),
    trackCoverage(true) {
  // Assign unique instruction IDs to each basic block
  for (auto &BasicBlock : *function) {
    basicBlockEntry[&BasicBlock] = numInstructions;
    numInstructions += BasicBlock.size();
  }

  instructions = new KInstruction*[numInstructions];

  std::map<Instruction*, unsigned> registerMap;

  // The first arg_size() registers are reserved for formals.
  unsigned rnum = numArgs;
  for (llvm::Function::iterator bbit = function->begin(), 
         bbie = function->end(); bbit != bbie; ++bbit) {
    for (llvm::BasicBlock::iterator it = bbit->begin(), ie = bbit->end();
         it != ie; ++it)
      registerMap[&*it] = rnum++;
  }
  numRegisters = rnum;
  
  unsigned i = 0;
  for (llvm::Function::iterator bbit = function->begin(), 
         bbie = function->end(); bbit != bbie; ++bbit) {
    for (llvm::BasicBlock::iterator it = bbit->begin(), ie = bbit->end();
         it != ie; ++it) {
      KInstruction *ki;

      switch(it->getOpcode()) {
      case Instruction::GetElementPtr:
      case Instruction::InsertValue:
      case Instruction::ExtractValue:
        ki = new KGEPInstruction(); break;
      default:
        ki = new KInstruction(); break;
      }

      Instruction *inst = &*it;
      ki->inst = inst;
      ki->dest = registerMap[inst];

      if (isa<CallInst>(it) || isa<InvokeInst>(it)) {
        const CallBase &cb = cast<CallBase>(*inst);
        Value *val = cb.getCalledOperand();
        unsigned numArgs = cb.arg_size();
        ki->operands = new int[numArgs+1];
        ki->operands[0] = getOperandNum(val, registerMap, km, ki);
        for (unsigned j=0; j<numArgs; j++) {
          Value *v = cb.getArgOperand(j);
          ki->operands[j+1] = getOperandNum(v, registerMap, km, ki);
        }
      } else {
        unsigned numOperands = it->getNumOperands();
        ki->operands = new int[numOperands];
        for (unsigned j=0; j<numOperands; j++) {
          Value *v = it->getOperand(j);
          ki->operands[j] = getOperandNum(v, registerMap, km, ki);
        }
      }

      instructions[i++] = ki;
    }
  }
}

KFunction::~KFunction() {
  for (unsigned i=0; i<numInstructions; ++i)
    delete instructions[i];
  delete[] instructions;
}
