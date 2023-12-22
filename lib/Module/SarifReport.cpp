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
#include "klee/Module/LocationInfo.h"
#include "klee/Support/ErrorHandling.h"

#include "klee/Support/CompilerWarning.h"
DISABLE_WARNING_PUSH
DISABLE_WARNING_DEPRECATED_DECLARATIONS
#include "llvm/IR/IntrinsicInst.h"
DISABLE_WARNING_POP

using namespace llvm;
using namespace klee;

namespace {
bool isOSSeparator(char c) { return c == '/' || c == '\\'; }

std::optional<ref<Location>>
tryConvertLocationJson(const LocationJson &locationJson) {
  const auto &physicalLocation = locationJson.physicalLocation;
  if (!physicalLocation.has_value()) {
    return std::nullopt;
  }

  const auto &artifactLocation = physicalLocation->artifactLocation;
  if (!artifactLocation.has_value() || !artifactLocation->uri.has_value()) {
    return std::nullopt;
  }

  const auto filename = std::move(*(artifactLocation->uri));

  const auto &region = physicalLocation->region;
  if (!region.has_value() || !region->startLine.has_value()) {
    return std::nullopt;
  }

  return Location::create(std::move(filename), *(region->startLine),
                          region->endLine, region->startColumn,
                          region->endColumn);
}

std::vector<ReachWithError>
tryConvertRuleJson(const std::string &ruleId, const std::string &toolName,
                   const std::optional<Message> &errorMessage) {
  if (toolName == "SecB") {
    if ("NullDereference" == ruleId) {
      return {ReachWithError::MustBeNullPointerException};
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
      return {ReachWithError::MayBeNullPointerException,
              ReachWithError::MustBeNullPointerException};
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
    } else if ("core.Reach" == ruleId) {
      return {ReachWithError::Reachable};
    } else {
      return {};
    }
  } else if (toolName == "CppCheck") {
    if ("nullPointer" == ruleId || "ctunullpointer" == ruleId) {
      return {ReachWithError::MayBeNullPointerException,
              ReachWithError::MustBeNullPointerException}; // TODO: check it out
    } else if ("doubleFree" == ruleId) {
      return {ReachWithError::DoubleFree};
    } else {
      return {};
    }
  } else if (toolName == "Infer") {
    if ("NULL_DEREFERENCE" == ruleId || "NULLPTR_DEREFERENCE" == ruleId) {
      return {ReachWithError::MayBeNullPointerException,
              ReachWithError::MustBeNullPointerException}; // TODO: check it out
    } else if ("USE_AFTER_DELETE" == ruleId || "USE_AFTER_FREE" == ruleId) {
      return {ReachWithError::UseAfterFree, ReachWithError::DoubleFree};
    } else {
      return {};
    }
  } else if (toolName == "Cooddy") {
    if ("NULL.DEREF" == ruleId || "NULL.UNTRUSTED.DEREF" == ruleId) {
      return {ReachWithError::MayBeNullPointerException,
              ReachWithError::MustBeNullPointerException};
    } else if ("MEM.DOUBLE.FREE" == ruleId) {
      return {ReachWithError::DoubleFree};
    } else if ("MEM.USE.FREE" == ruleId) {
      return {ReachWithError::UseAfterFree};
    } else {
      return {};
    }
  } else {
    return {};
  }
}

std::optional<Result> tryConvertResultJson(const ResultJson &resultJson,
                                           const std::string &toolName,
                                           const std::string &id) {
  std::vector<ReachWithError> errors = {};
  if (!resultJson.ruleId.has_value()) {
    errors = {ReachWithError::Reachable};
  } else {
    errors =
        tryConvertRuleJson(*resultJson.ruleId, toolName, resultJson.message);
    if (errors.size() == 0) {
      klee_warning("undefined error in %s result", id.c_str());
      return std::nullopt;
    }
  }

  std::vector<ref<Location>> locations;
  std::vector<std::optional<json>> metadatas;

  if (resultJson.codeFlows.size() > 0) {
    assert(resultJson.codeFlows.size() == 1);
    assert(resultJson.codeFlows[0].threadFlows.size() == 1);

    const auto &threadFlow = resultJson.codeFlows[0].threadFlows[0];
    for (auto &threadFlowLocation : threadFlow.locations) {
      if (!threadFlowLocation.location.has_value()) {
        return std::nullopt;
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
    return std::nullopt;
  }

  return Result{std::move(locations), std::move(metadatas), id,
                std::move(errors)};
}
} // anonymous namespace

namespace klee {
static const char *ReachWithErrorNames[] = {
    "DoubleFree",
    "UseAfterFree",
    "MayBeNullPointerException",
    "NullPointerException", // for backward compatibility with SecB
    "NullCheckAfterDerefException",
    "Reachable",
    "None",
};

const char *getErrorString(ReachWithError error) {
  return ReachWithErrorNames[error];
}

std::string getErrorsString(const std::vector<ReachWithError> &errors) {
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

struct TraceId {
  virtual ~TraceId() {}
  virtual std::string toString() const = 0;
  virtual void getNextId(const klee::ResultJson &resultJson) = 0;
};

class CooddyTraceId : public TraceId {
  std::string uid = "";

public:
  std::string toString() const override { return uid; }
  void getNextId(const klee::ResultJson &resultJson) override {
    uid = resultJson.fingerprints.value().cooddy_uid;
  }
};

class GetterTraceId : public TraceId {
  unsigned id = 0;

public:
  std::string toString() const override { return std::to_string(id); }
  void getNextId(const klee::ResultJson &resultJson) override {
    id = resultJson.id.value();
  }
};

class NumericTraceId : public TraceId {
  unsigned id = 0;

public:
  std::string toString() const override { return std::to_string(id); }
  void getNextId(const klee::ResultJson &resultJson) override { id++; }
};

TraceId *createTraceId(const std::string &toolName,
                       const std::vector<klee::ResultJson> &results) {
  if (toolName == "Cooddy")
    return new CooddyTraceId();
  else if (results.size() > 0 && results[0].id.has_value())
    return new GetterTraceId();
  return new NumericTraceId();
}

void setResultId(const ResultJson &resultJson, bool useProperId, unsigned &id) {
  if (useProperId) {
    assert(resultJson.id.has_value() && "all results must have an proper id");
    id = resultJson.id.value();
  } else {
    ++id;
  }
}

SarifReport convertAndFilterSarifJson(const SarifReportJson &reportJson) {
  SarifReport report;

  if (reportJson.runs.size() == 0) {
    return report;
  }

  assert(reportJson.runs.size() == 1);

  const RunJson &run = reportJson.runs[0];
  const std::string toolName = run.tool.driver.name;

  TraceId *id = createTraceId(toolName, run.results);

  for (const auto &resultJson : run.results) {
    id->getNextId(resultJson);
    auto maybeResult =
        tryConvertResultJson(resultJson, toolName, id->toString());
    if (maybeResult.has_value()) {
      report.results.push_back(*maybeResult);
    }
  }
  delete id;

  return report;
}

Location::EquivLocationHashSet Location::cachedLocations;
Location::LocationHashSet Location::locations;

ref<Location> Location::create(std::string filename_, unsigned int startLine_,
                               std::optional<unsigned int> endLine_,
                               std::optional<unsigned int> startColumn_,
                               std::optional<unsigned int> endColumn_) {
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

bool Location::isInside(const std::string &name) const {
  size_t suffixSize = 0;
  int m = name.size() - 1, n = filename.size() - 1;
  for (; m >= 0 && n >= 0 && name[m] == filename[n]; m--, n--) {
    suffixSize++;
    if (isOSSeparator(filename[n]))
      return true;
  }
  return suffixSize >= 3 && (n == -1 ? (m == -1 || isOSSeparator(name[m]))
                                     : (m == -1 && isOSSeparator(filename[n])));
}

bool Location::isInside(KBlock *block, const Instructions &origInsts) const {
  auto first = block->getFirstInstruction();
  auto last = block->getLastInstruction();
  if (!startColumn.has_value()) {
    if (first->getLine() > endLine)
      return false;
    return startLine <= last->getLine(); // and `first <= line` from above
  } else {
    for (unsigned i = 0, ie = block->getNumInstructions(); i < ie; ++i) {
      auto inst = block->instructions[i];
      auto opCode = block->instructions[i]->inst()->getOpcode();
      if (!isa<DbgInfoIntrinsic>(block->instructions[i]->inst()) &&
          inst->getLine() <= endLine && inst->getLine() >= startLine &&
          inst->getColumn() <= *endColumn &&
          inst->getColumn() >= *startColumn &&
          origInsts.at(inst->getLine()).at(inst->getColumn()).count(opCode) !=
              0) {
        return true;
      }
    }

    return false;
  }
}

bool Location::isInside(const llvm::Function *f,
                        const Instructions &origInsts) const {
  if (f->empty()) {
    return false;
  }
  auto first = &f->front().front();
  auto last = &f->back().back();
  auto firstLoc = getLocationInfo(first);
  auto lastLoc = getLocationInfo(last);
  if (!startColumn.has_value()) {
    if (firstLoc.line > endLine) {
      return false;
    }
    return startLine <= lastLoc.line;
  }
  for (const auto &block : *f) {
    for (const auto &inst : block) {
      auto locInfo = getLocationInfo(&inst);
      if (!isa<DbgInfoIntrinsic>(&inst) && locInfo.line <= endLine &&
          locInfo.line >= startLine && locInfo.column <= *endColumn &&
          locInfo.column >= *startColumn &&
          origInsts.at(locInfo.line)
                  .at(locInfo.column.value_or(0))
                  .count(inst.getOpcode()) != 0) {
        return true;
      }
    }
  }
  return false;
}

std::string Location::toString() const {
  return filename + ":" + std::to_string(startLine);
}
} // namespace klee
