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
#include "klee/Module/KInstruction.h"
#include "klee/Support/ErrorHandling.h"

#include <memory>

using namespace llvm;
using namespace klee;

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

bool TargetedExecutionManager::tryResolveLocations(
    Result &result, LocationToBlocks &locToBlocks) const {
  std::vector<ref<Location>> resolvedLocations;
  size_t index = 0;
  for (const auto &location : result.locations) {
    auto it = locToBlocks.find(location);
    if (it != locToBlocks.end()) {
      resolvedLocations.push_back(location);
    } else if (index == result.locations.size() - 1) {
      klee_warning(
          "Trace %u is malformed! %s at location %s, so skipping this trace.",
          result.id, getErrorString(result.error),
          location->toString().c_str());
      return false;
    }
    ++index;
  }

  result.locations = std::move(resolvedLocations);

  return true;
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
    } else {
      auto kf = (*locToBlocks[result.locations[0]].begin())->parent;
      if (whitelists.count(kf) == 0) {
        ref<TargetForest> whitelist = new TargetForest(kf);
        whitelists[kf] = whitelist;
      }
      whitelists[kf]->addTrace(result, locToBlocks);
    }
  }

  return whitelists;
}
