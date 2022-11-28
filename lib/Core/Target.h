//===-- Target.h ------------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_TARGET_H
#define KLEE_TARGET_H

#include "klee/Module/KModule.h"
#include "klee/Support/OptionCategories.h"
#include "llvm/Support/Casting.h"

#include <map>
#include <unordered_set>

namespace klee {
class CodeGraphDistance;
class ExecutionState;
class Executor;

enum TargetCalculateBy { Default, Blocks, Transitions };

struct Target {
private:
  KBlock *block;

public:
  explicit Target(KBlock *_block) : block(_block) {}

  bool operator<(const Target &other) const { return block < other.block; }

  bool operator==(const Target &other) const { return block == other.block; }

  bool atReturn() const { return llvm::isa<KReturnBlock>(block); }

  KBlock *getBlock() const { return block; }

  bool isNull() const { return block == nullptr; }

  explicit operator bool() const noexcept { return !isNull(); }

  std::string toString() const;
};

typedef std::pair<llvm::BasicBlock *, llvm::BasicBlock *> Transition;

struct TransitionHash {
  std::size_t operator()(const Transition &p) const {
    return reinterpret_cast<size_t>(p.first) * 31 +
           reinterpret_cast<size_t>(p.second);
  }
};

class TargetCalculator {
  typedef std::unordered_set<llvm::BasicBlock *> VisitedBlocks;
  typedef std::unordered_set<Transition, TransitionHash> VisitedTransitions;

  enum HistoryKind { Blocks, Transitions };

  typedef std::unordered_map<
      llvm::BasicBlock *, std::unordered_map<llvm::BasicBlock *, VisitedBlocks>>
      BlocksHistory;
  typedef std::unordered_map<
      llvm::BasicBlock *,
      std::unordered_map<llvm::BasicBlock *, VisitedTransitions>>
      TransitionsHistory;

public:
  TargetCalculator(const KModule &module, CodeGraphDistance &codeGraphDistance)
      : module(module), codeGraphDistance(codeGraphDistance) {}

  void update(const ExecutionState &state);

  Target calculate(ExecutionState &state);

private:
  const KModule &module;
  CodeGraphDistance &codeGraphDistance;
  BlocksHistory blocksHistory;
  TransitionsHistory transitionsHistory;

  bool differenceIsEmpty(
      const ExecutionState &state,
      const std::unordered_map<llvm::BasicBlock *, VisitedBlocks> &history,
      KBlock *target);
  bool differenceIsEmpty(
      const ExecutionState &state,
      const std::unordered_map<llvm::BasicBlock *, VisitedTransitions> &history,
      KBlock *target);
};
} // namespace klee

#endif /* KLEE_TARGET_H */
