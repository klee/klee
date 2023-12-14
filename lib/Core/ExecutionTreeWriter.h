//===-- PTreeWriter.h -------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#pragma once

#include <sqlite3.h>

#include <cstdint>
#include <string>

namespace klee {
class AnnotatedPTreeNode;

/// @brief Writes process tree nodes into an SQLite database
class PTreeWriter {
  friend class PersistentPTree;

  ::sqlite3 *db{nullptr};
  ::sqlite3_stmt *insertStmt{nullptr};
  ::sqlite3_stmt *transactionBeginStmt{nullptr};
  ::sqlite3_stmt *transactionCommitStmt{nullptr};
  std::uint32_t batch{0};
  bool flushed{true};

  /// Writes nodes in batches
  void batchCommit(bool force = false);

public:
  explicit PTreeWriter(const std::string &dbPath);
  ~PTreeWriter();
  PTreeWriter(const PTreeWriter &other) = delete;
  PTreeWriter(PTreeWriter &&other) noexcept = delete;
  PTreeWriter &operator=(const PTreeWriter &other) = delete;
  PTreeWriter &operator=(PTreeWriter &&other) noexcept = delete;

  /// Write new node into database
  void write(const AnnotatedPTreeNode &node);
};

} // namespace klee
