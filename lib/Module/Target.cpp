//===-- Target.cpp --------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Module/Target.h"
#include "klee/Module/TargetHash.h"

#include "klee/Module/CodeGraphInfo.h"
#include "klee/Module/KInstruction.h"

#include <set>
#include <sstream>
#include <vector>

using namespace llvm;
using namespace klee;

namespace klee {
llvm::cl::opt<bool> LocationAccuracy(
    "location-accuracy", cl::init(false),
    cl::desc("Check location with line and column accuracy (default=false)"));
}

ErrorLocation::ErrorLocation(const klee::ref<klee::Location> &loc)
    : startLine(loc->startLine), endLine(loc->endLine),
      startColumn(loc->startColumn), endColumn(loc->endColumn) {}

ErrorLocation::ErrorLocation(const KInstruction *ki) {
  startLine = (endLine = ki->getLine());
  startColumn = (endColumn = ki->getLine());
}

std::string ReproduceErrorTarget::toString() const {
  std::ostringstream repr;
  repr << "Target " << getId() << ": ";
  repr << "error in ";
  repr << block->toString();
  return repr.str();
}

std::string ReachBlockTarget::toString() const {
  std::ostringstream repr;
  repr << "Target: ";
  repr << block->toString();
  if (atEnd) {
    repr << " (at the end)";
  }
  return repr.str();
}

std::string CoverBranchTarget::toString() const {
  std::ostringstream repr;
  repr << "Target: ";
  repr << "cover " << branchCase << " branch at " << block->toString();
  return repr.str();
}

Target::TargetCacheSet Target::cachedTargets;

ref<Target> Target::createCachedTarget(ref<Target> target) {
  std::pair<CacheType::const_iterator, bool> success =
      cachedTargets.cache.insert(target.get());

  if (success.second) {
    // Cache miss
    target->isCached = true;
    return target;
  }
  // Cache hit
  return (ref<Target>)*(success.first);
}

ref<Target>
ReproduceErrorTarget::create(const std::vector<ReachWithError> &_errors,
                             const std::string &_id, ErrorLocation _loc,
                             KBlock *_block) {
  ReproduceErrorTarget *target =
      new ReproduceErrorTarget(_errors, _id, _loc, _block);
  return createCachedTarget(target);
}

ref<Target> ReachBlockTarget::create(KBlock *_block, bool _atEnd) {
  ReachBlockTarget *target = new ReachBlockTarget(_block, _atEnd);
  return createCachedTarget(target);
}

ref<Target> ReachBlockTarget::create(KBlock *_block) {
  ReachBlockTarget *target =
      new ReachBlockTarget(_block, isa<KReturnBlock>(_block));
  return createCachedTarget(target);
}

ref<Target> CoverBranchTarget::create(KBlock *_block, unsigned _branchCase) {
  CoverBranchTarget *target = new CoverBranchTarget(_block, _branchCase);
  return createCachedTarget(target);
}

bool ReproduceErrorTarget::isTheSameAsIn(const KInstruction *instr) const {
  const auto &errLoc = loc;
  return instr->getLine() >= errLoc.startLine &&
         instr->getLine() <= errLoc.endLine &&
         (!LocationAccuracy || !errLoc.startColumn.has_value() ||
          (instr->getColumn() >= *errLoc.startColumn &&
           instr->getColumn() <= *errLoc.endColumn));
}

int Target::compare(const Target &b) const { return internalCompare(b); }

bool Target::equals(const Target &b) const {
  return (toBeCleared || b.toBeCleared) || (isCached && b.isCached)
             ? this == &b
             : compare(b) == 0;
}

int ReachBlockTarget::internalCompare(const Target &b) const {
  if (getKind() != b.getKind()) {
    return getKind() < b.getKind() ? -1 : 1;
  }
  const ReachBlockTarget &other = static_cast<const ReachBlockTarget &>(b);

  if (block->parent->id != other.block->parent->id) {
    return block->parent->id < other.block->parent->id ? -1 : 1;
  }
  if (block->getId() != other.block->getId()) {
    return block->getId() < other.block->getId() ? -1 : 1;
  }

  if (atEnd != other.atEnd) {
    return !atEnd ? -1 : 1;
  }

  return 0;
}

int CoverBranchTarget::internalCompare(const Target &b) const {
  if (getKind() != b.getKind()) {
    return getKind() < b.getKind() ? -1 : 1;
  }
  const CoverBranchTarget &other = static_cast<const CoverBranchTarget &>(b);

  if (block->parent->id != other.block->parent->id) {
    return block->parent->id < other.block->parent->id ? -1 : 1;
  }
  if (block->getId() != other.block->getId()) {
    return block->getId() < other.block->getId() ? -1 : 1;
  }

  if (branchCase != other.branchCase) {
    return branchCase < other.branchCase ? -1 : 1;
  }

  return 0;
}

int ReproduceErrorTarget::internalCompare(const Target &b) const {
  if (getKind() != b.getKind()) {
    return getKind() < b.getKind() ? -1 : 1;
  }
  const ReproduceErrorTarget &other =
      static_cast<const ReproduceErrorTarget &>(b);

  if (id != other.id) {
    return id < other.id ? -1 : 1;
  }
  if (block->parent->id != other.block->parent->id) {
    return block->parent->id < other.block->parent->id ? -1 : 1;
  }
  if (block->getId() != other.block->getId()) {
    return block->getId() < other.block->getId() ? -1 : 1;
  }

  if (errors.size() != other.errors.size()) {
    return errors.size() < other.errors.size() ? -1 : 1;
  }

  for (unsigned i = 0; i < errors.size(); ++i) {
    if (errors.at(i) != other.errors.at(i)) {
      return errors.at(i) < other.errors.at(i) ? -1 : 1;
    }
  }

  return 0;
}

bool Target::operator<(const Target &other) const {
  return compare(other) == -1;
}

bool Target::operator==(const Target &other) const { return equals(other); }

Target::~Target() {
  if (isCached) {
    toBeCleared = true;
    cachedTargets.cache.erase(this);
  }
}
