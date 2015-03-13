//===-- ExternalDispatcher.cpp --------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "ExternalDispatcher.h"
#include "klee/Config/Version.h"

#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 3)
#include "llvm/IR/Module.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#else
#include "llvm/Module.h"
#include "llvm/Constants.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Instructions.h"
#include "llvm/LLVMContext.h"
#endif
#if LLVM_VERSION_CODE < LLVM_VERSION(3, 6)
#include "llvm/ExecutionEngine/JIT.h"
#elif LLVM_VERSION_CODE >= LLVM_VERSION(3, 6)
#include "llvm/ExecutionEngine/MCJIT.h"
#endif

#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/Support/DynamicLibrary.h"
#include "llvm/Support/raw_ostream.h"

#if LLVM_VERSION_CODE < LLVM_VERSION(3, 0)
#include "llvm/Target/TargetSelect.h"
#else
#include "llvm/Support/TargetSelect.h"
#endif

#if LLVM_VERSION_CODE < LLVM_VERSION(3, 5)
#include "llvm/Support/CallSite.h"
#else
#include "llvm/IR/CallSite.h"
#endif

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
/*void showFunctionStuffBeforeCall(Function *f)
{
	std::string TmpStr;
	llvm::raw_string_ostream os(TmpStr);
	os << "Before Call to: " << function->getName().str() << "(";
	for (unsigned i=0; i<arguments.size(); i++)
	{
		os << arguments[i];
		if (i != arguments.size()-1)
		os << ", ";
	}
	os << ")";
	klee_warning("%s", os.str().c_str());
}*/

void *ExternalDispatcher::resolveSymbol(const std::string &name) {
  assert(executionEngine);
  
  const char *str = name.c_str();

  // We use this to validate that function names can be resolved so we
  // need to match how the JIT does it. Unfortunately we can't
  // directly access the JIT resolution function
  // JIT::getPointerToNamedFunction so we emulate the important points.

  if (str[0] == 1) // asm specifier, skipped
    ++str;

  void *addr = sys::DynamicLibrary::SearchForAddressOfSymbol(str);
  if (addr)
    return addr;
  
  // If it has an asm specifier and starts with an underscore we retry
  // without the underscore. I (DWD) don't know why.
  if (name[0] == 1 && str[0]=='_') { 
    ++str;
    addr = sys::DynamicLibrary::SearchForAddressOfSymbol(str);
  }

  return addr;
}

ExternalDispatcher::ExternalDispatcher() {

  
  dispatchModule = new Module("ExternalDispatcher", getGlobalContext()) ;

  std::string error="";
  
  //executionEngine = ExecutionEngine::createJIT(dispatchModule, &error);
  dispatchModule_uniptr.reset(dispatchModule);
  executionEngine = EngineBuilder( std::move(dispatchModule_uniptr) ).create();
  
  if (!executionEngine) {
    llvm::errs() << "unable to make jit: " << error << "\n";
    abort();
  }
  // If we have a native target, initialize it to ensure it is linked in and
  // usable by the JIT.
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmParser();
  llvm::InitializeNativeTargetAsmPrinter();
  //

  // from ExecutionEngine::create
  if (executionEngine) {
    // Make sure we can resolve symbols in the program as well. The zero arg
    // to the function tells DynamicLibrary to load the program, not a library.
    sys::DynamicLibrary::LoadLibraryPermanently(0);
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

      // Force the JIT execution engine to go ahead and build the function. This
      // ensures that any errors or assertions in the compilation process will
      // trigger crashes instead of being caught as aborts in the external
      // function.
      //executionEngine->recompileAndRelinkFunction(dispatcher);
      
      executionEngine->finalizeObject();
      //executionEngine->getPointerToFunction(dispatcher); //maybe? though I doubt it is a suitable replacement for above line
    }
  } else {
    dispatcher = it->second;
  }
  
  return runProtectedCall(dispatcher, args);
}

// FIXME: This is not reentrant.
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
	  
	//printf("runFunction fptr:0x%p fname:%s\n", executionEngine->getPointerToFunction(f) ,f->getName().str().c_str() );
		std::string str;
	llvm::raw_string_ostream rso(str);
	f->print(rso);
	//printf("runFunction: \nf: %s , targetName: %s\n", str.c_str(), f->getName().str().c_str());
    executionEngine->runFunction(f, gvArgs);
    //executionEngine->callExternalFunction (f,gvArgs);
    res = true;
  }

  sigaction(SIGSEGV, &segvActionOld, 0);
  return res;
}

// For performance purposes we construct the stub in such a way that the
// arguments pointer is passed through the static global variable gTheArgsP in
// this file. This is done so that the stub function prototype trivially matches
// the special cases that the JIT knows how to directly call. If this is not
// done, then the jit will end up generating a nullary stub just to call our
// stub, for every single function call.
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

  std::vector<LLVM_TYPE_Q Type*> nullary;
  Twine dispatcherName(target->getName());
  dispatcherName.concat("_dispatcher");
  Function *dispatcher = Function::Create(FunctionType::get(Type::getVoidTy(getGlobalContext()), 
							    nullary, false),
					  GlobalVariable::ExternalLinkage, 
					  "hello"+target->getName().str(),
					  dispatchModule);

	//printf("dispatcher:0x%p\n", executionEngine->getPointerToFunction(dispatcher) );
	//printf("dispatcher:0x%p\n", dispatcher );
  BasicBlock *dBB = BasicBlock::Create(getGlobalContext(), "entry", dispatcher);

  // Get a Value* for &gTheArgsP, as an i64**.
  Instruction *argI64sp = 
    new IntToPtrInst(ConstantInt::get(Type::getInt64Ty(getGlobalContext()), 
                                      (uintptr_t) (void*) &gTheArgsP),
                     PointerType::getUnqual(PointerType::getUnqual(Type::getInt64Ty(getGlobalContext()))),
                     "argsp", dBB);
  Instruction *argI64s = new LoadInst(argI64sp, "args", dBB); 
  
  // Get the target function type.
  LLVM_TYPE_Q FunctionType *FTy =
    cast<FunctionType>(cast<PointerType>(target->getType())->getElementType());

  // Each argument will be passed by writing it into gTheArgsP[i].
  unsigned i = 0, idx = 2;
  for (CallSite::arg_iterator ai = cs.arg_begin(), ae = cs.arg_end();
       ai!=ae; ++ai, ++i) {
    // Determine the type the argument will be passed as. This accomodates for
    // the corresponding code in Executor.cpp for handling calls to bitcasted
    // functions.
    LLVM_TYPE_Q Type *argTy = (i < FTy->getNumParams() ? FTy->getParamType(i) : 
                               (*ai)->getType());
    Instruction *argI64p = 
      GetElementPtrInst::Create(argI64s, 
                                ConstantInt::get(Type::getInt32Ty(getGlobalContext()), 
                                                 idx), 
                                "", dBB);

    Instruction *argp = new BitCastInst(argI64p, PointerType::getUnqual(argTy),
                                        "", dBB);
    args[i] = new LoadInst(argp, "", dBB);

    unsigned argSize = argTy->getPrimitiveSizeInBits();
    idx += ((!!argSize ? argSize : 64) + 63)/64;
  }
  
  Constant *dispatchTarget =
    dispatchModule->getOrInsertFunction(target->getName(), FTy,
                                        target->getAttributes());
	std::string str;
	llvm::raw_string_ostream rso(str);
	dispatchTarget->print(rso);
	//printf("dispatchTarget: %s , targetName: %s\n", str.c_str(), target->getName().str().c_str());
	
#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 0)
  Instruction *result = CallInst::Create(dispatchTarget,
                                         llvm::ArrayRef<Value *>(args, args+i),
                                         "", dBB
                                         );
#else
  Instruction *result = CallInst::Create(dispatchTarget, args, args+i, "", dBB);
#endif
  if (result->getType() != Type::getVoidTy(getGlobalContext())) {
    Instruction *resp = 
      new BitCastInst(argI64s, PointerType::getUnqual(result->getType()), 
                      "", dBB);
    new StoreInst(result, resp, dBB);
  }

  ReturnInst::Create(getGlobalContext(), dBB);

  delete[] args;
//printf("ENDcreateDispatcher fptr:0x%p fname:%s\n", executionEngine->getPointerToFunction(target) ,target->getName().str().c_str() );
  return dispatcher;
}
