#include "klee/Core/MockBuilder.h"

#include "klee/Support/ErrorHandling.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"

#include <memory>

namespace klee {

MockBuilder::MockBuilder(const llvm::Module *initModule,
                         std::string mockEntrypoint, std::string userEntrypoint,
                         std::map<std::string, llvm::Type *> externals)
    : userModule(initModule), externals(std::move(externals)),
      mockEntrypoint(std::move(mockEntrypoint)),
      userEntrypoint(std::move(userEntrypoint)) {}

std::unique_ptr<llvm::Module> MockBuilder::build() {
  initMockModule();
  buildMockMain();
  buildExternalFunctionsDefinitions();
  return std::move(mockModule);
}

void MockBuilder::initMockModule() {
  mockModule = std::make_unique<llvm::Module>(userModule->getName().str() +
                                                  "__klee_externals",
                                              userModule->getContext());
  mockModule->setTargetTriple(userModule->getTargetTriple());
  mockModule->setDataLayout(userModule->getDataLayout());
  builder = std::make_unique<llvm::IRBuilder<>>(mockModule->getContext());
}

// Set up entrypoint in new module. Here we'll define external globals and then
// call user's entrypoint.
void MockBuilder::buildMockMain() {
  llvm::Function *userMainFn = userModule->getFunction(userEntrypoint);
  if (!userMainFn) {
    klee_error("Entry function '%s' not found in module.",
               userEntrypoint.c_str());
  }
  mockModule->getOrInsertFunction(mockEntrypoint, userMainFn->getFunctionType(),
                                  userMainFn->getAttributes());
  llvm::Function *mockMainFn = mockModule->getFunction(mockEntrypoint);
  if (!mockMainFn) {
    klee_error("Entry function '%s' not found in module.",
               mockEntrypoint.c_str());
  }
  auto globalsInitBlock =
      llvm::BasicBlock::Create(mockModule->getContext(), "entry", mockMainFn);
  builder->SetInsertPoint(globalsInitBlock);
  // Define all the external globals
  buildExternalGlobalsDefinitions();

  auto userMainCallee = mockModule->getOrInsertFunction(
      userEntrypoint, userMainFn->getFunctionType());
  std::vector<llvm::Value *> args;
  args.reserve(userMainFn->arg_size());
  for (auto it = mockMainFn->arg_begin(); it != mockMainFn->arg_end(); it++) {
    args.push_back(it);
  }
  auto callUserMain = builder->CreateCall(userMainCallee, args);
  builder->CreateRet(callUserMain);
}

void MockBuilder::buildExternalGlobalsDefinitions() {
  for (const auto &it : externals) {
    if (it.second->isFunctionTy()) {
      continue;
    }
    const std::string &extName = it.first;
    llvm::Type *type = it.second;
    mockModule->getOrInsertGlobal(extName, type);
    auto *global = mockModule->getGlobalVariable(extName);
    if (!global) {
      klee_error("Unable to add global variable '%s' to module",
                 extName.c_str());
    }
    global->setLinkage(llvm::GlobalValue::ExternalLinkage);
    if (!type->isSized()) {
      continue;
    }

    auto *zeroInitializer = llvm::Constant::getNullValue(it.second);
    if (!zeroInitializer) {
      klee_error("Unable to get zero initializer for '%s'", extName.c_str());
    }
    global->setInitializer(zeroInitializer);
    buildCallKleeMakeSymbol("klee_make_symbolic", global, type,
                            "@obj_" + extName);
  }
}

void MockBuilder::buildExternalFunctionsDefinitions() {
  for (const auto &it : externals) {
    if (!it.second->isFunctionTy()) {
      continue;
    }
    std::string extName = it.first;
    auto *type = llvm::cast<llvm::FunctionType>(it.second);
    mockModule->getOrInsertFunction(extName, type);
    llvm::Function *func = mockModule->getFunction(extName);
    if (!func) {
      klee_error("Unable to find function '%s' in module", extName.c_str());
    }
    if (!func->empty()) {
      continue;
    }
    auto *BB =
        llvm::BasicBlock::Create(mockModule->getContext(), "entry", func);
    builder->SetInsertPoint(BB);

    if (!func->getReturnType()->isSized()) {
      builder->CreateRet(nullptr);
      continue;
    }

    auto *mockReturnValue =
        builder->CreateAlloca(func->getReturnType(), nullptr);
    buildCallKleeMakeSymbol("klee_make_mock", mockReturnValue,
                            func->getReturnType(), "@call_" + extName);
    auto *loadInst = builder->CreateLoad(mockReturnValue, "klee_var");
    builder->CreateRet(loadInst);
  }
}

void MockBuilder::buildCallKleeMakeSymbol(const std::string &klee_function_name,
                                          llvm::Value *source, llvm::Type *type,
                                          const std::string &symbol_name) {
  auto *klee_mk_symb_type = llvm::FunctionType::get(
      llvm::Type::getVoidTy(mockModule->getContext()),
      {llvm::Type::getInt8PtrTy(mockModule->getContext()),
       llvm::Type::getInt64Ty(mockModule->getContext()),
       llvm::Type::getInt8PtrTy(mockModule->getContext())},
      false);
  auto kleeMakeSymbolCallee =
      mockModule->getOrInsertFunction(klee_function_name, klee_mk_symb_type);
  auto bitcastInst = builder->CreateBitCast(
      source, llvm::Type::getInt8PtrTy(mockModule->getContext()));
  auto str_name = builder->CreateGlobalString(symbol_name);
  auto gep = builder->CreateConstInBoundsGEP2_64(str_name, 0, 0);
  builder->CreateCall(
      kleeMakeSymbolCallee,
      {bitcastInst,
       llvm::ConstantInt::get(
           mockModule->getContext(),
           llvm::APInt(64, mockModule->getDataLayout().getTypeStoreSize(type),
                       false)),
       gep});
}

} // namespace klee
