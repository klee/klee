//===-- Annotation.h --------------------------------------------*- C++ -*-===//
//
//                     The KLEEF Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//===----------------------------------------------------------------------===//

#ifndef KLEE_ANNOTATION_H
#define KLEE_ANNOTATION_H

#include "nlohmann/json_fwd.hpp"

#include <map>
#include <set>
#include <string>
#include <vector>

using json = nlohmann::json;

namespace klee {

namespace Statement {
enum class Kind {
  Unknown,

  Deref,
  InitNull,
  MaybeInitNull,
  // TODO: rename to alloc
  AllocSource,
  Free
};

enum class Property {
  Unknown,

  Deterministic,
  Noreturn,
};

struct Unknown {
protected:
  std::string rawAnnotation;
  std::string rawOffset;
  std::string rawValue;

public:
  std::vector<std::string> offset;

  explicit Unknown(const std::string &str = "Unknown");
  virtual ~Unknown();

  virtual bool operator==(const Unknown &other) const;
  [[nodiscard]] virtual Kind getKind() const;

  [[nodiscard]] const std::vector<std::string> &getOffset() const;
  [[nodiscard]] std::string toString() const;
};

struct Deref final : public Unknown {
  explicit Deref(const std::string &str = "Deref");

  [[nodiscard]] Kind getKind() const override;
};

struct InitNull final : public Unknown {
public:
  explicit InitNull(const std::string &str = "InitNull");

  [[nodiscard]] Kind getKind() const override;
};

struct MaybeInitNull final : public Unknown {
public:
  explicit MaybeInitNull(const std::string &str = "MaybeInitNull");

  [[nodiscard]] Kind getKind() const override;
};

struct Alloc final : public Unknown {
public:
  enum Type {
    ALLOC = 1,        // malloc, calloc, realloc
    NEW = 2,          // operator new
    NEW_BRACKETS = 3, // operator new[]
    OPEN_FILE = 4,    // open file (fopen, open)
    MUTEX_LOCK = 5    // mutex lock (pthread_mutex_lock)
  };

  Type value = Alloc::Type::ALLOC;

  explicit Alloc(const std::string &str = "AllocSource::1");

  [[nodiscard]] Kind getKind() const override;
};

struct Free final : public Unknown {
public:
  enum Type {
    FREE = 1,            // Kind of free function
    DELETE = 2,          // operator delete
    DELETE_BRACKETS = 3, // operator delete[]
    CLOSE_FILE = 4,      // close file
    MUTEX_UNLOCK = 5     // mutex unlock (pthread_mutex_unlock)
  };

  Type value = Free::Type::FREE;

  explicit Free(const std::string &str = "FreeSource::1");

  [[nodiscard]] Kind getKind() const override;
};

using Ptr = std::shared_ptr<Unknown>;
bool operator==(const Ptr &first, const Ptr &second);
} // namespace Statement

// Annotation format: https://github.com/UnitTestBot/klee/discussions/92
struct Annotation {
  std::string functionName;
  std::vector<Statement::Ptr> returnStatements;
  std::vector<std::vector<Statement::Ptr>> argsStatements;
  std::set<Statement::Property> properties;

  bool operator==(const Annotation &other) const;
};

using AnnotationsMap = std::map<std::string, Annotation>;

AnnotationsMap parseAnnotationsJson(const json &annotationsJson);
AnnotationsMap parseAnnotations(const std::string &path);
} // namespace klee

#endif // KLEE_ANNOTATION_H
