//===-- KInstruction.h ------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_KINSTRUCTION_H
#define KLEE_KINSTRUCTION_H

#include "KModule.h"
#include "klee/Config/Version.h"
#include "klee/Support/CompilerWarning.h"

DISABLE_WARNING_PUSH
DISABLE_WARNING_DEPRECATED_DECLARATIONS
#include "llvm/Support/DataTypes.h"
#include "llvm/Support/raw_ostream.h"
DISABLE_WARNING_POP

#include <unordered_map>
#include <vector>

namespace llvm {
class Instruction;
}

namespace klee {
class Executor;
class KModule;
struct KBlock;

/// KInstruction - Intermediate instruction representation used
/// during execution.
struct KInstruction {
  llvm::Instruction *inst;

  /// Value numbers for each operand. -1 is an invalid value,
  /// otherwise negative numbers are indices (negated and offset by
  /// 2) into the module constant table and positive numbers are
  /// register indices.
  int *operands;
  KBlock *parent;

private:
  // Instruction index in the basic block
  const unsigned globalIndex;

public:
  /// Unique index for KFunction and KInstruction inside KModule
  /// from 0 to [KFunction + KInstruction]
  [[nodiscard]] unsigned getGlobalIndex() const;
  /// Instruction index in the basic block
  [[nodiscard]] unsigned getIndex() const;
  /// Destination register index.
  [[nodiscard]] unsigned getDest() const;

  KInstruction(const std::unordered_map<llvm::Instruction *, unsigned>
                   &_instructionToRegisterMap,
               llvm::Instruction *_inst, KModule *_km, KBlock *_kb,
               unsigned &_globalIndexInc);

  KInstruction() = delete;
  explicit KInstruction(const KInstruction &ki) = delete;
  virtual ~KInstruction();

  [[nodiscard]] size_t getLine() const;
  [[nodiscard]] size_t getColumn() const;
  [[nodiscard]] std::string getSourceFilepath() const;

  [[nodiscard]] std::string getSourceLocationString() const;
  [[nodiscard]] std::string toString() const;

  [[nodiscard]] inline KBlock *getKBlock() const { return parent; }
  [[nodiscard]] inline KFunction *getKFunction() const {
    return getKBlock()->parent;
  }
  [[nodiscard]] inline KModule *getKModule() const {
    return getKFunction()->parent;
  }
};

struct KGEPInstruction : KInstruction {
  /// indices - The list of variable sized adjustments to add to the pointer
  /// operand to execute the instruction. The first element is the operand
  /// index into the GetElementPtr instruction, and the second element is the
  /// element size to multiple that index by.
  std::vector<std::pair<unsigned, uint64_t>> indices;

  /// offset - A constant offset to add to the pointer operand to execute the
  /// instruction.
  uint64_t offset;

public:
  KGEPInstruction(const std::unordered_map<llvm::Instruction *, unsigned>
                      &_instructionToRegisterMap,
                  llvm::Instruction *_inst, KModule *_km, KBlock *_kb,
                  unsigned &_globalIndexInc)
      : KInstruction(_instructionToRegisterMap, _inst, _km, _kb,
                     _globalIndexInc) {}
  KGEPInstruction() = delete;
  explicit KGEPInstruction(const KGEPInstruction &ki) = delete;
};
} // namespace klee

#endif /* KLEE_KINSTRUCTION_H */
