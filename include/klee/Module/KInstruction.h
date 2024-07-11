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
#include "KValue.h"

#include "llvm/Support/raw_ostream.h"

#include <unordered_map>
#include <vector>

namespace llvm {
class Instruction;
}

namespace klee {
class Executor;
class KModule;
struct KBlock;
struct KFunction;

static const unsigned MAGIC_HASH_CONSTANT = 39;

/// KInstruction - Intermediate instruction representation used
/// during execution.
struct KInstruction : public KValue {

  struct Index {
    unsigned long instID;
    unsigned long blockID;
    unsigned long funcID;

    bool operator==(const Index &other) const {
      return std::tie(instID, blockID, funcID) ==
             std::tie(other.instID, other.blockID, other.funcID);
    }

    bool operator<(const Index &other) const {
      return std::tie(instID, blockID, funcID) <
             std::tie(other.instID, other.blockID, other.funcID);
    }

    unsigned hash() const {
      unsigned res = instID;
      res = res * MAGIC_HASH_CONSTANT + blockID;
      res = res * MAGIC_HASH_CONSTANT + funcID;
      return res;
    }

    void print(llvm::raw_ostream &os) const {
      os << "[" << instID << ", " << blockID << ", " << funcID << "]";
    }
  };

  llvm::Instruction *inst() const {
    return llvm::dyn_cast_or_null<llvm::Instruction>(value);
  }

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
  std::string getSourceLocation() const;

  [[nodiscard]] size_t getLine() const;
  [[nodiscard]] size_t getColumn() const;
  [[nodiscard]] std::string getSourceFilepath() const;
  Index getID() const;

  [[nodiscard]] std::string getSourceLocationString() const;
  [[nodiscard]] std::string toString() const;
  bool operator==(const KInstruction &other) const {
    return getID() == other.getID();
  }

  [[nodiscard]] inline KBlock *getKBlock() const { return parent; }
  [[nodiscard]] inline KFunction *getKFunction() const {
    return getKBlock()->parent;
  }

  [[nodiscard]] bool operator<(const KValue &rhs) const override {
    if (getKind() == rhs.getKind()) {
      return getID() < cast<KInstruction>(rhs).getID();
    } else {
      return getKind() < rhs.getKind();
    }
  }

  [[nodiscard]] inline KModule *getKModule() const {
    return getKFunction()->parent;
  }

  [[nodiscard]] unsigned hash() const override { return getID().hash(); }

  static bool classof(const KValue *rhs) {
    return rhs->getKind() == Kind::INSTRUCTION;
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

struct KInstructionCompare {
  bool operator()(const KInstruction *a, const KInstruction *b) const {
    return a->getID() < b->getID();
  }
};
} // namespace klee

#endif /* KLEE_KINSTRUCTION_H */
