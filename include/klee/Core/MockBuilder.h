#ifndef KLEE_MOCKBUILDER_H
#define KLEE_MOCKBUILDER_H

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"

#include <set>
#include <string>

namespace klee {

class MockBuilder {
private:
  const llvm::Module *userModule;
  std::unique_ptr<llvm::Module> mockModule;
  std::unique_ptr<llvm::IRBuilder<>> builder;
  std::map<std::string, llvm::Type *> externals;

  const std::string mockEntrypoint, userEntrypoint;

  void initMockModule();
  void buildMockMain();
  void buildExternalGlobalsDefinitions();
  void buildExternalFunctionsDefinitions();
  void buildCallKleeMakeSymbol(const std::string &klee_function_name,
                               llvm::Value *source, llvm::Type *type,
                               const std::string &symbol_name);

public:
  MockBuilder(const llvm::Module *initModule, std::string mockEntrypoint,
              std::string userEntrypoint,
              std::map<std::string, llvm::Type *> externals);

  std::unique_ptr<llvm::Module> build();
};

} // namespace klee

#endif // KLEE_MOCKBUILDER_H
