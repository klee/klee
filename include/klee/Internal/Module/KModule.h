//===-- KModule.h -----------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_KMODULE_H
#define KLEE_KMODULE_H

#include "klee/Config/Version.h"
#include "klee/Interpreter.h"

#include <map>
#include <set>
#include <vector>

namespace llvm {
  class BasicBlock;
  class Constant;
  class Function;
  class Instruction;
  class Module;
  class DataLayout;
}

namespace klee {
  struct Cell;
  class Executor;
  class Expr;
  class InterpreterHandler;
  class InstructionInfoTable;
  struct KInstruction;
  class KModule;
  template<class T> class ref;

  struct KFunction {
    llvm::Function *function;

    unsigned numArgs, numRegisters;

    unsigned numInstructions;
    KInstruction **instructions;

    std::map<llvm::BasicBlock*, unsigned> basicBlockEntry;

    /// Whether instructions in this function should count as
    /// "coverable" for statistics and search heuristics.
    bool trackCoverage;

  private:
    KFunction(const KFunction&);
    KFunction &operator=(const KFunction&);

  public:
    explicit KFunction(llvm::Function*, KModule *);
    ~KFunction();

    unsigned getArgRegister(unsigned index) { return index; }
  };


  class KConstant {
  public:
    /// Actual LLVM constant this represents.
    llvm::Constant* ct;

    /// The constant ID.
    unsigned id;

    /// First instruction where this constant was encountered, or NULL
    /// if not applicable/unavailable.
    KInstruction *ki;

    KConstant(llvm::Constant*, unsigned, KInstruction*);
  };


  class KModule {
  public:
    llvm::Module *module;
    llvm::DataLayout *targetData;

    // Our shadow versions of LLVM structures.
    std::vector<KFunction*> functions;
    std::map<llvm::Function*, KFunction*> functionMap;

    // Functions which escape (may be called indirectly)
    // XXX change to KFunction
    std::set<llvm::Function*> escapingFunctions;

    InstructionInfoTable *infos;

    std::vector<llvm::Constant*> constants;
    std::map<const llvm::Constant*, KConstant*> constantMap;
    KConstant* getKConstant(const llvm::Constant *c);

    Cell *constantTable;

    // Functions which are part of KLEE runtime
    std::set<const llvm::Function*> internalFunctions;

  private:
    // Mark function with functionName as part of the KLEE runtime
    void addInternalFunction(const char* functionName);

  public:
    KModule(llvm::Module *_module);
    ~KModule();

    /// Initialize local data structures.
    //
    // FIXME: ihandler should not be here
    void prepare(const Interpreter::ModuleOptions &opts, 
                 InterpreterHandler *ihandler);

    /// Return an id for the given constant, creating a new one if necessary.
    unsigned getConstantID(llvm::Constant *c, KInstruction* ki);
  };
} // End klee namespace

#endif
