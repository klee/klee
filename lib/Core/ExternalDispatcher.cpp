//===-- ExternalDispatcher.cpp --------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "ExternalDispatcher.h"

#include "CoreStats.h"
#include "klee/Config/Version.h"
#include "klee/Module/KCallable.h"
#include "klee/Module/KModule.h"

#include "klee/Support/CompilerWarning.h"
DISABLE_WARNING_PUSH
DISABLE_WARNING_DEPRECATED_DECLARATIONS
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/ExecutionEngine/MCJIT.h"
#include "llvm/Support/DynamicLibrary.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/TargetSelect.h"
DISABLE_WARNING_POP

#include <csetjmp>
#include <csignal>

using namespace llvm;
using namespace klee;

/***/

static sigjmp_buf escapeCallJmpBuf;

extern "C" {

static void sigsegv_handler(int signal, siginfo_t *info, void *context) {
  siglongjmp(escapeCallJmpBuf, 1);
}
}

namespace klee {

class ExternalDispatcherImpl {
private:
  typedef std::map<const llvm::Instruction *, llvm::Function *> dispatchers_ty;
  dispatchers_ty dispatchers; //dispatchers是一个从Instruction指向Function的map
  llvm::Function *createDispatcher(KCallable *target, llvm::Instruction *i,
                                   llvm::Module *module);
  llvm::ExecutionEngine *executionEngine;
  LLVMContext &ctx;
  std::map<std::string, void *> preboundFunctions;
  bool runProtectedCall(llvm::Function *f, uint64_t *args);
  llvm::Module *singleDispatchModule;
  std::vector<std::string> moduleIDs;
  std::string &getFreshModuleID();
  int lastErrno;

public:
  ExternalDispatcherImpl(llvm::LLVMContext &ctx);
  ~ExternalDispatcherImpl();
  bool executeCall(KCallable *callable, llvm::Instruction *i,
                   uint64_t *args);
  void *resolveSymbol(const std::string &name);
  int getLastErrno();
  void setLastErrno(int newErrno);
};

std::string &ExternalDispatcherImpl::getFreshModuleID() {
  // We store the module IDs because `llvm::Module` constructor takes the
  // module ID as a StringRef so it doesn't own the ID.  Therefore we need to
  // own the ID.
  static uint64_t counter = 0;
  std::string underlyingString;
  llvm::raw_string_ostream ss(underlyingString);
  ss << "ExternalDispatcherModule_" << counter;
  moduleIDs.push_back(ss.str()); // moduleIDs now has a copy
  ++counter;                     // Increment for next call
  return moduleIDs.back();
}

void *ExternalDispatcherImpl::resolveSymbol(const std::string &name) {
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
  if (name[0] == 1 && str[0] == '_') {
    ++str;
    addr = sys::DynamicLibrary::SearchForAddressOfSymbol(str);
  }

  return addr;
}

ExternalDispatcherImpl::ExternalDispatcherImpl(LLVMContext &ctx)
    : ctx(ctx), lastErrno(0) {
  std::string error;
  singleDispatchModule = new Module(getFreshModuleID(), ctx);
  // The MCJIT JITs whole modules at a time rather than individual functions
  // so we will let it manage the modules.
  // Note that we don't do anything with `singleDispatchModule`. This is just
  // so we can use the EngineBuilder API.
  auto dispatchModuleUniq = std::unique_ptr<Module>(singleDispatchModule);
  executionEngine = EngineBuilder(std::move(dispatchModuleUniq))
                        .setErrorStr(&error)
                        .setEngineKind(EngineKind::JIT)
                        .create();

  if (!executionEngine) {
    llvm::errs() << "unable to make jit: " << error << "\n";
    abort();
  }

  // If we have a native target, initialize it to ensure it is linked in and
  // usable by the JIT.
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmParser();
  llvm::InitializeNativeTargetAsmPrinter();

  // from ExecutionEngine::create
  if (executionEngine) {
    // Make sure we can resolve symbols in the program as well. The zero arg
    // to the function tells DynamicLibrary to load the program, not a library.
    sys::DynamicLibrary::LoadLibraryPermanently(0);
  }

#ifdef WINDOWS
  preboundFunctions["getpid"] = (void *)(long)getpid;
  preboundFunctions["putchar"] = (void *)(long)putchar;
  preboundFunctions["printf"] = (void *)(long)printf;
  preboundFunctions["fprintf"] = (void *)(long)fprintf;
  preboundFunctions["sprintf"] = (void *)(long)sprintf;
#endif
}

ExternalDispatcherImpl::~ExternalDispatcherImpl() {
  delete executionEngine;
  // NOTE: the `executionEngine` owns all modules so
  // we don't need to delete any of them.
}

//利用一个map来保存那些JIT已经建模处理过的指令，从而避免重复处理之前已经建模过的指令
bool ExternalDispatcherImpl::executeCall(KCallable *callable, Instruction *i,
                                         uint64_t *args) {
  ++stats::externalCalls;
  dispatchers_ty::iterator it = dispatchers.find(i);
  if (it != dispatchers.end()) {
    // Code already JIT'ed for this
    return runProtectedCall(it->second, args); //如果dispatchers中保存了指令i对应的函数，就直接运行该函数，it->second是函数指针，args是具体值参数
  }

  // Code for this not JIT'ed. Do this now.
  Function *dispatcher;
#ifdef WINDOWS
  std::map<std::string, void *>::iterator it2 =
      preboundFunctions.find(f->getName());

  if (it2 != preboundFunctions.end()) {
    // only bind once
    if (it2->second) {
      executionEngine->addGlobalMapping(f, it2->second);
      it2->second = 0;
    }
  }
#endif

  Module *dispatchModule = NULL;
  // The MCJIT generates whole modules at a time so for every call that we
  // haven't made before we need to create a new Module.
  // 这里的意思应该是MCJIT一次为待测程序生成全部的modules，所以对于那些之前未处理过的新的函数调用，要生成新的module
  dispatchModule = new Module(getFreshModuleID(), ctx); //生成新的Mudule对象，为其分配一个唯一的module ID
  dispatcher = createDispatcher(callable, i, dispatchModule); //在dispatchModule中为callable生成一个代理stub函数，里面可能只有一些最基本的内容，用dispatcher指向这个stub函数
  dispatchers.insert(std::make_pair(i, dispatcher)); //在map中更新指令i与函数dispatcher的映射

  // Force the JIT execution engine to go ahead and build the function. This
  // ensures that any errors or assertions in the compilation process will
  // trigger crashes instead of being caught as aborts in the external
  // function.
  //按注释理解，这里dispatcher生成成功后，MCJIT会build这个stub函数，生成可执行代码
  if (dispatcher) {
    // The dispatchModule is now ready so tell MCJIT to generate the code for
    // it.
    auto dispatchModuleUniq = std::unique_ptr<Module>(dispatchModule);
    executionEngine->addModule(
        std::move(dispatchModuleUniq)); // MCJIT takes ownership 将dispatchModule添加到MCJIT中，使MCJIT接管对dispatchModule的管理
    // Force code generation
    uint64_t fnAddr =
        executionEngine->getFunctionAddress(dispatcher->getName().str()); //这里应该是stub函数的函数名fnName，通过它得到函数地址
    executionEngine->finalizeObject(); //将dispatcher对应的模块中的IR代码生成为可执行代码
    assert(fnAddr && "failed to get function address");
    (void)fnAddr;
  } else {
    // MCJIT didn't take ownership of the module so delete it.
    delete dispatchModule;
  }
  return runProtectedCall(dispatcher, args);
}

// FIXME: This is not reentrant.
static uint64_t *gTheArgsP;
bool ExternalDispatcherImpl::runProtectedCall(Function *f, uint64_t *args) { //在一个受保护的环境中调用JIT编译后的函数
  struct sigaction segvAction, segvActionOld;
  bool res;

  if (!f)
    return false;

  std::vector<GenericValue> gvArgs; //genericValue是llvm的类，表示通用的值，可以在JIT编译器中传递和存储不同类型的值，例如整数、浮点数、指针等，而不需要明确指定具体的类型。
  gTheArgsP = args;//将args保存到全局变量gTheArgsP
  //这一段都是对系统信号进行处理，本质上应该是为了为实际执行函数调用准备环境
  segvAction.sa_handler = nullptr;
  sigemptyset(&(segvAction.sa_mask));
  sigaddset(&(segvAction.sa_mask), SIGSEGV);
  segvAction.sa_flags = SA_SIGINFO;
  segvAction.sa_sigaction = ::sigsegv_handler;
  sigaction(SIGSEGV, &segvAction, &segvActionOld);

  if (sigsetjmp(escapeCallJmpBuf, 1)) {
    res = false;
  } else {
    errno = lastErrno;
    executionEngine->runFunction(f, gvArgs); //调用llvm内部的函数来执行函数，传入一个经过JIT编译后的LLVM函数f，以及f所需的参数集合
    // Explicitly acquire errno information
    lastErrno = errno;
    res = true;
  }

  sigaction(SIGSEGV, &segvActionOld, nullptr);
  return res;
}

// FIXME: This might have been relevant for the old JIT but the MCJIT
// has a completly different implementation so this comment below is
// likely irrelevant and misleading.
//
// For performance purposes we construct the stub in such a way that the
// arguments pointer is passed through the static global variable gTheArgsP in
// this file. This is done so that the stub function prototype trivially matches
// the special cases that the JIT knows how to directly call. If this is not
// done, then the jit will end up generating a nullary stub just to call our
// stub, for every single function call.
// 个人理解，这个函数的主要作用就是利用被调函数target的一系列信息创建一个stub函数，用来代理target函数的调用
// 在函数内会创建一个新的函数并绑定到模块module中，并为其添加一些基本的内容，最终返回指向这个新函数的指针
Function *ExternalDispatcherImpl::createDispatcher(KCallable *target,
                                                   Instruction *inst,
                                                   Module *module) {
  if (isa<KFunction>(target) && !resolveSymbol(target->getName().str()))
    return 0;

  const CallBase &cb = cast<CallBase>(*inst);
  Value **args = new Value *[cb.arg_size()];

  std::vector<Type *> nullary;

  // MCJIT functions need unique names, or wrong function can be called.
  // The module identifier is included because for the MCJIT we need
  // unique function names across all `llvm::Modules`s.
  std::string fnName =
      "dispatcher_" + target->getName().str() + module->getModuleIdentifier(); //创建一个独一无二的函数名
  Function *dispatcher =
      Function::Create(FunctionType::get(Type::getVoidTy(ctx), nullary, false),
                       GlobalVariable::ExternalLinkage, fnName, module); //创建一个新的Function对象，名为fnName，将它添加到模块module中，链接类型为ExternalLinkage

  BasicBlock *dBB = BasicBlock::Create(ctx, "entry", dispatcher);//在dispatcher指向的函数（fnName）中创建一个名为entry的基本块

  llvm::IRBuilder<> Builder(dBB); //使用llvm的IRBuilder类来生成目标代码
  // Get a Value* for &gTheArgsP, as an i64**.
  auto argI64sp = Builder.CreateIntToPtr(
      ConstantInt::get(Type::getInt64Ty(ctx), (uintptr_t)(void *)&gTheArgsP),
      PointerType::getUnqual(PointerType::getUnqual(Type::getInt64Ty(ctx))),
      "argsp");
  auto argI64s = Builder.CreateLoad(
      argI64sp->getType()->getPointerElementType(), argI64sp, "args");

  // Get the target function type.
  FunctionType *FTy = target->getFunctionType();

  // Each argument will be passed by writing it into gTheArgsP[i].
  unsigned i = 0, idx = 2;
  for (auto ai = cb.arg_begin(), ae = cb.arg_end(); ai != ae; ++ai, ++i) {
    // Determine the type the argument will be passed as. This accommodates for
    // the corresponding code in Executor.cpp for handling calls to bitcasted
    // functions.
    auto argTy =
        (i < FTy->getNumParams() ? FTy->getParamType(i) : (*ai)->getType());

    // fp80 must be aligned to 16 according to the System V AMD 64 ABI
    if (argTy->isX86_FP80Ty() && idx & 0x01)
      idx++;

    auto argI64p =
        Builder.CreateGEP(argI64s->getType()->getPointerElementType(), argI64s,
                          ConstantInt::get(Type::getInt32Ty(ctx), idx));

    auto argp = Builder.CreateBitCast(argI64p, PointerType::getUnqual(argTy));
    args[i] =
        Builder.CreateLoad(argp->getType()->getPointerElementType(), argp);

    unsigned argSize = argTy->getPrimitiveSizeInBits();
    idx += ((!!argSize ? argSize : 64) + 63) / 64;
  }
  //上面的代码应该是根据参数类型和参数个数将参数从调用指令中提取出来，写入全局变量gTheArgsP

  llvm::CallInst *result;
  if (auto* func = dyn_cast<KFunction>(target)) {
    auto dispatchTarget = module->getOrInsertFunction(target->getName(), FTy,
                                                      func->function->getAttributes());
    result = Builder.CreateCall(dispatchTarget,
                                llvm::ArrayRef<Value *>(args, args + i));//创建一个调用指令来调用目标函数
  } else if (auto* asmValue = dyn_cast<KInlineAsm>(target)) {
    result = Builder.CreateCall(asmValue->getInlineAsm(),
                                llvm::ArrayRef<Value *>(args, args + i));
  } else {
    assert(0 && "Unhandled KCallable derived class");
  }
  if (result->getType() != Type::getVoidTy(ctx)) { //如果目标函数的返回类型不是void，将返回值也保存到全局变量gTheArgsP
    auto resp = Builder.CreateBitCast(
        argI64s, PointerType::getUnqual(result->getType()));
    Builder.CreateStore(result, resp);
  }

  Builder.CreateRetVoid(); //创建一个空的返回指令来结束dispatcher函数

  delete[] args;

  return dispatcher;
}

int ExternalDispatcherImpl::getLastErrno() { return lastErrno; }
void ExternalDispatcherImpl::setLastErrno(int newErrno) {
  lastErrno = newErrno;
}

ExternalDispatcher::ExternalDispatcher(llvm::LLVMContext &ctx)
    : impl(new ExternalDispatcherImpl(ctx)) {}

ExternalDispatcher::~ExternalDispatcher() { delete impl; }

bool ExternalDispatcher::executeCall(KCallable *callable,
                                     llvm::Instruction *i, uint64_t *args) {
  return impl->executeCall(callable, i, args);
}

void *ExternalDispatcher::resolveSymbol(const std::string &name) {
  return impl->resolveSymbol(name);
}

int ExternalDispatcher::getLastErrno() { return impl->getLastErrno(); }
void ExternalDispatcher::setLastErrno(int newErrno) {
  impl->setLastErrno(newErrno);
}
}
