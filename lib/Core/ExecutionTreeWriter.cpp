//===-- ExecutionTreeWriter.cpp -------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "ExecutionTreeWriter.h"

#include "ExecutionTree.h"
#include "klee/Support/ErrorHandling.h"
#include "klee/Support/OptionCategories.h"

#include "llvm/Support/CommandLine.h"

namespace {
llvm::cl::opt<unsigned> BatchSize(
    "exec-tree-batch-size", llvm::cl::init(100U),
    llvm::cl::desc("Number of execution tree nodes to batch for writing, "
                   "see --write-exec-tree (default=100)"),
    llvm::cl::cat(klee::ExecTreeCat));
} // namespace

using namespace klee;

void prepare_statement(sqlite3 *db, const std::string &query, sqlite3_stmt **stmt) {
  int result;
#ifdef SQLITE_PREPARE_PERSISTENT
  result = sqlite3_prepare_v3(db, query.c_str(), -1, SQLITE_PREPARE_PERSISTENT,
                              stmt, nullptr);
#else
  result = sqlite3_prepare_v3(db, query.c_str(), -1, 0, stmt, nullptr);
#endif
  if (result != SQLITE_OK) {
    klee_warning("Execution tree database: cannot prepare query: %s [%s]",
                 sqlite3_errmsg(db), query.c_str());
    sqlite3_close(db);
    klee_error("Execution tree database: cannot prepare query: %s",
               query.c_str());
  }
}

ExecutionTreeWriter::ExecutionTreeWriter(const std::string &dbPath) {
  // create database file
  if (sqlite3_open(dbPath.c_str(), &db) != SQLITE_OK)
    klee_error("Cannot create execution tree database: %s", sqlite3_errmsg(db));

  // - set options: asynchronous + WAL
  char *errMsg = nullptr;
  if (sqlite3_exec(db, "PRAGMA synchronous = OFF;", nullptr, nullptr,
                   &errMsg) != SQLITE_OK) {
    klee_warning("Execution tree database: cannot set option: %s", errMsg);
    sqlite3_free(errMsg);
  }
  if (sqlite3_exec(db, "PRAGMA journal_mode = WAL;", nullptr, nullptr,
                   &errMsg) != SQLITE_OK) {
    klee_warning("Execution tree database: cannot set option: %s", errMsg);
    sqlite3_free(errMsg);
  }

  // - create table
  std::string query =
      "CREATE TABLE IF NOT EXISTS nodes ("
      "ID INT PRIMARY KEY, stateID INT, leftID INT, rightID INT,"
      "asmLine INT, kind INT);";
  char *zErr = nullptr;
  if (sqlite3_exec(db, query.c_str(), nullptr, nullptr, &zErr) != SQLITE_OK) {
    klee_warning("Execution tree database: initialisation error: %s", zErr);
    sqlite3_free(zErr);
    sqlite3_close(db);
    klee_error("Execution tree database: initialisation error.");
  }

  // create prepared statements
  // - insertStmt
  query = "INSERT INTO nodes VALUES (?, ?, ?, ?, ?, ?);";
  prepare_statement(db, query, &insertStmt);
  // - transactionBeginStmt
  query = "BEGIN TRANSACTION";
  prepare_statement(db, query, &transactionBeginStmt);
  // - transactionCommitStmt
  query = "COMMIT TRANSACTION";
  prepare_statement(db, query, &transactionCommitStmt);

  // begin transaction
  if (sqlite3_step(transactionBeginStmt) != SQLITE_DONE) {
    klee_warning("Execution tree database: cannot begin transaction: %s",
                 sqlite3_errmsg(db));
  }
  if (sqlite3_reset(transactionBeginStmt) != SQLITE_OK) {
    klee_warning("Execution tree database: cannot reset transaction: %s",
                 sqlite3_errmsg(db));
  }
}

ExecutionTreeWriter::~ExecutionTreeWriter() {
  batchCommit(!flushed);

  // finalize prepared statements
  sqlite3_finalize(insertStmt);
  sqlite3_finalize(transactionBeginStmt);
  sqlite3_finalize(transactionCommitStmt);

  // commit
  if (sqlite3_exec(db, "END TRANSACTION", nullptr, nullptr, nullptr) !=
      SQLITE_OK) {
    klee_warning("Execution tree database: cannot end transaction: %s",
                 sqlite3_errmsg(db));
  }

  if (sqlite3_close(db) != SQLITE_OK) {
    klee_warning("Execution tree database: cannot close database: %s",
                 sqlite3_errmsg(db));
  }
}

void ExecutionTreeWriter::batchCommit(bool force) {
  ++batch;
  if (batch < BatchSize && !force)
    return;

  // commit and begin transaction
  if (sqlite3_step(transactionCommitStmt) != SQLITE_DONE) {
    klee_warning("Execution tree database: transaction commit error: %s",
                 sqlite3_errmsg(db));
  }

  if (sqlite3_reset(transactionCommitStmt) != SQLITE_OK) {
    klee_warning("Execution tree database: transaction reset error: %s",
                 sqlite3_errmsg(db));
  }

  if (sqlite3_step(transactionBeginStmt) != SQLITE_DONE) {
    klee_warning("Execution tree database: transaction begin error: %s",
                 sqlite3_errmsg(db));
  }

  if (sqlite3_reset(transactionBeginStmt) != SQLITE_OK) {
    klee_warning("Execution tree database: transaction reset error: %s",
                 sqlite3_errmsg(db));
  }

  batch = 0;
  flushed = true;
}

void ExecutionTreeWriter::write(const AnnotatedExecutionTreeNode &node) {
  unsigned rc = 0;

  // bind values (SQLITE_OK is defined as 0 - just check success once at the
  // end)
  rc |= sqlite3_bind_int64(insertStmt, 1, node.id);
  rc |= sqlite3_bind_int(insertStmt, 2, node.stateID);
  rc |= sqlite3_bind_int64(
      insertStmt, 3,
      node.left.getPointer()
          ? (static_cast<AnnotatedExecutionTreeNode *>(node.left.getPointer()))->id
          : 0);
  rc |= sqlite3_bind_int64(
      insertStmt, 4,
      node.right.getPointer()
          ? (static_cast<AnnotatedExecutionTreeNode *>(node.right.getPointer()))->id
          : 0);
  rc |= sqlite3_bind_int(insertStmt, 5, node.asmLine);
  std::uint8_t value{0};
  if (std::holds_alternative<BranchType>(node.kind)) {
    value = static_cast<std::uint8_t>(std::get<BranchType>(node.kind));
  } else if (std::holds_alternative<StateTerminationType>(node.kind)) {
    value =
        static_cast<std::uint8_t>(std::get<StateTerminationType>(node.kind));
  } else {
    assert(false && "ExecutionTreeWriter: Illegal node kind!");
  }
  rc |= sqlite3_bind_int(insertStmt, 6, value);
  if (rc != SQLITE_OK) {
    // This is either a programming error (e.g. SQLITE_MISUSE) or we ran out of
    // resources (e.g. SQLITE_NOMEM). Calling sqlite3_errmsg() after a possible
    // successful call above is undefined, hence no error message here.
    klee_error("Execution tree database: cannot persist data for node: %u",
               node.id);
  }

  // insert
  if (sqlite3_step(insertStmt) != SQLITE_DONE) {
    klee_warning(
        "Execution tree database: cannot persist data for node: %u: %s",
        node.id, sqlite3_errmsg(db));
  }

  if (sqlite3_reset(insertStmt) != SQLITE_OK) {
    klee_warning("Execution tree database: error reset node: %u: %s", node.id,
                 sqlite3_errmsg(db));
  }

  batchCommit();
}
