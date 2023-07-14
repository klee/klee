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

#include "klee/Module/TargetHash.h"

#include "klee/ADT/RNG.h"
#include "klee/ADT/Ref.h"
#include "klee/Module/KModule.h"
#include "klee/Module/SarifReport.h"
#include "klee/Support/OptionCategories.h"
#include "klee/System/Time.h"

#include "klee/Support/CompilerWarning.h"
DISABLE_WARNING_PUSH
DISABLE_WARNING_DEPRECATED_DECLARATIONS
#include "llvm/Support/Casting.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"
DISABLE_WARNING_POP

#include <map>
#include <queue>
#include <set>
#include <unordered_set>
#include <vector>

namespace klee {
using nonstd::optional;

struct ErrorLocation {
  unsigned int startLine;
  unsigned int endLine;
  optional<unsigned int> startColumn;
  optional<unsigned int> endColumn;
};

struct Target {
private:
  KBlock *block;
  std::vector<ReachWithError>
      errors;                  // None - if it is not terminated in error trace
  std::string id;              // "" - if it is not terminated in error trace
  optional<ErrorLocation> loc; // TODO(): only for check in reportTruePositive

  explicit Target(const std::vector<ReachWithError> &_errors,
                  const std::string &_id, optional<ErrorLocation> _loc,
                  KBlock *_block)
      : block(_block), errors(_errors), id(_id), loc(_loc) {
    std::sort(errors.begin(), errors.end());
  }

  static ref<Target> createCachedTarget(ref<Target> target);

protected:
  friend class ref<Target>;
  friend class ref<const Target>;

  struct TargetHash {
    unsigned operator()(Target *const t) const { return t->hash(); }
  };

  struct TargetCmp {
    bool operator()(Target *const a, Target *const b) const {
      return a->equals(*b);
    }
  };

  typedef std::unordered_set<Target *, TargetHash, TargetCmp> CacheType;

  struct TargetCacheSet {
    CacheType cache;
    ~TargetCacheSet() {
      while (cache.size() != 0) {
        ref<Target> tmp = *cache.begin();
        tmp->isCached = false;
        cache.erase(cache.begin());
      }
    }
  };

  static TargetCacheSet cachedTargets;
  bool isCached = false;
  bool toBeCleared = false;

  /// @brief Required by klee::ref-managed objects
  mutable class ReferenceCounter _refCount;

public:
  bool isReported = false;

  static ref<Target> create(const std::vector<ReachWithError> &_errors,
                            const std::string &_id,
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

  const std::vector<ReachWithError> &getErrors() const { return errors; }
  bool isThatError(ReachWithError err) const {
    return std::find(errors.begin(), errors.end(), err) != errors.end();
  }
  bool shouldFailOnThisTarget() const {
    return std::find(errors.begin(), errors.end(), ReachWithError::None) ==
           errors.end();
  }

  bool isTheSameAsIn(const KInstruction *instr) const;

  /// returns true if we cannot use CFG reachability checks
  /// from instr children to this target
  /// to avoid solver calls
  bool mustVisitForkBranches(KInstruction *instr) const;

  std::string getId() const { return id; }

  std::string toString() const;
  ~Target();
};
} // namespace klee

#endif /* KLEE_TARGET_H */
