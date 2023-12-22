//===-- SarifReport.h --------------------------------------------*- C++-*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_SARIF_REPORT_H
#define KLEE_SARIF_REPORT_H

#include <optional>
#include <set>
#include <string>
#include <unordered_set>
#include <vector>

#include "klee/ADT/Ref.h"
#include "nlohmann/json.hpp"
#include "llvm/IR/Function.h"

using json = nlohmann::json;

namespace nlohmann {
template <typename T> struct adl_serializer<std::optional<T>> {
  static void to_json(json &j, const std::optional<T> &opt) {
    if (opt == std::nullopt) {
      j = nullptr;
    } else {
      j = *opt;
    }
  }

  static void from_json(const json &j, std::optional<T> &opt) {
    if (j.is_null()) {
      opt = std::nullopt;
    } else {
      opt = j.get<T>();
    }
  }
};
} // namespace nlohmann

namespace klee {
enum ReachWithError {
  DoubleFree = 0,
  UseAfterFree,
  MayBeNullPointerException,  // void f(int *x) { *x = 42; } - should it error?
  MustBeNullPointerException, // MayBeNPE = yes, MustBeNPE = no
  NullCheckAfterDerefException,
  Reachable,
  None,
};

const char *getErrorString(ReachWithError error);
std::string getErrorsString(const std::vector<ReachWithError> &errors);

struct FunctionInfo;
struct KBlock;
struct LocationInfo;

struct Message {
  std::string text;
};

struct ArtifactLocationJson {
  std::optional<std::string> uri;
};

struct RegionJson {
  std::optional<unsigned int> startLine;
  std::optional<unsigned int> endLine;
  std::optional<unsigned int> startColumn;
  std::optional<unsigned int> endColumn;
};

struct PhysicalLocationJson {
  std::optional<ArtifactLocationJson> artifactLocation;
  std::optional<RegionJson> region;
};

struct LocationJson {
  std::optional<Message> message;
  std::optional<PhysicalLocationJson> physicalLocation;
};

struct ThreadFlowLocationJson {
  std::optional<LocationJson> location;
  std::optional<json> metadata;
};

struct ThreadFlowJson {
  std::vector<ThreadFlowLocationJson> locations;
};

struct CodeFlowJson {
  std::vector<ThreadFlowJson> threadFlows;
};

struct Fingerprints {
  std::string cooddy_uid;
};

static void to_json(json &j, const Fingerprints &p) {
  j = json{{"cooddy.uid", p.cooddy_uid}};
}

static void from_json(const json &j, Fingerprints &p) {
  j.at("cooddy.uid").get_to(p.cooddy_uid);
}

struct ResultJson {
  std::optional<std::string> ruleId;
  std::optional<std::string> level;
  std::optional<Message> message;
  std::optional<unsigned> id;
  std::optional<Fingerprints> fingerprints;
  std::vector<LocationJson> locations;
  std::vector<CodeFlowJson> codeFlows;
};

struct RuleJson {
  std::string id;
  std::optional<std::string> name;
  std::optional<Message> shortDescription;
  std::optional<std::string> helpUri;
};

struct DriverJson {
  std::string name;
  std::optional<std::string> informationUri;
  std::vector<RuleJson> rules;
};

struct ToolJson {
  DriverJson driver;
};

struct RunJson {
  std::vector<ResultJson> results;
  ToolJson tool;
};

struct SarifReportJson {
  std::string version;
  std::vector<RunJson> runs;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(RuleJson, id, name,
                                                shortDescription, helpUri)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ArtifactLocationJson, uri)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(RegionJson, startLine, endLine,
                                                startColumn, endColumn)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PhysicalLocationJson,
                                                artifactLocation, region)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(LocationJson, message,
                                                physicalLocation)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ThreadFlowLocationJson,
                                                location, metadata)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ThreadFlowJson, locations)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(CodeFlowJson, threadFlows)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Message, text)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ResultJson, ruleId, level,
                                                message, id, fingerprints,
                                                codeFlows, locations)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(DriverJson, name,
                                                informationUri, rules)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ToolJson, driver)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(RunJson, results, tool)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SarifReportJson, version, runs)

struct Location {
  struct LocationHash {
    std::size_t operator()(const Location *l) const { return l->hash(); }
  };

  struct LocationCmp {
    bool operator()(const Location *a, const Location *b) const {
      return a == b;
    }
  };

  struct EquivLocationCmp {
    bool operator()(const Location *a, const Location *b) const {
      if (a == NULL || b == NULL)
        return false;
      return *a == *b;
    }
  };
  std::string filename;
  unsigned int startLine;
  unsigned int endLine;
  std::optional<unsigned int> startColumn;
  std::optional<unsigned int> endColumn;

  static ref<Location> create(std::string filename_, unsigned int startLine_,
                              std::optional<unsigned int> endLine_,
                              std::optional<unsigned int> startColumn_,
                              std::optional<unsigned int> endColumn_);

  ~Location();
  std::size_t hash() const { return hashValue; }

  /// @brief Required by klee::ref-managed objects
  class ReferenceCounter _refCount;

  bool operator==(const Location &other) const {
    return filename == other.filename && startLine == other.startLine &&
           endLine == other.endLine && startColumn == other.startColumn &&
           endColumn == other.endColumn;
  }

  bool isInside(const std::string &name) const;

  using Instructions = std::unordered_map<
      unsigned int,
      std::unordered_map<unsigned int, std::unordered_set<unsigned int>>>;

  bool isInside(KBlock *block, const Instructions &origInsts) const;

  bool isInside(const llvm::Function *f, const Instructions &origInsts) const;

  std::string toString() const;

private:
  typedef std::unordered_set<Location *, LocationHash, EquivLocationCmp>
      EquivLocationHashSet;
  typedef std::unordered_set<Location *, LocationHash, LocationCmp>
      LocationHashSet;

  static EquivLocationHashSet cachedLocations;
  static LocationHashSet locations;

  size_t hashValue = 0;
  void computeHash() {
    hash_combine(hashValue, filename);
    hash_combine(hashValue, startLine);
    hash_combine(hashValue, endLine);
    hash_combine(hashValue, startColumn);
    hash_combine(hashValue, endColumn);
  }

  template <class T> inline void hash_combine(std::size_t &s, const T &v) {
    std::hash<T> h;
    s ^= h(v) + 0x9e3779b9 + (s << 6) + (s >> 2);
  }

  Location(std::string filename_, unsigned int startLine_,
           std::optional<unsigned int> endLine_,
           std::optional<unsigned int> startColumn_,
           std::optional<unsigned int> endColumn_)
      : filename(filename_), startLine(startLine_),
        endLine(endLine_.has_value() ? *endLine_ : startLine_),
        startColumn(startColumn_),
        endColumn(endColumn_.has_value() ? endColumn_ : startColumn_) {
    computeHash();
  }
};

struct RefLocationHash {
  unsigned operator()(const ref<Location> &t) const { return t->hash(); }
};

struct RefLocationCmp {
  bool operator()(const ref<Location> &a, const ref<Location> &b) const {
    return a.get() == b.get();
  }
};

struct Result {
  std::vector<ref<Location>> locations;
  std::vector<std::optional<json>> metadatas;
  std::string id;
  std::vector<ReachWithError> errors;
};

struct SarifReport {
  std::vector<Result> results;

  bool empty() const { return results.empty(); }
};

SarifReport convertAndFilterSarifJson(const SarifReportJson &reportJson);

} // namespace klee

#endif /* KLEE_SARIF_REPORT_H */
