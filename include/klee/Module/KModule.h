/*
 * This source file has been modified by Huawei. Copyright (c) 2021
 */

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
#include "klee/Core/Interpreter.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/IR/CFG.h"

#include <map>
#include <memory>
#include <set>
#include <vector>
#include <deque>

namespace llvm {
  class BasicBlock;
  class Constant;
  class Function;
  class Value;
  class Instruction;
  class Module;
  class DataLayout;

  /// Compute the true target of a function call, resolving LLVM aliases
  /// and bitcasts.
  llvm::Function* getTargetFunction(llvm::Value *calledVal);
}

namespace klee {
  struct Cell;
  class Executor;
  class Expr;
  class InterpreterHandler;
  class InstructionInfoTable;
  struct KInstruction;
  class KModule;
  struct KFunction;
  template<class T> class ref;

  enum KBlockType {
    Base,
    Call,
  };

  struct KBlock {
    KFunction *parent;
    llvm::BasicBlock *basicBlock;

    unsigned numInstructions;
    KInstruction **instructions;

    /// Whether instructions in this function should count as
    /// "coverable" for statistics and search heuristics.
    bool trackCoverage;

  public:
    explicit KBlock(KFunction*, llvm::BasicBlock*, KModule*,
                    std::map<llvm::Instruction*, unsigned>&,
                    std::map<unsigned, KInstruction*>&,
                    KInstruction **);
    KBlock(const KBlock &) = delete;
    KBlock &operator=(const KBlock &) = delete;

    ~KBlock();

    unsigned getArgRegister(unsigned index) const { return index; }
    void handleKInstruction(std::map<llvm::Instruction*, unsigned> &registerMap,
                            llvm::Instruction *inst, KModule *km, KInstruction *ki);
    virtual KBlockType getKBlockType() const { return KBlockType::Base; };
  };

  struct KCallBlock : KBlock {
    KInstruction *kcallInstruction;
    llvm::Function *calledFunction;

  public:
    explicit KCallBlock(KFunction*, llvm::BasicBlock*, KModule*,
                    std::map<llvm::Instruction*, unsigned>&, std::map<unsigned, KInstruction*>&,
                    llvm::Function*, KInstruction **);
    KBlockType getKBlockType() const override { return KBlockType::Call; };
  };

  struct KFunction {
    KModule *parent;
    llvm::Function *function;

    unsigned numArgs, numRegisters;

    std::map<unsigned, KInstruction*> reg2inst;
    unsigned numInstructions;
    unsigned numBlocks;
    KInstruction **instructions;

    std::map<llvm::Instruction*, KInstruction*> instructionMap;
    std::vector<std::unique_ptr<KBlock>> blocks;
    std::map<llvm::BasicBlock*, KBlock*> blockMap;
    KBlock *entryKBlock;
    std::vector<KBlock*> finalKBlocks;
    std::vector<KCallBlock*> kCallBlocks;

    /// Whether instructions in this function should count as
    /// "coverable" for statistics and search heuristics.
    bool trackCoverage;

  private:
    std::map<KBlock*, std::map<KBlock*, unsigned int>> distance;
    std::map<KBlock*, std::map<KBlock*, unsigned int>> backwardDistance;
    // BFS algorithm
    void calculateDistance(KBlock *bb);
    void calculateBackwardDistance(KBlock *bb);

  public:
    explicit KFunction(llvm::Function*, KModule *);
    KFunction(const KFunction &) = delete;
    KFunction &operator=(const KFunction &) = delete;

    ~KFunction();

    unsigned getArgRegister(unsigned index) const { return index; }
    std::map<KBlock*, unsigned int>& getDistance(KBlock *kb);
    std::map<KBlock*, unsigned int>& getBackwardDistance(KBlock *kb);
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
    std::unique_ptr<llvm::Module> module;
    std::unique_ptr<llvm::DataLayout> targetData;

    // Our shadow versions of LLVM structures.
    std::vector<std::unique_ptr<KFunction>> functions;
    std::map<llvm::Function*, KFunction*> functionMap;
    std::map<llvm::Function*, std::set<llvm::Function*>> callMap;

    // Functions which escape (may be called indirectly)
    // XXX change to KFunction
    std::set<llvm::Function*> escapingFunctions;

    std::unique_ptr<InstructionInfoTable> infos;

    std::vector<llvm::Constant*> constants;
    std::map<const llvm::Constant *, std::unique_ptr<KConstant>> constantMap;
    KConstant* getKConstant(const llvm::Constant *c);

    std::unique_ptr<Cell[]> constantTable;

    // Functions which are part of KLEE runtime
    std::set<const llvm::Function*> internalFunctions;

  private:
    std::map<KFunction*, std::map<KFunction*, unsigned int>> distance;
    std::map<KFunction*, std::map<KFunction*, unsigned int>> backwardDistance;

    // Mark function with functionName as part of the KLEE runtime
    void addInternalFunction(const char* functionName);
    // Replace std functions with KLEE intrinsics
    void replaceFunction(const std::unique_ptr<llvm::Module> &m, const char *original,
                           const char *replacement);

    // BFS algorithm
    void calculateDistance(KFunction *kf);
    void calculateBackwardDistance(KFunction *kf);

  public:
    KModule() = default;

    /// Optimise and prepare module such that KLEE can execute it
    //
    void optimiseAndPrepare(const Interpreter::ModuleOptions &opts,
                            llvm::ArrayRef<const char *>);

    /// Manifest the generated module (e.g. assembly.ll, output.bc) and
    /// prepares KModule
    ///
    /// @param ih
    /// @param forceSourceOutput true if assembly.ll should be created
    ///
    // FIXME: ihandler should not be here
    void manifest(InterpreterHandler *ih, bool forceSourceOutput);

    /// Link the provided modules together as one KLEE module.
    ///
    /// If the entry point is empty, all modules are linked together.
    /// If the entry point is not empty, all modules are linked which resolve
    /// the dependencies of the module containing entryPoint
    ///
    /// @param modules list of modules to be linked together
    /// @param entryPoint name of the function which acts as the program's entry
    /// point
    /// @return true if at least one module has been linked in, false if nothing
    /// changed
    bool link(std::vector<std::unique_ptr<llvm::Module>> &modules,
              const std::string &entryPoint);

    void instrument(const Interpreter::ModuleOptions &opts);

    /// Return an id for the given constant, creating a new one if necessary.
    unsigned getConstantID(llvm::Constant *c, KInstruction* ki);

    /// Run passes that check if module is valid LLVM IR and if invariants
    /// expected by KLEE's Executor hold.
    void checkModule();

    KBlock *getKBlock(llvm::BasicBlock *bb);
    std::map<KFunction*, unsigned int>& getBackwardDistance(KFunction *kf);
    std::map<KFunction*, unsigned int>& getDistance(KFunction *kf);
  };
} // End klee namespace

#endif /* KLEE_KMODULE_H */
