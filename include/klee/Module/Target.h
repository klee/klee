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

enum ReachWithError {
  DoubleFree = 0,
  UseAfterFree,
  NullPointerException,
  NullCheckAfterDerefException,
  Reachable,
  None,
};

static const char *ReachWithErrorNames[] = {
    "DoubleFree",
    "UseAfterFree",
    "NullPointerException",
    "NullCheckAfterDerefException",
    "Reachable",
    "None",
};

struct Target {
private:
  typedef std::unordered_set<Target *, TargetHash, EquivTargetCmp>
      EquivTargetHashSet;
  typedef std::unordered_set<Target *, TargetHash, TargetCmp> TargetHashSet;
  static EquivTargetHashSet cachedTargets;
  static TargetHashSet targets;
  KBlock *block;
  ReachWithError error;
  unsigned id;

  Target(KBlock *block, ReachWithError error, unsigned id)
      : block(block), error(error), id(id) {}

  Target(KBlock *block, ReachWithError error) : Target(block, error, 0) {}

  Target(KBlock *block) : Target(block, ReachWithError::None) {}

  static ref<Target> getFromCacheOrReturn(Target *target);

public:
  bool isReported = false;
  /// @brief Required by klee::ref-managed objects
  class ReferenceCounter _refCount;

  static ref<Target> create(KBlock *block, ReachWithError error, unsigned id);
  static ref<Target> create(KBlock *block);

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

  unsigned getId() const { return id; }

  ReachWithError getError() const { return error; }

  std::string toString() const;
  ~Target();
};
} // namespace klee

#endif /* KLEE_TARGET_H */
