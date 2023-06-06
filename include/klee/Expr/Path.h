#ifndef KLEE_PATH_H
#define KLEE_PATH_H

#include <stack>
#include <string>
#include <vector>

namespace klee {
struct KBlock;
struct KFunction;
struct KInstruction;
class KModule;

// Callsite, called function
using stackframe_ty = std::pair<KInstruction *, KFunction *>;

class Path {
public:
  using path_ty = std::vector<KBlock *>;
  enum class TransitionKind { StepInto, StepOut, None };

  struct PathIndex {
    unsigned long block;
    unsigned long instruction;
  };

  struct PathIndexCompare {
    bool operator()(const PathIndex &a, const PathIndex &b) const {
      return a.block < b.block ||
             (a.block == b.block && a.instruction < b.instruction);
    }
  };

  struct BlockRange {
    unsigned long first;
    unsigned long last;
  };

  void advance(KInstruction *ki);

  friend bool operator==(const Path &lhs, const Path &rhs) {
    return lhs.KBlocks == rhs.KBlocks &&
           lhs.firstInstruction == rhs.firstInstruction &&
           lhs.lastInstruction == rhs.lastInstruction;
  }
  friend bool operator!=(const Path &lhs, const Path &rhs) {
    return !(lhs == rhs);
  }

  friend bool operator<(const Path &lhs, const Path &rhs) {
    return lhs.KBlocks < rhs.KBlocks ||
           (lhs.KBlocks == rhs.KBlocks &&
            (lhs.firstInstruction < rhs.firstInstruction ||
             (lhs.firstInstruction == rhs.firstInstruction &&
              lhs.lastInstruction < rhs.lastInstruction)));
  }

  unsigned KBlockSize() const;
  const path_ty &getBlocks() const;
  unsigned getFirstIndex() const;
  unsigned getLastIndex() const;

  PathIndex getCurrentIndex() const;

  std::vector<stackframe_ty> getStack(bool reversed) const;

  std::vector<std::pair<KFunction *, BlockRange>> asFunctionRanges() const;
  std::string toString() const;

  static Path concat(const Path &l, const Path &r);

  static Path parse(const std::string &str, const KModule &km);

  Path() = default;

  Path(unsigned firstInstruction, std::vector<KBlock *> kblocks,
       unsigned lastInstruction)
      : KBlocks(kblocks), firstInstruction(firstInstruction),
        lastInstruction(lastInstruction) {}

private:
  path_ty KBlocks;
  // Index of the first instruction in the first basic block
  unsigned firstInstruction = 0;
  // Index of the last (current) instruction in the current basic block
  unsigned lastInstruction = 0;

  static TransitionKind getTransitionKind(KBlock *a, KBlock *b);
};

}; // namespace klee

#endif
