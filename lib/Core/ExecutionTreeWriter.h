//===-- ExecutionTreeWriter.h -----------------------------------*- C++ -*-===//
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
class AnnotatedExecutionTreeNode;

/// @brief Writes execution tree nodes into an SQLite database
class ExecutionTreeWriter {
  friend class PersistentExecutionTree;

  ::sqlite3 *db{nullptr};
  ::sqlite3_stmt *insertStmt{nullptr};
  ::sqlite3_stmt *transactionBeginStmt{nullptr};
  ::sqlite3_stmt *transactionCommitStmt{nullptr};
  std::uint32_t batch{0};
  bool flushed{true};

  /// Writes nodes in batches
  void batchCommit(bool force = false);

public:
  explicit ExecutionTreeWriter(const std::string &dbPath);
  ~ExecutionTreeWriter();
  ExecutionTreeWriter(const ExecutionTreeWriter &other) = delete;
  ExecutionTreeWriter(ExecutionTreeWriter &&other) noexcept = delete;
  ExecutionTreeWriter &operator=(const ExecutionTreeWriter &other) = delete;
  ExecutionTreeWriter &operator=(ExecutionTreeWriter &&other) noexcept = delete;

  /// Write new node into database
  void write(const AnnotatedExecutionTreeNode &node);
};

} // namespace klee
