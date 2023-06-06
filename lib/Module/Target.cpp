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

Target::EquivTargetHashSet Target::cachedTargets;
Target::TargetHashSet Target::targets;

ref<Target> Target::getFromCacheOrReturn(Target *target) {
  std::pair<TargetHashSet::const_iterator, bool> success =
      cachedTargets.insert(target);
  if (success.second) {
    // Cache miss
    targets.insert(target);
    return target;
  }
  // Cache hit
  delete target;
  target = *(success.first);
  return target;
}

ref<Target> Target::create(const std::set<ReachWithError> &_errors,
                           unsigned _id, optional<ErrorLocation> _loc,
                           KBlock *_block) {
  Target *target = new Target(_errors, _id, _loc, _block);
  return getFromCacheOrReturn(target);
}

ref<Target> Target::create(KBlock *_block) {
  return create({ReachWithError::None}, 0, nonstd::nullopt, _block);
}

bool Target::isTheSameAsIn(KInstruction *instr) const {
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

int Target::compare(const Target &other) const {
  if (errors != other.errors) {
    return errors < other.errors ? -1 : 1;
  }
  if (id != other.id) {
    return id < other.id ? -1 : 1;
  }
  if (block != other.block) {
    return block < other.block ? -1 : 1;
  }
  return 0;
}

bool Target::equals(const Target &other) const { return compare(other) == 0; }

bool Target::operator<(const Target &other) const {
  return compare(other) == -1;
}

bool Target::operator==(const Target &other) const { return equals(other); }

Target::~Target() {
  if (targets.find(this) != targets.end()) {
    cachedTargets.erase(this);
    targets.erase(this);
  }
}
