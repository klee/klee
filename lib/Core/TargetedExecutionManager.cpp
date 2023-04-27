//===-- TargetedExecutionManager.cpp --------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "TargetedExecutionManager.h"

#include "ExecutionState.h"
#include "klee/Core/TerminationTypes.h"
#include "klee/Module/CodeGraphDistance.h"
#include "klee/Module/KInstruction.h"
#include "klee/Support/ErrorHandling.h"

#include <memory>

using namespace llvm;
using namespace klee;

namespace klee {
llvm::cl::opt<bool> CheckTraversability(
    "check-traversability", cl::init(false),
    cl::desc("Check error trace for traversability (default=false)"));

llvm::cl::opt<bool> SmartResolveEntryFunction(
    "smart-resolve-entry-function", cl::init(false),
    cl::desc("Resolve entry function using code flow graph instead of taking "
             "function of first location (default=false)"));
} // namespace klee

class LocatedEventManager {
  using FilenameCache = std::unordered_map<std::string, bool>;
  std::unordered_map<std::string, std::unique_ptr<FilenameCache>>
      filenameCacheMap;
  FilenameCache *filenameCache = nullptr;

public:
  LocatedEventManager() {}

  void prefetchFindFilename(const std::string &filename) {
    auto it = filenameCacheMap.find(filename);
    if (it != filenameCacheMap.end()) {
      filenameCache = it->second.get();
    } else {
      filenameCache = nullptr;
    }
  }

  bool isInside(Location &loc, const klee::FunctionInfo &fi) {
    bool isInside = false;
    if (filenameCache == nullptr) {
      isInside = loc.isInside(fi);
      auto filenameCachePtr = std::make_unique<FilenameCache>();
      filenameCache = filenameCachePtr.get();
      filenameCacheMap.insert(
          std::make_pair(fi.file, std::move(filenameCachePtr)));
      filenameCache->insert(std::make_pair(loc.filename, isInside));
    } else {
      auto it = filenameCache->find(loc.filename);
      if (it == filenameCache->end()) {
        isInside = loc.isInside(fi);
        filenameCache->insert(std::make_pair(loc.filename, isInside));
      } else {
        isInside = it->second;
      }
    }
    return isInside;
  }
};

TargetedExecutionManager::LocationToBlocks
TargetedExecutionManager::prepareAllLocations(KModule *kmodule,
                                              Locations &locations) const {
  LocationToBlocks locToBlocks;
  LocatedEventManager lem;
  const auto &infos = kmodule->infos;
  for (const auto &kfunc : kmodule->functions) {
    const auto &fi = infos->getFunctionInfo(*kfunc->function);
    lem.prefetchFindFilename(fi.file);
    if (kmodule->origInfos.count(fi.file) == 0) {
      continue;
    }
    const auto &origInstsInFile = kmodule->origInfos.at(fi.file);
    for (auto it = locations.begin(); it != locations.end();) {
      auto loc = *it;
      if (locToBlocks.count(loc) != 0) {
        ++it;
        continue;
      }

      if (!lem.isInside(*loc, fi)) {
        ++it;
        continue;
      }
      Blocks blocks = Blocks();
      for (const auto &kblock : kfunc->blocks) {
        auto b = kblock.get();
        if (!loc->isInside(b, origInstsInFile)) {
          continue;
        }
        blocks.insert(b);
      }

      if (blocks.size() > 0) {
        locToBlocks[loc] = blocks;
        it = locations.erase(it);
      } else {
        ++it;
      }
    }
  }

  return locToBlocks;
}

TargetedExecutionManager::Locations
TargetedExecutionManager::collectAllLocations(const SarifReport &paths) const {
  Locations locations;
  for (const auto &res : paths.results) {
    for (const auto &loc : res.locations) {
      locations.insert(loc);
    }
  }

  return locations;
}

bool TargetedExecutionManager::canReach(const ref<Location> &from,
                                        const ref<Location> &to,
                                        LocationToBlocks &locToBlocks) const {
  for (auto fromBlock : locToBlocks[from]) {
    for (auto toBlock : locToBlocks[to]) {
      auto fromKf = fromBlock->parent;
      auto toKf = toBlock->parent;
      if (fromKf == toKf) {
        if (fromBlock == toBlock) {
          return true;
        }

        const auto &blockDist = codeGraphDistance.getDistance(fromBlock);
        if (blockDist.count(toBlock) != 0) {
          return true;
        }
      } else {
        const auto &funcDist = codeGraphDistance.getDistance(fromKf);
        if (funcDist.count(toKf) != 0) {
          return true;
        }

        const auto &backwardFuncDist =
            codeGraphDistance.getBackwardDistance(fromKf);
        if (backwardFuncDist.count(toKf) != 0) {
          return true;
        }
      }
    }
  }

  return false;
}

bool TargetedExecutionManager::tryResolveLocations(
    Result &result, LocationToBlocks &locToBlocks) const {
  std::vector<ref<Location>> resolvedLocations;
  size_t index = 0;
  for (const auto &location : result.locations) {
    auto it = locToBlocks.find(location);
    if (it != locToBlocks.end()) {
      if (!resolvedLocations.empty() && CheckTraversability) {
        if (!canReach(resolvedLocations.back(), location, locToBlocks)) {
          klee_warning("Trace %u is untraversable! Can't reach location %s "
                       "from location %s, so skipping this trace.",
                       result.id, location->toString().c_str(),
                       resolvedLocations.back()->toString().c_str());
          return false;
        }
      }
      resolvedLocations.push_back(location);
    } else if (index == result.locations.size() - 1) {
      klee_warning(
          "Trace %u is malformed! %s at location %s, so skipping this trace.",
          result.id, getErrorsString(result.errors).c_str(),
          location->toString().c_str());
      return false;
    }
    ++index;
  }

  result.locations = std::move(resolvedLocations);

  return true;
}

KFunction *TargetedExecutionManager::tryResolveEntryFunction(
    const Result &result, LocationToBlocks &locToBlocks) const {
  assert(result.locations.size() > 0);

  auto resKf = (*locToBlocks[result.locations[0]].begin())->parent;
  if (SmartResolveEntryFunction) {
    for (size_t i = 1; i < result.locations.size(); ++i) {
      const auto &funcDist = codeGraphDistance.getDistance(resKf);
      auto curKf = (*locToBlocks[result.locations[i]].begin())->parent;
      if (funcDist.count(curKf) == 0) {
        const auto &curFuncDist = codeGraphDistance.getDistance(curKf);
        if (curFuncDist.count(resKf) == 0) {
          klee_warning("Trace %u is malformed! Can't resolve entry function, "
                       "so skipping this trace.",
                       result.id);
          return nullptr;
        } else {
          resKf = curKf;
        }
      }
    }
  }

  return resKf;
}

std::unordered_map<KFunction *, ref<TargetForest>>
TargetedExecutionManager::prepareTargets(KModule *kmodule, SarifReport paths) {
  Locations locations = collectAllLocations(paths);
  LocationToBlocks locToBlocks = prepareAllLocations(kmodule, locations);

  std::unordered_map<KFunction *, ref<TargetForest>> whitelists;

  for (auto &result : paths.results) {
    bool isFullyResolved = tryResolveLocations(result, locToBlocks);
    if (!isFullyResolved) {
      broken_traces.insert(result.id);
      continue;
    }

    auto kf = tryResolveEntryFunction(result, locToBlocks);
    if (!kf) {
      broken_traces.insert(result.id);
      continue;
    }

    if (whitelists.count(kf) == 0) {
      ref<TargetForest> whitelist = new TargetForest(kf);
      whitelists[kf] = whitelist;
    }
    whitelists[kf]->addTrace(result, locToBlocks);
  }

  return whitelists;
}

void TargetedExecutionManager::reportFalseNegative(ExecutionState &state,
                                                   ReachWithError error) {
  klee_warning("100.00%% %s False Negative at: %s", getErrorString(error),
               state.prevPC->getSourceLocation().c_str());
}

bool TargetedExecutionManager::reportTruePositive(ExecutionState &state,
                                                  ReachWithError error) {
  bool atLeastOneReported = false;
  for (auto kvp : state.targetForest) {
    auto target = kvp.first;
    if (!target->isThatError(error) || broken_traces.count(target->getId()) ||
        reported_traces.count(target->getId()))
      continue;

    /// The following code checks if target is a `call ...` instruction and we
    /// failed somewhere *inside* call
    auto possibleInstruction = state.prevPC;
    int i = state.stack.size() - 1;
    bool found = true;

    while (!target->isTheSameAsIn(
        possibleInstruction)) { // TODO: target->getBlock() ==
                                // possibleInstruction should also be checked,
                                // but more smartly
      if (i <= 0) {
        found = false;
        break;
      }
      possibleInstruction = state.stack[i].caller;
      i--;
    }
    if (!found)
      continue;

    state.error = error;
    atLeastOneReported = true;
    assert(!target->isReported);
    if (target->isThatError(ReachWithError::Reachable)) {
      klee_warning("100.00%% %s Reachable at trace %u", getErrorString(error),
                   target->getId());
    } else {
      klee_warning("100.00%% %s True Positive at trace %u",
                   getErrorString(error), target->getId());
    }
    target->isReported = true;
    reported_traces.insert(target->getId());
  }
  return atLeastOneReported;
}
