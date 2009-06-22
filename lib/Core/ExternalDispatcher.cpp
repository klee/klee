//===-- ExternalDispatcher.cpp --------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "ExternalDispatcher.h"

#include "llvm/Module.h"
#include "llvm/Constants.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Instructions.h"
#include "llvm/ModuleProvider.h"
#include "llvm/ExecutionEngine/JIT.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/Support/CallSite.h"
#include "llvm/System/DynamicLibrary.h"
#include "llvm/Support/Streams.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetSelect.h"
#include <setjmp.h>
#include <signal.h>

using namespace llvm;
using namespace klee;

/***/

static jmp_buf escapeCallJmpBuf;

extern "C" {

static void sigsegv_handler(int signal, siginfo_t *info, void *context) {
  longjmp(escapeCallJmpBuf, 1);
}

}

void *ExternalDispatcher::resolveSymbol(const std::string &name) {
  assert(executionEngine);
  
  const char *str = name.c_str();

  // We use this to validate that function names can be resolved so we
  // need to match how the JIT does it. Unfortunately we can't
  // directly access the JIT resolution function
  // JIT::getPointerToNamedFunction so we emulate the important points.

  if (str[0] == 1) // asm specifier, skipped
    ++str;

  void *addr = dl_symbols.SearchForAddressOfSymbol(str);
  if (addr)
    return addr;
  
  // If it has an asm specifier and starts with an underscore we retry
  // without the underscore. I (DWD) don't know why.
  if (name[0] == 1 && str[0]=='_') { 
    ++str;
    addr = dl_symbols.SearchForAddressOfSymbol(str);
  }

  return addr;
}

ExternalDispatcher::ExternalDispatcher() {
  dispatchModule = new Module("ExternalDispatcher");
  ExistingModuleProvider* MP = new ExistingModuleProvider(dispatchModule);
  
  std::string error;
  executionEngine = ExecutionEngine::createJIT(MP, &error);
  if (!executionEngine) {
    llvm::cerr << "unable to make jit: " << error << "\n";
    abort();
  }

  // If we have a native target, initialize it to ensure it is linked in and
  // usable by the JIT.
  llvm::InitializeNativeTarget();

  // from ExecutionEngine::create
  if (executionEngine) {
    // Make sure we can resolve symbols in the program as well. The zero arg
    // to the function tells DynamicLibrary to load the program, not a library.
    dl_symbols.LoadLibraryPermanently(0);
  }

#ifdef WINDOWS
  preboundFunctions["getpid"] = (void*) (long) getpid;
  preboundFunctions["putchar"] = (void*) (long) putchar;
  preboundFunctions["printf"] = (void*) (long) printf;
  preboundFunctions["fprintf"] = (void*) (long) fprintf;
  preboundFunctions["sprintf"] = (void*) (long) sprintf;
#endif
}

ExternalDispatcher::~ExternalDispatcher() {
  delete executionEngine;
}

bool ExternalDispatcher::executeCall(Function *f, Instruction *i, uint64_t *args) {
  dispatchers_ty::iterator it = dispatchers.find(i);
  Function *dispatcher;

  if (it == dispatchers.end()) {
#ifdef WINDOWS
    std::map<std::string, void*>::iterator it2 = 
      preboundFunctions.find(f->getName()));

    if (it2 != preboundFunctions.end()) {
      // only bind once
      if (it2->second) {
        executionEngine->addGlobalMapping(f, it2->second);
        it2->second = 0;
      }
    }
#endif

    dispatcher = createDispatcher(f,i);

    dispatchers.insert(std::make_pair(i, dispatcher));

    if (dispatcher) {
      // force the JIT execution engine to go ahead and build the
      // function. this ensures that any errors or assertions in the
      // compilation process will trigger crashes instead of being
      // caught as aborts in the external function.
      executionEngine->recompileAndRelinkFunction(dispatcher);
    }
  } else {
    dispatcher = it->second;
  }

  return runProtectedCall(dispatcher, args);
}

// XXX not reentrant
static uint64_t *gTheArgsP;

bool ExternalDispatcher::runProtectedCall(Function *f, uint64_t *args) {
  struct sigaction segvAction, segvActionOld;
  bool res;
  
  if (!f)
    return false;

  std::vector<GenericValue> gvArgs;
  gTheArgsP = args;

  segvAction.sa_handler = 0;
  memset(&segvAction.sa_mask, 0, sizeof(segvAction.sa_mask));
  segvAction.sa_flags = SA_SIGINFO;
  segvAction.sa_sigaction = ::sigsegv_handler;
  sigaction(SIGSEGV, &segvAction, &segvActionOld);

  if (setjmp(escapeCallJmpBuf)) {
    res = false;
  } else {
    executionEngine->runFunction(f, gvArgs);
    res = true;
  }

  sigaction(SIGSEGV, &segvActionOld, 0);
  return res;
}

// for performance purposes we construct the stub in such a way that
// the arguments pointer is passed through the static global variable
// gTheArgsP in this file. This is done so that the stub function
// prototype trivially matches the special cases that the JIT knows
// how to directly call. If this is not done, then the jit will end up
// generating a nullary stub just to call our stub, for every single
// function call.
Function *ExternalDispatcher::createDispatcher(Function *target, Instruction *inst) {
  if (!resolveSymbol(target->getName()))
    return 0;

  CallSite cs;
  if (inst->getOpcode()==Instruction::Call) {
    cs = CallSite(cast<CallInst>(inst));
  } else {
    cs = CallSite(cast<InvokeInst>(inst));
  }

  Value **args = new Value*[cs.arg_size()];

  std::vector<const Type*> nullary;
  
  Function *dispatcher = Function::Create(FunctionType::get(Type::VoidTy, 
							    nullary, false),
					  GlobalVariable::ExternalLinkage, 
					  "",
					  dispatchModule);


  BasicBlock *dBB = BasicBlock::Create("entry", dispatcher);

  Instruction *argI64sp = new IntToPtrInst(ConstantInt::get(Type::Int64Ty, (long) (void*) &gTheArgsP),
					   PointerType::getUnqual(PointerType::getUnqual(Type::Int64Ty)),
					   "argsp",
					   dBB);
  Instruction *argI64s = new LoadInst(argI64sp, "args", dBB); 

  unsigned i = 0;
  for (CallSite::arg_iterator ai = cs.arg_begin(), ae = cs.arg_end();
       ai!=ae; ++ai, ++i) {
    Value *index = ConstantInt::get(Type::Int32Ty, i+1);

    Instruction *argI64p = GetElementPtrInst::Create(argI64s, index, "", dBB);
    Instruction *argp = new BitCastInst(argI64p, 
                                        PointerType::getUnqual((*ai)->getType()), "", dBB);
    args[i] = new LoadInst(argp, "", dBB);
  }

  Instruction *result = CallInst::Create(target, args, args+i, "", dBB);

  if (result->getType() != Type::VoidTy) {
    Instruction *resp = new BitCastInst(argI64s, 
                                        PointerType::getUnqual(result->getType()), "", dBB);
    new StoreInst(result, resp, dBB);
  }

  ReturnInst::Create(dBB);

  delete[] args;

  return dispatcher;
}
