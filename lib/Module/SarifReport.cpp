//===-- SarifReport.cpp----------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Module/SarifReport.h"

#include "klee/Module/KInstruction.h"
#include "klee/Module/KModule.h"
#include "klee/Support/ErrorHandling.h"

#include "llvm/IR/IntrinsicInst.h"

using namespace llvm;

namespace {
using namespace klee;

bool isOSSeparator(char c) { return c == '/' || c == '\\'; }

optional<ref<Location>>
tryConvertLocationJson(const LocationJson &locationJson) {
  const auto &physicalLocation = locationJson.physicalLocation;
  if (!physicalLocation.has_value()) {
    return nonstd::nullopt;
  }

  const auto &artifactLocation = physicalLocation->artifactLocation;
  if (!artifactLocation.has_value() || !artifactLocation->uri.has_value()) {
    return nonstd::nullopt;
  }

  const auto filename = std::move(*(artifactLocation->uri));

  const auto &region = physicalLocation->region;
  if (!region.has_value() || !region->startLine.has_value()) {
    return nonstd::nullopt;
  }

  return Location::create(std::move(filename), *(region->startLine),
                          region->endLine, region->startColumn,
                          region->endColumn);
}

std::unordered_set<ReachWithError>
tryConvertRuleJson(const std::string &ruleId, const std::string &toolName,
                   const optional<Message> &errorMessage) {
  if (toolName == "huawei") {
    if ("NullDereference" == ruleId) {
      return {ReachWithError::NullPointerException};
    } else if ("CheckAfterDeref" == ruleId) {
      return {ReachWithError::NullCheckAfterDerefException};
    } else if ("DoubleFree" == ruleId) {
      return {ReachWithError::DoubleFree};
    } else if ("UseAfterFree" == ruleId) {
      return {ReachWithError::UseAfterFree};
    } else if ("Reached" == ruleId) {
      return {ReachWithError::Reachable};
    } else {
      return {};
    }
  } else if (toolName == "clang") {
    if ("core.NullDereference" == ruleId) {
      return {ReachWithError::NullPointerException};
    } else if ("unix.Malloc" == ruleId) {
      if (errorMessage.has_value()) {
        if (errorMessage->text == "Attempt to free released memory") {
          return {ReachWithError::DoubleFree};
        } else if (errorMessage->text == "Use of memory after it is freed") {
          return {ReachWithError::UseAfterFree};
        } else {
          return {};
        }
      } else {
        return {ReachWithError::UseAfterFree, ReachWithError::DoubleFree};
      }
    } else {
      return {};
    }
  } else if (toolName == "CppCheck") {
    if ("nullPointer" == ruleId || "ctunullpointer" == ruleId) {
      return {ReachWithError::NullPointerException};
    } else if ("doubleFree" == ruleId) {
      return {ReachWithError::DoubleFree};
    } else {
      return {};
    }
  } else if (toolName == "Infer") {
    if ("NULL_DEREFERENCE" == ruleId || "NULLPTR_DEREFERENCE" == ruleId) {
      return {ReachWithError::NullPointerException};
    } else if ("USE_AFTER_DELETE" == ruleId || "USE_AFTER_FREE" == ruleId) {
      return {ReachWithError::UseAfterFree, ReachWithError::DoubleFree};
    } else {
      return {};
    }
  } else {
    return {};
  }
}

optional<Result> tryConvertResultJson(const ResultJson &resultJson,
                                      const std::string &toolName,
                                      unsigned id) {
  std::unordered_set<ReachWithError> errors = {ReachWithError::None};
  if (!resultJson.ruleId.has_value()) {
    errors = {ReachWithError::Reachable};
  } else {
    errors =
        tryConvertRuleJson(*resultJson.ruleId, toolName, resultJson.message);
    if (errors.size() == 0) {
      klee_warning("undefined error in %u result", id);
      return nonstd::nullopt;
    }
  }

  std::vector<ref<Location>> locations;
  std::vector<optional<json>> metadatas;

  if (resultJson.codeFlows.size() > 0) {
    assert(resultJson.codeFlows.size() == 1);
    assert(resultJson.codeFlows[0].threadFlows.size() == 1);

    const auto &threadFlow = resultJson.codeFlows[0].threadFlows[0];
    for (auto &threadFlowLocation : threadFlow.locations) {
      if (!threadFlowLocation.location.has_value()) {
        return nonstd::nullopt;
      }

      auto maybeLocation = tryConvertLocationJson(*threadFlowLocation.location);
      if (maybeLocation.has_value()) {
        locations.push_back(*maybeLocation);
        metadatas.push_back(std::move(threadFlowLocation.metadata));
      }
    }
  } else {
    assert(resultJson.locations.size() == 1);
    auto maybeLocation = tryConvertLocationJson(resultJson.locations[0]);
    if (maybeLocation.has_value()) {
      locations.push_back(*maybeLocation);
    }
  }

  if (locations.empty()) {
    return nonstd::nullopt;
  }

  return Result{std::move(locations), std::move(metadatas), id,
                std::move(errors)};
}
} // anonymous namespace

namespace klee {
const char *getErrorString(ReachWithError error) {
  return ReachWithErrorNames[error];
}

std::string getErrorsString(const std::unordered_set<ReachWithError> &errors) {
  if (errors.size() == 1) {
    return getErrorString(*errors.begin());
  }

  std::string res = "(";
  size_t index = 0;
  for (auto err : errors) {
    res += getErrorString(err);
    if (index != errors.size() - 1) {
      res += "|";
    }
    index++;
  }
  res += ")";
  return res;
}

SarifReport convertAndFilterSarifJson(const SarifReportJson &reportJson) {
  SarifReport report;

  if (reportJson.runs.size() == 0) {
    return report;
  }

  assert(reportJson.runs.size() == 1);

  const RunJson &run = reportJson.runs[0];

  unsigned id = 0;

  for (const auto &resultJson : run.results) {
    auto maybeResult =
        tryConvertResultJson(resultJson, run.tool.driver.name, ++id);

    if (maybeResult.has_value()) {
      report.results.push_back(*maybeResult);
    }
  }

  return report;
}

Location::EquivLocationHashSet Location::cachedLocations;
Location::LocationHashSet Location::locations;

ref<Location> Location::create(std::string filename_, unsigned int startLine_,
                               optional<unsigned int> endLine_,
                               optional<unsigned int> startColumn_,
                               optional<unsigned int> endColumn_) {
  Location *loc =
      new Location(filename_, startLine_, endLine_, startColumn_, endColumn_);
  std::pair<EquivLocationHashSet::const_iterator, bool> success =
      cachedLocations.insert(loc);
  if (success.second) {
    // Cache miss
    locations.insert(loc);
    return loc;
  }
  // Cache hit
  delete loc;
  loc = *(success.first);
  return loc;
}

Location::~Location() {
  if (locations.find(this) != locations.end()) {
    locations.erase(this);
    cachedLocations.erase(this);
  }
}

bool Location::isInside(const FunctionInfo &info) const {
  size_t suffixSize = 0;
  int m = info.file.size() - 1, n = filename.size() - 1;
  for (; m >= 0 && n >= 0 && info.file[m] == filename[n]; m--, n--) {
    suffixSize++;
    if (isOSSeparator(filename[n]))
      return true;
  }
  return suffixSize >= 3 && (n == -1 ? (m == -1 || isOSSeparator(info.file[m]))
                                     : (m == -1 && isOSSeparator(filename[n])));
}

bool Location::isInside(KBlock *block, const Instructions &origInsts) const {
  auto first = block->getFirstInstruction()->info;
  auto last = block->getLastInstruction()->info;
  if (!startColumn.has_value()) {
    if (first->line > endLine)
      return false;
    return startLine <= last->line; // and `first <= line` from above
  } else {
    for (size_t i = 0; i < block->numInstructions; ++i) {
      auto inst = block->instructions[i]->info;
      auto opCode = block->instructions[i]->inst->getOpcode();
      if (!isa<DbgInfoIntrinsic>(block->instructions[i]->inst) &&
          inst->line <= endLine && inst->line >= startLine &&
          inst->column <= *endColumn && inst->column >= *startColumn &&
          origInsts.at(inst->line).at(inst->column).count(opCode) != 0) {
        return true;
      }
    }

    return false;
  }
}

std::string Location::toString() const {
  return filename + ":" + std::to_string(startLine);
}
} // namespace klee
