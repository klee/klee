//===-- CodeLocation.h ------------------------------------------*- C++ -*-===//
//
//                     The KLEEF Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#ifndef KLEE_CODE_LOCATION_H
#define KLEE_CODE_LOCATION_H

#include "klee/ADT/Ref.h"
#include "klee/Expr/Path.h"
#include "klee/Module/LocationInfo.h"

#include <cstdint>
#include <optional>
#include <string>

namespace klee {

struct PhysicalLocationJson;
struct KValue;

/// @brief Represents the location in source code with additional
/// information about place in the state's path.
struct CodeLocation {
  /// @brief Required by klee::ref-managed objects
  class ReferenceCounter _refCount;

  /// @brief Path index in `Path`.
  const Path::PathIndex pathIndex;

  /// @brief Corresponding llvm entity.
  const KValue *source;

  /// @brief Location in source code.
  const LocationInfo location;

private:
  CodeLocation(const Path::PathIndex &pathIndex, const KValue *source,
               const std::string &sourceFilename, uint64_t sourceCodeLine,
               std::optional<uint64_t> sourceCodeColumn);

  CodeLocation(const CodeLocation &) = delete;
  CodeLocation &operator=(const CodeLocation &) = delete;

public:
  /// @brief Factory method for `CodeLocation` enhanced with `PathIndex`
  /// in history. Wraps constructed objects in the ref to provide
  /// zero-cost copying of code locations.
  /// @param sourceFilename Name of source file to which location refers.
  /// @param sourceCodeLine Line in source to which location refers.
  /// @param sourceCodeColumn Column in source code to which location refers.
  /// @return `CodeLocation` representing the location in source code.
  static ref<CodeLocation> create(const Path::PathIndex &pathIndex,
                                  const KValue *source,
                                  const std::string &sourceFilename,
                                  uint64_t sourceCodeLine,
                                  std::optional<uint64_t> sourceCodeColumn);

  /// @brief Factory method for `CodeLocation`. Wraps constructed
  /// objects in the ref to provide zero-cost copying of code locations.
  /// @param sourceFilename Name of source file to which location refers.
  /// @param sourceCodeLine Line in source to which location refers.
  /// @param sourceCodeColumn Column in source code to which location refers.
  /// @return `CodeLocation` representing the location in source code.
  static ref<CodeLocation> create(const KValue *source,
                                  const std::string &sourceFilename,
                                  uint64_t sourceCodeLine,
                                  std::optional<uint64_t> sourceCodeColumn);

  /// @brief Converts code location info to SARIFs representation
  /// of location.
  /// @param location location info in source code.
  /// @return SARIFs representation of location.
  PhysicalLocationJson serialize() const;
};

}; // namespace klee

#endif // KLEE_CODE_LOCATION_H
