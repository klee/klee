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

#include "klee/ADT/RNG.h"
#include "klee/ADT/Ref.h"
#include "klee/Module/KModule.h"
#include "klee/Module/SarifReport.h"
#include "klee/System/Time.h"

#include "klee/Support/OptionCategories.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"

#include <map>
#include <queue>
#include <set>
#include <unordered_set>
#include <vector>

namespace klee {
struct EquivTargetCmp;
struct TargetHash;
struct TargetCmp;

using nonstd::optional;

struct ErrorLocation {
  unsigned int startLine;
  unsigned int endLine;
  optional<unsigned int> startColumn;
  optional<unsigned int> endColumn;
};

struct Target {
private:
  typedef std::unordered_set<Target *, TargetHash, EquivTargetCmp>
      EquivTargetHashSet;
  typedef std::unordered_set<Target *, TargetHash, TargetCmp> TargetHashSet;
  static EquivTargetHashSet cachedTargets;
  static TargetHashSet targets;
  KBlock *block;
  ReachWithError error;        // None - if it is not terminated in error trace
  unsigned id;                 // 0 - if it is not terminated in error trace
  optional<ErrorLocation> loc; // TODO(): only for check in reportTruePositive

  explicit Target(ReachWithError _error, unsigned _id,
                  optional<ErrorLocation> _loc, KBlock *_block)
      : block(_block), error(_error), id(_id), loc(_loc) {}

  static ref<Target> getFromCacheOrReturn(Target *target);

public:
  bool isReported = false;
  /// @brief Required by klee::ref-managed objects
  class ReferenceCounter _refCount;

  static ref<Target> create(ReachWithError _error, unsigned _id,
                            optional<ErrorLocation> _loc, KBlock *_block);
  static ref<Target> create(KBlock *_block);

  int compare(const Target &other) const;

  bool equals(const Target &other) const;

  bool operator<(const Target &other) const;

  bool operator==(const Target &other) const;

  bool atReturn() const { return isa<KReturnBlock>(block); }

  KBlock *getBlock() const { return block; }

  bool isNull() const { return block == nullptr; }

  explicit operator bool() const noexcept { return !isNull(); }

  unsigned hash() const { return reinterpret_cast<uintptr_t>(block); }

  bool shouldFailOnThisTarget() const { return error != ReachWithError::None; }

  bool isTheSameAsIn(KInstruction *instr) const;

  unsigned getId() const { return id; }

  ReachWithError getError() const { return error; }

  std::string toString() const;
  ~Target();
};
} // namespace klee

#endif /* KLEE_TARGET_H */
