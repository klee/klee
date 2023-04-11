//===-- TargetForest.cpp --------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Module/TargetForest.h"

#include "klee/Expr/Expr.h"
#include "klee/Module/KInstruction.h"
#include "klee/Module/KModule.h"
#include "klee/Module/TargetHash.h"

using namespace klee;
using namespace llvm;

void TargetForest::Layer::unionWith(TargetForest::Layer *other) {
  if (other->forest.empty())
    return;
  for (const auto &kv : other->forest) {
    auto it = forest.find(kv.first);
    if (it == forest.end()) {
      forest.insert(std::make_pair(kv.first, kv.second));
      continue;
    }
    auto layer = new Layer(it->second);
    layer->unionWith(kv.second.get());
    it->second = layer;
  }
}

void TargetForest::Layer::block(ref<Target> target) {
  if (empty())
    return;
  auto res = forest.find(target);
  if (res != forest.end()) {
    forest.erase(res);
  }
  for (InternalLayer::iterator itf = forest.begin(), eitf = forest.end();
       itf != eitf;) {
    ref<Layer> layer = itf->second->blockLeaf(target);
    itf->second = layer;
    if (itf->second->empty()) {
      itf = forest.erase(itf);
      eitf = forest.end();
    } else {
      ++itf;
    }
  }
}

bool TargetForest::Layer::deepFind(ref<Target> target) const {
  if (empty())
    return false;
  auto res = forest.find(target);
  if (res != forest.end()) {
    return true;
  }
  for (auto &f : forest) {
    if (f.second->deepFind(target))
      return true;
  }
  return false;
}

bool TargetForest::Layer::deepFindIn(ref<Target> child,
                                     ref<Target> target) const {
  auto res = forest.find(child);
  if (res == forest.end()) {
    return false;
  }
  if (child == target) {
    return true;
  }
  return res->second->deepFind(target);
}

TargetForest::Layer *TargetForest::Layer::removeChild(ref<Target> child) const {
  auto result = new Layer(this);
  result->forest.erase(child);
  return result;
}

TargetForest::Layer *TargetForest::Layer::addChild(ref<Target> child) const {
  auto result = new Layer(this);
  result->forest.insert({child, new Layer()});
  return result;
}

TargetForest::Layer *
TargetForest::Layer::blockLeafInChild(ref<Target> child,
                                      ref<Target> leaf) const {
  auto subtree = forest.find(child);
  assert(subtree != forest.end());
  if (subtree->second->forest.empty()) {
    if (subtree->first != leaf) {
      return new Layer(this);
    } else {
      return removeChild(child);
    }
  }

  ref<Layer> sublayer = new Layer(subtree->second);
  sublayer->block(leaf);
  if (sublayer->empty()) {
    return removeChild(child);
  } else {
    InternalLayer subforest;
    subforest[child] = sublayer;
    sublayer = new Layer(subforest);
    auto result = replaceChildWith(child, sublayer.get());
    return result;
  }
}

TargetForest::Layer *TargetForest::Layer::blockLeaf(ref<Target> leaf) const {
  auto result = new Layer(this);
  if (forest.empty()) {
    return result;
  }

  for (auto &layer : forest) {
    result = ref<Layer>(result)->blockLeafInChild(layer.first, leaf);
  }
  return result;
}

TargetForest::Layer *
TargetForest::Layer::replaceChildWith(ref<Target> const child,
                                      TargetForest::Layer *other) const {
  auto result = removeChild(child);
  result->unionWith(other);
  return result;
}

void TargetForest::Layer::dump(unsigned n) const {
  llvm::errs() << "THE " << n << " LAYER:\n";
  for (const auto &kv : forest) {
    llvm::errs() << kv.first->toString() << "\n";
  }
  llvm::errs() << "-----------------------\n";
  if (!forest.empty()) {
    for (const auto &kv : forest) {
      kv.second->dump(n + 1);
    }
    llvm::errs() << "++++++++++++++++++++++\n";
  }
}

TargetForest::History::EquivTargetsHistoryHashSet
    TargetForest::History::cachedHistories;
TargetForest::History::TargetsHistoryHashSet TargetForest::History::histories;

ref<TargetForest::History>
TargetForest::History::create(ref<Target> _target,
                              ref<History> _visitedTargets) {
  History *history = new History(_target, _visitedTargets);

  std::pair<EquivTargetsHistoryHashSet::const_iterator, bool> success =
      cachedHistories.insert(history);
  if (success.second) {
    // Cache miss
    histories.insert(history);
    return history;
  }
  // Cache hit
  delete history;
  history = *(success.first);
  return history;
}

ref<TargetForest::History> TargetForest::History::create(ref<Target> _target) {
  return create(_target, nullptr);
}

ref<TargetForest::History> TargetForest::History::create() {
  return create(nullptr);
}

int TargetForest::History::compare(const History &h) const {
  if (this == &h)
    return 0;

  if (target && h.target) {
    if (target != h.target)
      return (target < h.target) ? -1 : 1;
  } else {
    return h.target ? -1 : (target ? 1 : 0);
  }

  if (visitedTargets && h.visitedTargets) {
    if (h.visitedTargets != h.visitedTargets)
      return (visitedTargets < h.visitedTargets) ? -1 : 1;
  } else {
    return h.visitedTargets ? -1 : (visitedTargets ? 1 : 0);
  }

  return 0;
}

bool TargetForest::History::equals(const History &h) const {
  return compare(h) == 0;
}

void TargetForest::History::dump() const {
  if (target) {
    llvm::errs() << target->toString() << "\n";
  } else {
    llvm::errs() << "end.\n";
    assert(!visitedTargets);
    return;
  }
  if (visitedTargets)
    visitedTargets->dump();
}

TargetForest::History::~History() {
  if (histories.find(this) != histories.end()) {
    histories.erase(this);
    cachedHistories.erase(this);
  }
}

void TargetForest::stepTo(ref<Target> loc) {
  if (forest->empty())
    return;
  auto res = forest->find(loc);
  if (res == forest->end()) {
    return;
  }
  if (loc->shouldFailOnThisTarget()) {
    forest = new Layer();
  } else {
    history = history->add(loc);
    forest = forest->replaceChildWith(loc, res->second.get());
    loc->isReported = true;
  }
  if (forest->empty() && !loc->shouldFailOnThisTarget()) {
    history = History::create();
  }
}

void TargetForest::add(ref<Target> target) {
  auto res = forest->find(target);
  if (res != forest->end()) {
    return;
  }
  forest = forest->addChild(target);
}

void TargetForest::remove(ref<Target> target) {
  auto res = forest->find(target);
  if (res == forest->end()) {
    return;
  }
  forest = forest->removeChild(target);
}

void TargetForest::blockIn(ref<Target> subtarget, ref<Target> target) {
  if (!forest->deepFindIn(subtarget, target)) {
    return;
  }
  forest = forest->blockLeafInChild(subtarget, target);
}

void TargetForest::dump() const {
  llvm::errs() << "History:\n";
  history->dump();
  llvm::errs() << "Forest:\n";
  forest->dump(1);
}

ref<TargetForest> TargetForest::deepCopy() {
  return new TargetForest(forest->deepCopy(), entryFunction);
}

ref<TargetForest::Layer> TargetForest::Layer::deepCopy() {
  auto copyForest = new TargetForest::Layer(this);
  for (auto &targetAndForest : forest) {
    auto target = targetAndForest.first;
    auto layer = targetAndForest.second;
    copyForest->forest[target] = layer->deepCopy();
  }
  return copyForest;
}
