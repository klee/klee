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

#include "klee/ADT/Ref.h"
#include "klee/Module/KModule.h"
#include "klee/Module/SarifReport.h"

#include <unordered_set>

namespace klee {

struct ErrorLocation {
  unsigned int startLine;
  unsigned int endLine;
  std::optional<unsigned int> startColumn;
  std::optional<unsigned int> endColumn;

  ErrorLocation(const klee::ref<klee::Location> &loc);
  ErrorLocation(const KInstruction *ki);
};

class ReproduceErrorTarget;

class Target {
private:
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

  using CacheType = std::unordered_set<Target *, TargetHash, TargetCmp>;

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

  KBlock *block;
  static TargetCacheSet cachedTargets;
  bool isCached = false;
  bool toBeCleared = false;

  /// @brief Required by klee::ref-managed objects
  mutable class ReferenceCounter _refCount;

  explicit Target(KBlock *_block) : block(_block) {}

  static ref<Target> createCachedTarget(ref<Target> target);

public:
  static const unsigned MAGIC_HASH_CONSTANT = 39;

  enum Kind {
    CoverBranch = 3,
    ReproduceError,
    ReachBlock,
  };

  virtual ~Target();
  virtual Kind getKind() const = 0;
  virtual int internalCompare(const Target &b) const = 0;
  static bool classof(const Target *) { return true; }
  virtual std::string toString() const = 0;

  int compare(const Target &other) const;

  bool equals(const Target &other) const;

  bool operator<(const Target &other) const;

  bool operator==(const Target &other) const;

  KBlock *getBlock() const { return block; }

  bool isNull() const { return block == nullptr; }

  explicit operator bool() const noexcept { return !isNull(); }

  unsigned hash() const { return reinterpret_cast<uintptr_t>(block); }

  bool shouldFailOnThisTarget() const {
    return isa<ReproduceErrorTarget>(this);
  }
};

class ReachBlockTarget : public Target {
protected:
  bool atEnd;

  explicit ReachBlockTarget(KBlock *_block, bool _atEnd)
      : Target(_block), atEnd(_atEnd) {}

public:
  static ref<Target> create(KBlock *_block, bool _atEnd);
  static ref<Target> create(KBlock *_block);

  Kind getKind() const override { return Kind::ReachBlock; }

  static bool classof(const Target *S) {
    return S->getKind() == Kind::ReachBlock;
  }
  static bool classof(const ReachBlockTarget *) { return true; }

  bool isAtEnd() const { return atEnd; }

  virtual int internalCompare(const Target &other) const override;
  virtual std::string toString() const override;
};

class CoverBranchTarget : public Target {
protected:
  unsigned branchCase;

  explicit CoverBranchTarget(KBlock *_block, unsigned _branchCase)
      : Target(_block), branchCase(_branchCase) {}

public:
  static ref<Target> create(KBlock *_block, unsigned _branchCase);

  Kind getKind() const override { return Kind::CoverBranch; }

  static bool classof(const Target *S) {
    return S->getKind() == Kind::CoverBranch;
  }
  static bool classof(const CoverBranchTarget *) { return true; }

  virtual int internalCompare(const Target &other) const override;
  virtual std::string toString() const override;

  unsigned getBranchCase() const { return branchCase; }
};

class ReproduceErrorTarget : public Target {
private:
  std::vector<ReachWithError>
      errors;        // None - if it is not terminated in error trace
  std::string id;    // "" - if it is not terminated in error trace
  ErrorLocation loc; // TODO(): only for check in reportTruePositive

protected:
  explicit ReproduceErrorTarget(const std::vector<ReachWithError> &_errors,
                                const std::string &_id, ErrorLocation _loc,
                                KBlock *_block)
      : Target(_block), errors(_errors), id(_id), loc(_loc) {
    assert(errors.size() > 0);
    std::sort(errors.begin(), errors.end());
  }

public:
  static ref<Target> create(const std::vector<ReachWithError> &_errors,
                            const std::string &_id, ErrorLocation _loc,
                            KBlock *_block);

  Kind getKind() const override { return Kind::ReproduceError; }

  static bool classof(const Target *S) {
    return S->getKind() == Kind::ReproduceError;
  }
  static bool classof(const ReproduceErrorTarget *) { return true; }

  virtual int internalCompare(const Target &other) const override;
  virtual std::string toString() const override;

  std::string getId() const { return id; }

  bool isReported = false;

  const std::vector<ReachWithError> &getErrors() const { return errors; }
  bool isThatError(ReachWithError err) const {
    return std::find(errors.begin(), errors.end(), err) != errors.end();
  }

  bool isTheSameAsIn(const KInstruction *instr) const;
};
} // namespace klee

#endif /* KLEE_TARGET_H */
