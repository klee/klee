#ifndef KLEE_ANNOTATION_H
#define KLEE_ANNOTATION_H

#include "map"
#include "set"
#include "string"
#include "vector"

#include "nlohmann/json.hpp"
#include "nonstd/optional.hpp"

using nonstd::nullopt;
using nonstd::optional;
using json = nlohmann::json;

namespace klee {

// Annotation format: https://github.com/UnitTestBot/klee/discussions/92
struct Annotation {
  enum class StatementKind {
    Unknown,

    Deref,
    InitNull,
  };

  enum class Property {
    Unknown,

    Determ,
    Noreturn,
  };

  struct StatementUnknown {
    explicit StatementUnknown(const std::string &str);
    virtual ~StatementUnknown();

    virtual Annotation::StatementKind getStatementName() const;
    virtual bool operator==(const StatementUnknown &other) const;

    std::string statementStr;
    std::vector<std::string> offset;

  protected:
    void parseOffset(const std::string &offsetStr);
    void parseOnlyOffset(const std::string &str);
  };

  struct StatementDeref final : public StatementUnknown {
    explicit StatementDeref(const std::string &str);

    Annotation::StatementKind getStatementName() const override;
  };

  struct StatementInitNull final : public StatementUnknown {
    explicit StatementInitNull(const std::string &str);

    Annotation::StatementKind getStatementName() const override;
  };

  using StatementPtr = std::shared_ptr<StatementUnknown>;
  using StatementPtrs = std::vector<StatementPtr>;

  bool operator==(const Annotation &other) const;

  std::string functionName;
  std::vector<StatementPtrs> statements;
  std::set<Property> properties;
};

using Annotations = std::map<std::string, Annotation>;

const std::map<std::string, Annotation::Property> toProperties{
    {"determ", Annotation::Property::Determ},
    {"noreturn", Annotation::Property::Noreturn},
};

Annotations parseAnnotationsFile(const json &annotationsJson);
Annotations parseAnnotationsFile(const std::string &path);

bool operator==(const Annotation::StatementPtr &first,
                const Annotation::StatementPtr &second);

} // namespace klee

#endif // KLEE_ANNOTATION_H
