//===-- TargetForest.cpp --------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Module/TargetForest.h"

#include "klee/Core/TargetedExecutionReporter.h"
#include "klee/Expr/Expr.h"
#include "klee/Module/KInstruction.h"
#include "klee/Module/KModule.h"
#include "klee/Module/TargetHash.h"

#include "klee/Support/ErrorHandling.h"

using namespace klee;
using namespace llvm;

TargetForest::UnorderedTargetsSet::EquivUnorderedTargetsSetHashSet
    TargetForest::UnorderedTargetsSet::cachedVectors;
TargetForest::UnorderedTargetsSet::UnorderedTargetsSetHashSet
    TargetForest::UnorderedTargetsSet::vectors;

TargetForest::UnorderedTargetsSet::UnorderedTargetsSet(
    const ref<Target> &target) {
  targetsVec.push_back(target);
  sortAndComputeHash();
}

TargetForest::UnorderedTargetsSet::UnorderedTargetsSet(
    const TargetsSet &targets) {
  for (const auto &p : targets) {
    targetsVec.push_back(p);
  }
  sortAndComputeHash();
}

void TargetForest::UnorderedTargetsSet::sortAndComputeHash() {
  std::sort(targetsVec.begin(), targetsVec.end(), RefTargetLess{});
  RefTargetHash hasher;
  std::size_t seed = targetsVec.size();
  for (const auto &r : targetsVec) {
    seed ^= hasher(r) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
  }
  hashValue = seed;
}

ref<TargetForest::UnorderedTargetsSet>
TargetForest::UnorderedTargetsSet::create(const ref<Target> &target) {
  UnorderedTargetsSet *vec = new UnorderedTargetsSet(target);
  return create(vec);
}

ref<TargetForest::UnorderedTargetsSet>
TargetForest::UnorderedTargetsSet::create(const TargetsSet &targets) {
  UnorderedTargetsSet *vec = new UnorderedTargetsSet(targets);
  return create(vec);
}

ref<TargetForest::UnorderedTargetsSet>
TargetForest::UnorderedTargetsSet::create(UnorderedTargetsSet *vec) {
  std::pair<EquivUnorderedTargetsSetHashSet::const_iterator, bool> success =
      cachedVectors.insert(vec);
  if (success.second) {
    // Cache miss
    vectors.insert(vec);
    return vec;
  }
  // Cache hit
  delete vec;
  vec = *(success.first);
  return vec;
}

TargetForest::UnorderedTargetsSet::~UnorderedTargetsSet() {
  if (vectors.find(this) != vectors.end()) {
    vectors.erase(this);
    cachedVectors.erase(this);
  }
}

void TargetForest::Layer::addTrace(
    const Result &result,
    const std::unordered_map<ref<Location>, std::unordered_set<KBlock *>,
                             RefLocationHash, RefLocationCmp> &locToBlocks) {
  auto forest = this;
  for (size_t i = 0; i < result.locations.size(); ++i) {
    const auto &loc = result.locations[i];
    auto it = locToBlocks.find(loc);
    assert(it != locToBlocks.end());
    TargetsSet targets;
    for (auto block : it->second) {
      ref<Target> target = nullptr;
      if (i == result.locations.size() - 1) {
        target = Target::create(result.errors, result.id,
                                ErrorLocation{loc->startLine, loc->endLine,
                                              loc->startColumn, loc->endColumn},
                                block);
      } else {
        target = Target::create(block);
      }
      targets.insert(target);
    }

    ref<UnorderedTargetsSet> targetsVec = UnorderedTargetsSet::create(targets);
    if (forest->forest.count(targetsVec) == 0) {
      ref<TargetForest::Layer> next = new TargetForest::Layer();
      forest->insert(targetsVec, next);
    }

    for (auto &target : targetsVec->getTargets()) {
      forest->insertTargetsToVec(target, targetsVec);
    }

    forest = forest->forest[targetsVec].get();
  }
}

void TargetForest::Layer::propagateConfidenceToChildren() {
  auto parentConfidence = getConfidence();
  for (auto &kv : forest) {
    kv.second->confidence = kv.second->getConfidence(parentConfidence);
  }
}

void TargetForest::Layer::unionWith(TargetForest::Layer *other) {
  if (other->forest.empty())
    return;
  other->propagateConfidenceToChildren();
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

  for (const auto &kv : other->targetsToVector) {
    auto it = targetsToVector.find(kv.first);
    if (it == targetsToVector.end()) {
      targetsToVector.insert(std::make_pair(kv.first, kv.second));
      continue;
    }
    it->second.insert(kv.second.begin(), kv.second.end());
  }
}

void TargetForest::Layer::block(ref<Target> target) {
  if (empty())
    return;
  removeTarget(target);
  for (InternalLayer::iterator itf = forest.begin(); itf != forest.end();) {
    ref<Layer> layer = itf->second->blockLeaf(target);
    itf->second = layer;
    if (itf->second->empty()) {
      for (auto &itfTarget : itf->first->getTargets()) {
        auto it = targetsToVector.find(itfTarget);
        if (it != targetsToVector.end()) {
          it->second.erase(itf->first);
          if (it->second.empty()) {
            targetsToVector.erase(it);
          }
        }
      }
      itf = forest.erase(itf);
    } else {
      ++itf;
    }
  }
}

void TargetForest::Layer::removeTarget(ref<Target> target) {
  auto it = targetsToVector.find(target);

  if (it == targetsToVector.end()) {
    return;
  }

  auto targetsVectors = std::move(it->second);

  targetsToVector.erase(it);

  for (auto &targetsVec : targetsVectors) {
    bool shouldDelete = true;

    for (auto &localTarget : targetsVec->getTargets()) {
      if (targetsToVector.count(localTarget) != 0) {
        shouldDelete = false;
      }
    }

    if (shouldDelete) {
      forest.erase(targetsVec);
    }
  }
}

bool TargetForest::Layer::deepFind(ref<Target> target) const {
  if (empty())
    return false;
  auto res = targetsToVector.find(target);
  if (res != targetsToVector.end()) {
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
  auto res = targetsToVector.find(child);
  if (res == targetsToVector.end()) {
    return false;
  }

  if (child == target) {
    return true;
  }
  for (auto &targetsVec : res->second) {
    auto it = forest.find(targetsVec);
    assert(it != forest.end());
    if (it->second->deepFind(target)) {
      return true;
    }
  }

  return false;
}

TargetForest::Layer *TargetForest::Layer::removeChild(ref<Target> child) const {
  auto result = new Layer(this);
  result->removeTarget(child);
  return result;
}

TargetForest::Layer *
TargetForest::Layer::removeChild(ref<UnorderedTargetsSet> child) const {
  auto result = new Layer(this);
  result->forest.erase(child);
  for (auto &target : child->getTargets()) {
    auto it = result->targetsToVector.find(target);
    if (it == result->targetsToVector.end()) {
      continue;
    }

    it->second.erase(child);
    if (it->second.empty()) {
      result->targetsToVector.erase(it);
    }
  }
  return result;
}

TargetForest::Layer *TargetForest::Layer::addChild(ref<Target> child) const {
  auto targetsVec = UnorderedTargetsSet::create(child);
  auto result = new Layer(this);
  if (forest.count(targetsVec) != 0) {
    return result;
  }
  result->forest.insert({targetsVec, new Layer()});

  result->targetsToVector[child].insert(targetsVec);
  return result;
}

TargetForest::Layer *
TargetForest::Layer::blockLeafInChild(ref<UnorderedTargetsSet> child,
                                      ref<Target> leaf) const {
  auto subtree = forest.find(child);
  assert(subtree != forest.end());
  if (subtree->second->forest.empty()) {
    auto targetsVectors = targetsToVector.find(leaf);
    if (targetsVectors == targetsToVector.end() ||
        targetsVectors->second.count(subtree->first) == 0) {
      return new Layer(this);
    } else {
      return removeChild(leaf);
    }
  }

  ref<Layer> sublayer = new Layer(subtree->second);
  sublayer->block(leaf);
  if (sublayer->empty()) {
    return removeChild(child);
  } else {
    InternalLayer subforest;
    subforest[child] = sublayer;
    TargetsToVector subTargetsToVector;
    for (auto &target : child->getTargets()) {
      subTargetsToVector[target].insert(child);
    }
    sublayer = new Layer(subforest, subTargetsToVector, confidence);
    auto result = replaceChildWith(child, sublayer.get());
    return result;
  }
}

TargetForest::Layer *
TargetForest::Layer::blockLeafInChild(ref<Target> child,
                                      ref<Target> leaf) const {
  TargetForest::Layer *res = nullptr;
  auto it = targetsToVector.find(child);
  if (it == targetsToVector.end()) {
    return res;
  }
  for (auto &targetsVec : it->second) {
    if (res) {
      res = res->blockLeafInChild(targetsVec, leaf);
    } else {
      res = blockLeafInChild(targetsVec, leaf);
    }
  }
  return res;
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

TargetForest::Layer *TargetForest::Layer::replaceChildWith(
    ref<TargetForest::UnorderedTargetsSet> const child,
    TargetForest::Layer *other) const {
  auto result = removeChild(child);
  result->unionWith(other);
  return result;
}

TargetForest::Layer *TargetForest::Layer::replaceChildWith(
    ref<Target> child,
    const std::unordered_set<ref<UnorderedTargetsSet>,
                             RefUnorderedTargetsSetHash,
                             RefUnorderedTargetsSetCmp> &other) const {
  std::vector<Layer *> layers;
  for (auto &targetsVec : other) {
    auto it = forest.find(targetsVec);
    assert(it != forest.end());
    layers.push_back(it->second.get());
  }
  auto result = new Layer(this);
  for (auto &targetsVec : other) {
    for (auto &target : targetsVec->getTargets()) {
      auto it = result->targetsToVector.find(target);
      if (it != result->targetsToVector.end()) {
        it->second.erase(targetsVec);
        if (it->second.empty()) {
          result->targetsToVector.erase(it);
        }
      }
    }
    result->forest.erase(targetsVec);
  }
  for (auto layer : layers) {
    result->unionWith(layer);
  }
  return result;
}

bool TargetForest::Layer::allNodesRefCountOne() const {
  bool all = true;
  for (const auto &it : forest) {
    all &= it.second->_refCount.getCount() == 1;
    assert(all);
    all &= it.second->allNodesRefCountOne();
  }
  return all;
}

void TargetForest::Layer::dump(unsigned n) const {
  llvm::errs() << "THE " << n << " LAYER:\n";
  llvm::errs() << "Confidence: " << confidence << "\n";
  for (const auto &kv : targetsToVector) {
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
    forest = forest->replaceChildWith(loc, res->second);
    loc->isReported = true;
  }
  if (forest->empty() && !loc->shouldFailOnThisTarget()) {
    history = History::create();
  }
}

void TargetForest::add(ref<Target> target) {
  if (forest->find(target) != forest->end()) {
    return;
  }
  forest = forest->addChild(target);
}

void TargetForest::remove(ref<Target> target) {
  if (forest->find(target) == forest->end()) {
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

void TargetForest::block(const ref<Target> &target) {
  if (!forest->deepFind(target)) {
    return;
  }

  forest = forest->blockLeaf(target);
}

void TargetForest::dump() const {
  llvm::errs() << "History:\n";
  history->dump();
  llvm::errs() << "Forest:\n";
  forest->dump(1);
}

bool TargetForest::allNodesRefCountOne() const {
  return forest->allNodesRefCountOne();
}

void TargetForest::Layer::addLeafs(
    std::vector<std::pair<ref<UnorderedTargetsSet>, confidence::ty>> &leafs,
    confidence::ty parentConfidence) const {
  for (const auto &targetAndForest : forest) {
    auto targetsVec = targetAndForest.first;
    auto layer = targetAndForest.second;
    auto confidence = layer->getConfidence(parentConfidence);
    if (layer->empty()) {
      leafs.push_back(std::make_pair(targetsVec, confidence));
    } else {
      layer->addLeafs(leafs, confidence);
    }
  }
}

std::vector<std::pair<ref<TargetForest::UnorderedTargetsSet>, confidence::ty>>
TargetForest::leafs() const {
  std::vector<std::pair<ref<UnorderedTargetsSet>, confidence::ty>> leafs;
  forest->addLeafs(leafs, forest->getConfidence());
  return leafs;
}

ref<TargetForest> TargetForest::deepCopy() {
  return new TargetForest(forest->deepCopy(), entryFunction);
}

ref<TargetForest::Layer> TargetForest::Layer::deepCopy() {
  auto copyForest = new TargetForest::Layer(this);
  for (auto &targetAndForest : forest) {
    auto targetsVec = targetAndForest.first;
    auto layer = targetAndForest.second;
    copyForest->forest[targetsVec] = layer->deepCopy();
  }
  return copyForest;
}

void TargetForest::Layer::divideConfidenceBy(unsigned factor) {
  for (auto &targetAndForest : forest) {
    auto layer = targetAndForest.second;
    layer->confidence /= factor;
  }
}

TargetForest::Layer *TargetForest::Layer::divideConfidenceBy(
    const TargetForest::TargetToStateSetMap &reachableStatesOfTarget) {
  if (forest.empty() || reachableStatesOfTarget.empty())
    return this;
  auto result = new Layer(this);
  for (auto &targetAndForest : forest) {
    auto targetsVec = targetAndForest.first;
    auto layer = targetAndForest.second;
    std::unordered_set<ExecutionState *> reachableStatesForTargetsVec;
    for (const auto &target : targetsVec->getTargets()) {
      auto it = reachableStatesOfTarget.find(target);
      if (it == reachableStatesOfTarget.end()) {
        continue;
      }

      for (auto state : it->second) {
        reachableStatesForTargetsVec.insert(state);
      }
    }

    size_t count = reachableStatesForTargetsVec.size();
    if (count) {
      if (count == 1)
        continue;
      auto next = new Layer(layer);
      result->forest[targetsVec] = next;
      next->confidence /= count;
    } else
      result->forest[targetsVec] =
          layer->divideConfidenceBy(reachableStatesOfTarget);
  }
  return result;
}

void TargetForest::Layer::collectHowManyEventsInTracesWereReached(
    std::unordered_map<std::string, std::pair<unsigned, unsigned>>
        &traceToEventCount,
    unsigned reached, unsigned total) const {
  total++;
  for (const auto &p : forest) {
    auto targetsVec = p.first;
    auto child = p.second;
    bool isReported = false;
    for (auto &target : targetsVec->getTargets()) {
      if (target->isReported) {
        isReported = true;
      }
    }
    auto reachedCurrent = isReported ? reached + 1 : reached;
    auto target = *targetsVec->getTargets().begin();
    if (target->shouldFailOnThisTarget()) {
      traceToEventCount[target->getId()] =
          std::make_pair(reachedCurrent, total);
    }
    child->collectHowManyEventsInTracesWereReached(traceToEventCount,
                                                   reachedCurrent, total);
  }
}
