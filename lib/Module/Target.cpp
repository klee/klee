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

#include "klee/Module/CodeGraphDistance.h"
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

std::string Target::toString() const {
  std::ostringstream repr;
  repr << "Target " << getId() << ": ";
  if (shouldFailOnThisTarget()) {
    repr << "error in ";
  }
  repr << block->toString();
  if (atReturn()) {
    repr << " (at the end)";
  }
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

ref<Target> Target::create(const std::vector<ReachWithError> &_errors,
                           const std::string &_id, optional<ErrorLocation> _loc,
                           KBlock *_block) {
  Target *target = new Target(_errors, _id, _loc, _block);
  return createCachedTarget(target);
}

ref<Target> Target::create(KBlock *_block) {
  return create({ReachWithError::None}, "", nonstd::nullopt, _block);
}

bool Target::isTheSameAsIn(const KInstruction *instr) const {
  if (!loc.has_value()) {
    return false;
  }
  const auto &errLoc = *loc;
  return instr->info->line >= errLoc.startLine &&
         instr->info->line <= errLoc.endLine &&
         (!LocationAccuracy || !errLoc.startColumn.has_value() ||
          (instr->info->column >= *errLoc.startColumn &&
           instr->info->column <= *errLoc.endColumn));
}

bool Target::mustVisitForkBranches(KInstruction *instr) const {
  // null check after deref error is checked after fork
  // but reachability of this target from instr children
  // will always give false, so we need to force visiting
  // fork branches here
  return isTheSameAsIn(instr) &&
         isThatError(ReachWithError::NullCheckAfterDerefException);
}

int Target::compare(const Target &other) const {
  if (id != other.id) {
    return id < other.id ? -1 : 1;
  }
  if (block->parent->id != other.block->parent->id) {
    return block->parent->id < other.block->parent->id ? -1 : 1;
  }
  if (block->id != other.block->id) {
    return block->id < other.block->id ? -1 : 1;
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

bool Target::equals(const Target &b) const {
  return (toBeCleared || b.toBeCleared) || (isCached && b.isCached)
             ? this == &b
             : compare(b) == 0;
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
