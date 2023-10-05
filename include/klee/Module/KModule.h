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
#include "klee/Module/KCallable.h"

#include "klee/Support/CompilerWarning.h"
DISABLE_WARNING_PUSH
DISABLE_WARNING_DEPRECATED_DECLARATIONS
#include "llvm/ADT/ArrayRef.h"
#include "llvm/IR/CFG.h"
DISABLE_WARNING_POP

#include <deque>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

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
Function *getTargetFunction(Value *calledVal);
} // namespace llvm

namespace klee {
struct Cell;
class Executor;
class Expr;
class InterpreterHandler;
struct KInstruction;
class KModule;
struct KFunction;
struct KCallBlock;
template <class T> class ref;

enum KBlockType { Base, Call, Return };

struct KBlock {
  KFunction *parent;
  llvm::BasicBlock *basicBlock;
  KInstruction **instructions;

public:
  KBlock(KFunction *, llvm::BasicBlock *, KModule *,
         const std::unordered_map<llvm::Instruction *, unsigned> &,
         KInstruction **, unsigned &globalIndexInc);
  KBlock() = delete;
  KBlock(const KBlock &) = delete;
  KBlock &operator=(const KBlock &) = delete;
  virtual ~KBlock() = default;

  virtual KBlockType getKBlockType() const { return KBlockType::Base; }
  static bool classof(const KBlock *) { return true; }

  unsigned getNumInstructions() const noexcept { return basicBlock->size(); }
  KInstruction *getFirstInstruction() const noexcept { return instructions[0]; }
  KInstruction *getLastInstruction() const noexcept {
    return instructions[getNumInstructions() - 1];
  }
  std::string getLabel() const;
  std::string toString() const;

  /// Block number in function
  [[nodiscard]] uintptr_t getId() const;
};

typedef std::function<bool(KBlock *)> KBlockPredicate;

struct KCallBlock : KBlock {
  KInstruction *kcallInstruction;
  std::set<llvm::Function *> calledFunctions;

public:
  KCallBlock() = delete;
  KCallBlock(KFunction *, llvm::BasicBlock *, KModule *,
             const std::unordered_map<llvm::Instruction *, unsigned> &,
             std::set<llvm::Function *>, KInstruction **,
             unsigned &globalIndexInc);
  static bool classof(const KCallBlock *) { return true; }
  static bool classof(const KBlock *E) {
    return E->getKBlockType() == KBlockType::Call;
  }
  KBlockType getKBlockType() const override { return KBlockType::Call; };
  bool intrinsic() const;
  bool internal() const;
  bool kleeHandled() const;
  KFunction *getKFunction() const;
};

struct KReturnBlock : KBlock {
public:
  KReturnBlock() = delete;
  KReturnBlock(KFunction *, llvm::BasicBlock *, KModule *,
               const std::unordered_map<llvm::Instruction *, unsigned> &,
               KInstruction **, unsigned &globalIndexInc);
  static bool classof(const KReturnBlock *) { return true; }
  static bool classof(const KBlock *E) {
    return E->getKBlockType() == KBlockType::Return;
  }
  KBlockType getKBlockType() const override { return KBlockType::Return; };
};

struct KFunction : public KCallable {
private:
  std::unordered_map<std::string, KBlock *> labelMap;
  const unsigned globalIndex;

public:
  KModule *parent;
  llvm::Function *function;
  KInstruction **instructions;

  [[nodiscard]] KInstruction *getInstructionByRegister(size_t reg) const;

  std::unordered_map<const llvm::Instruction *, KInstruction *> instructionMap;
  std::vector<std::unique_ptr<KBlock>> blocks;
  std::unordered_map<const llvm::BasicBlock *, KBlock *> blockMap;
  KBlock *entryKBlock;
  std::vector<KBlock *> returnKBlocks;
  std::vector<KCallBlock *> kCallBlocks;

  /// count of instructions in function
  unsigned numInstructions;
  [[nodiscard]] size_t getNumArgs() const;
  [[nodiscard]] size_t getNumRegisters() const;
  unsigned id;

  bool kleeHandled = false;

  explicit KFunction(llvm::Function *, KModule *, unsigned &);
  KFunction() = delete;
  KFunction(const KFunction &) = delete;
  KFunction &operator=(const KFunction &) = delete;

  ~KFunction();

  unsigned getArgRegister(unsigned index) const { return index; }

  llvm::StringRef getName() const override {
    return function ? function->getName() : "";
  }

  llvm::FunctionType *getFunctionType() const override {
    return function->getFunctionType();
  }

  const std::unordered_map<std::string, KBlock *> &getLabelMap() {
    if (labelMap.size() == 0) {
      for (auto &kb : blocks) {
        labelMap[kb->getLabel()] = kb.get();
      }
    }
    return labelMap;
  }

  llvm::Value *getValue() override { return function; }

  static bool classof(const KCallable *callable) {
    return callable->getKind() == CK_Function;
  }

  [[nodiscard]] size_t getLine() const;
  [[nodiscard]] std::string getSourceFilepath() const;

  /// Unique index for KFunction and KInstruction inside KModule
  /// from 0 to [KFunction + KInstruction]
  [[nodiscard]] inline unsigned getGlobalIndex() const { return globalIndex; }
};

struct KBlockCompare {
  bool operator()(const KBlock *a, const KBlock *b) const {
    return a->parent->getGlobalIndex() < b->parent->getGlobalIndex() ||
           (a->parent->getGlobalIndex() == b->parent->getGlobalIndex() &&
            a->getId() < b->getId());
  }
};

class KConstant {
public:
  /// Actual LLVM constant this represents.
  llvm::Constant *ct;

  /// The constant ID.
  unsigned id;

  /// First instruction where this constant was encountered, or NULL
  /// if not applicable/unavailable.
  KInstruction *ki;

  KConstant(llvm::Constant *, unsigned, KInstruction *);
};

class KModule {
private:
  bool withPosixRuntime; // TODO move to opts
  unsigned maxGlobalIndex;

public:
  std::unique_ptr<llvm::Module> module;
  std::unique_ptr<llvm::DataLayout> targetData;

  // Our shadow versions of LLVM structures.
  std::vector<std::unique_ptr<KFunction>> functions;
  std::unordered_map<const llvm::Function *, KFunction *> functionMap;
  std::unordered_map<llvm::Function *, std::set<llvm::Function *>> callMap;
  std::unordered_map<std::string, KFunction *> functionNameMap;
  [[nodiscard]] unsigned getFunctionId(const llvm::Function *) const;

  // Functions which escape (may be called indirectly)
  // XXX change to KFunction
  std::set<llvm::Function *> escapingFunctions;

  std::set<std::string> mainModuleFunctions;
  std::set<std::string> mainModuleGlobals;

  FLCtoOpcode origInstructions;

  std::vector<llvm::Constant *> constants;
  std::unordered_map<const llvm::Constant *, std::unique_ptr<KConstant>>
      constantMap;
  KConstant *getKConstant(const llvm::Constant *c);

  std::unique_ptr<Cell[]> constantTable;

  // Functions which are part of KLEE runtime
  std::set<const llvm::Function *> internalFunctions;

  // instruction to assembly.ll line empty if no statistic enabled
  std::unordered_map<uintptr_t, uint64_t> asmLineMap;

  // Mark function with functionName as part of the KLEE runtime
  void addInternalFunction(const char *functionName);
  // Replace std functions with KLEE intrinsics
  void replaceFunction(const std::unique_ptr<llvm::Module> &m,
                       const char *original, const char *replacement);

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
  void manifest(InterpreterHandler *ih, Interpreter::GuidanceKind guidance,
                bool forceSourceOutput);

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
            const unsigned flag);

  void instrument(const Interpreter::ModuleOptions &opts);

  /// Return an id for the given constant, creating a new one if necessary.
  unsigned getConstantID(llvm::Constant *c, KInstruction *ki);

  /// Run passes that check if module is valid LLVM IR and if invariants
  /// expected by KLEE's Executor hold.
  void checkModule();

  KBlock *getKBlock(llvm::BasicBlock *bb);

  bool inMainModule(const llvm::Function &f);

  bool inMainModule(const llvm::GlobalVariable &v);

  bool WithPOSIXRuntime() { return withPosixRuntime; }

  std::optional<size_t> getAsmLine(const uintptr_t ref) const;
  std::optional<size_t> getAsmLine(const llvm::Function *func) const;
  std::optional<size_t> getAsmLine(const llvm::Instruction *inst) const;

  inline unsigned getMaxGlobalIndex() const { return maxGlobalIndex; }
  unsigned getGlobalIndex(const llvm::Function *func) const;
  unsigned getGlobalIndex(const llvm::Instruction *inst) const;
};
} // namespace klee

#endif /* KLEE_KMODULE_H */
