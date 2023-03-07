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

std::string Target::toString() const {
  std::ostringstream repr;
  repr << "Target " << getId() << ": ";
  if (shouldFailOnThisTarget()) {
    repr << "error in ";
  }
  repr << block->getAssemblyLocation();
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

ref<Target> Target::create(KBlock *block, ReachWithError error, unsigned id) {
  Target *target = new Target(block, error, id);
  return getFromCacheOrReturn(target);
}

ref<Target> Target::create(KBlock *block) {
  Target *target = new Target(block);
  return getFromCacheOrReturn(target);
}

int Target::compare(const Target &other) const {
  if (block != other.block) {
    return block < other.block ? -1 : 1;
  }
  if (error != other.error) {
    return error < other.error ? -1 : 1;
  }
  if (id != other.id) {
    return id < other.id ? -1 : 1;
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
