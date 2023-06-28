#include "klee/Module/Annotation.h"

#include "klee/Support/ErrorHandling.h"

#include <fstream>

namespace {

using namespace klee;

Annotation::StatementPtr toStatement(const std::string &name,
                                     const std::string &str) {
  if (name == "Deref") {
    return std::make_shared<Annotation::StatementDeref>(str);
  } else if (name == "InitNull") {
    return std::make_shared<Annotation::StatementInitNull>(str);
  } else {
    return std::make_shared<Annotation::StatementUnknown>(str);
  }
}

} // namespace

namespace klee {

Annotation::StatementUnknown::StatementUnknown(const std::string &str)
    : statementStr(str) {
  parseOnlyOffset(str);
}

Annotation::StatementUnknown::~StatementUnknown() = default;

Annotation::StatementKind
Annotation::StatementUnknown::getStatementName() const {
  return Annotation::StatementKind::Unknown;
}

bool Annotation::StatementUnknown::operator==(
    const Annotation::StatementUnknown &other) const {
  return (statementStr == other.statementStr) && (offset == other.offset);
}

void Annotation::StatementUnknown::parseOffset(const std::string &offsetStr) {
  size_t pos = 0;
  while (pos < offsetStr.size()) {
    if (offsetStr[pos] == '*') {
      offset.emplace_back("*");
      pos++;
    } else if (offsetStr[pos] == '&') {
      offset.emplace_back("&");
      pos++;
    } else if (offsetStr[pos] == '[') {
      size_t posEndExpr = offsetStr.find(']', pos);
      if (posEndExpr == std::string::npos) {
        klee_error("Incorrect annotation offset format.");
      }
      offset.push_back(offsetStr.substr(pos + 1, posEndExpr - 1 - pos));
      pos = posEndExpr + 1;
    } else {
      klee_error("Incorrect annotation offset format.");
    }
  }
}

void Annotation::StatementUnknown::parseOnlyOffset(const std::string &str) {
  const size_t delimiterAfterName = str.find(':');
  if (delimiterAfterName == std::string::npos) {
    return;
  }
  const size_t delimiterAfterOffset = str.find(':', delimiterAfterName + 1);
  const size_t offsetLength =
      (delimiterAfterOffset == std::string::npos)
          ? std::string::npos
          : delimiterAfterOffset - delimiterAfterName - 1;
  parseOffset(str.substr(delimiterAfterName + 1, offsetLength));
}

Annotation::StatementDeref::StatementDeref(const std::string &str)
    : StatementUnknown(str) {}

Annotation::StatementKind Annotation::StatementDeref::getStatementName() const {
  return Annotation::StatementKind::Deref;
}

Annotation::StatementInitNull::StatementInitNull(const std::string &str)
    : StatementUnknown(str) {}

Annotation::StatementKind
Annotation::StatementInitNull::getStatementName() const {
  return Annotation::StatementKind::InitNull;
}

bool Annotation::operator==(const Annotation &other) const {
  return (functionName == other.functionName) &&
         (statements == other.statements) && (properties == other.properties);
}

void from_json(const json &j, Annotation::StatementPtr &statement) {
  if (!j.is_string()) {
    klee_error("Incorrect annotation format.");
  }
  const std::string jStr = j.get<std::string>();
  statement = toStatement(jStr.substr(0, jStr.find(':')), jStr);
}

void from_json(const json &j, Annotation::Property &property) {
  if (!j.is_string()) {
    klee_error("Incorrect properties format in annotations file.");
  }
  const std::string jStr = j.get<std::string>();

  property = Annotation::Property::Unknown;
  const auto propertyPtr = toProperties.find(jStr);
  if (propertyPtr != toProperties.end()) {
    property = propertyPtr->second;
  }
}

Annotations parseAnnotationsFile(const json &annotationsJson) {
  Annotations annotations;
  for (auto &item : annotationsJson.items()) {
    Annotation annotation;
    annotation.functionName = item.key();

    const json &j = item.value();
    if (!j.is_object() || !j.contains("annotation") ||
        !j.contains("properties")) {
      klee_error("Incorrect annotations file format.");
    }

    annotation.statements =
        j.at("annotation").get<std::vector<Annotation::StatementPtrs>>();
    annotation.properties =
        j.at("properties").get<std::set<Annotation::Property>>();
    annotations[item.key()] = annotation;
  }
  return annotations;
}

Annotations parseAnnotationsFile(const std::string &path) {
  std::ifstream annotationsFile(path);
  if (!annotationsFile.good()) {
    klee_error("Opening %s failed.", path.c_str());
  }

  json annotationsJson = json::parse(annotationsFile, nullptr, false);
  if (annotationsJson.is_discarded()) {
    klee_error("Parsing JSON %s failed.", path.c_str());
  }

  return parseAnnotationsFile(annotationsJson);
}

bool operator==(const Annotation::StatementPtr &first,
                const Annotation::StatementPtr &second) {
  if (first->getStatementName() != second->getStatementName()) {
    return false;
  }

  switch (first->getStatementName()) {
  case Annotation::StatementKind::Unknown:
    return (*dynamic_cast<Annotation::StatementUnknown *>(first.get()) ==
            *dynamic_cast<Annotation::StatementUnknown *>(second.get()));
  case Annotation::StatementKind::Deref:
    return (*dynamic_cast<Annotation::StatementDeref *>(first.get()) ==
            *dynamic_cast<Annotation::StatementDeref *>(second.get()));
  case Annotation::StatementKind::InitNull:
    return (*dynamic_cast<Annotation::StatementInitNull *>(first.get()) ==
            *dynamic_cast<Annotation::StatementInitNull *>(second.get()));
  default:
    __builtin_unreachable();
  }
}

} // namespace klee
