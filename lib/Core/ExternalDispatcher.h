//===-- ExternalDispatcher.h ------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_EXTERNALDISPATCHER_H
#define KLEE_EXTERNALDISPATCHER_H

#include <map>
#include <string>
#include <stdint.h>

namespace llvm {
  class ExecutionEngine;
  class Instruction;
  class Function;
  class FunctionType;
  class Module;
}

namespace klee {
  class ExternalDispatcher {
  private:
    typedef std::map<const llvm::Instruction*,llvm::Function*> dispatchers_ty;
    dispatchers_ty dispatchers;
    llvm::Module *dispatchModule;
    llvm::ExecutionEngine *executionEngine;
    std::map<std::string, void*> preboundFunctions;
    
    llvm::Function *createDispatcher(llvm::Function *f, llvm::Instruction *i);
    bool runProtectedCall(llvm::Function *f, uint64_t *args);
    
  public:
    ExternalDispatcher();
    ~ExternalDispatcher();

    /* Call the given function using the parameter passing convention of
     * ci with arguments in args[1], args[2], ... and writing the result
     * into args[0].
     */
    bool executeCall(llvm::Function *function, llvm::Instruction *i, uint64_t *args);
    void *resolveSymbol(const std::string &name);
  };  
}

#endif
