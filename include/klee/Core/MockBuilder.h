//===-- MockBuilder.h -------------------------------------------*- C++ -*-===//
//
//                     The KLEEF Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//===----------------------------------------------------------------------===//

#ifndef KLEE_MOCKBUILDER_H
#define KLEE_MOCKBUILDER_H

#include "klee/Core/Interpreter.h"
#include "klee/Module/Annotation.h"

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"

#include <set>
#include <string>

namespace klee {

class MockBuilder {
private:
  const llvm::Module *userModule;
  llvm::LLVMContext &ctx;
  std::unique_ptr<llvm::Module> mockModule;
  std::unique_ptr<llvm::IRBuilder<>> builder;

  const Interpreter::ModuleOptions &opts;
  const Interpreter::InterpreterOptions &interpreterOptions;

  std::set<std::string> ignoredExternals;
  std::vector<std::pair<std::string, std::string>> redefinitions;

  InterpreterHandler *interpreterHandler;

  std::set<std::string> &mainModuleFunctions;
  std::set<std::string> &mainModuleGlobals;

  AnnotationsMap annotations;

  void initMockModule();
  void buildMockMain();
  void buildExternalGlobalsDefinitions();
  void buildExternalFunctionsDefinitions();
  void
  buildCallKleeMakeSymbolic(const std::string &kleeMakeSymbolicFunctionName,
                            llvm::Value *source, llvm::Type *type,
                            const std::string &symbolicName);

  void buildAnnotationForExternalFunctionArgs(
      llvm::Function *func,
      const std::vector<std::vector<Statement::Ptr>> &statementsNotAllign);
  void buildAnnotationForExternalFunctionReturn(
      llvm::Function *func, const std::vector<Statement::Ptr> &statements);
  void buildAnnotationForExternalFunctionProperties(
      llvm::Function *func, const std::set<Statement::Property> &properties);

  std::map<std::string, llvm::FunctionType *> getExternalFunctions();
  std::map<std::string, llvm::Type *> getExternalGlobals();

  std::pair<llvm::Value *, llvm::Value *>
  goByOffset(llvm::Value *value, const std::vector<std::string> &offset);

public:
  MockBuilder(const llvm::Module *initModule,
              const Interpreter::ModuleOptions &opts,
              const Interpreter::InterpreterOptions &interpreterOptions,
              const std::set<std::string> &ignoredExternals,
              std::vector<std::pair<std::string, std::string>> &redefinitions,
              InterpreterHandler *interpreterHandler,
              std::set<std::string> &mainModuleFunctions,
              std::set<std::string> &mainModuleGlobals);

  std::unique_ptr<llvm::Module> build();
  void buildAllocSource(llvm::Value *prev, llvm::Type *elemType,
                        const Statement::Alloc *allocSourcePtr);
  void buildFree(llvm::Value *elem, const Statement::Free *freePtr);
  void processingValue(llvm::Value *prev, llvm::Type *elemType,
                       const Statement::Alloc *allocSourcePtr,
                       bool initNullPtr);
};

} // namespace klee

#endif // KLEE_MOCKBUILDER_H
