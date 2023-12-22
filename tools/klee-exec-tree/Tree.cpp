//===-- Tree.cpp ------------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "Tree.h"

#include <sqlite3.h>

#include <cassert>
#include <cstdlib>
#include <iostream>

Tree::Tree(const std::filesystem::path &path) {
  // open db
  ::sqlite3 *db;
  if (sqlite3_open_v2(path.c_str(), &db, SQLITE_OPEN_READONLY, nullptr) !=
      SQLITE_OK) {
    std::cerr << "Cannot open execution tree database: " << sqlite3_errmsg(db)
              << std::endl;
    exit(EXIT_FAILURE);
  }

  // initialise prepared statement
  ::sqlite3_stmt *readStmt;
  std::string query{
      "SELECT ID, stateID, leftID, rightID, asmLine, kind FROM nodes;"};
  if (sqlite3_prepare_v3(db, query.c_str(), -1, SQLITE_PREPARE_PERSISTENT,
                         &readStmt, nullptr) != SQLITE_OK) {
    std::cerr << "Cannot prepare read statement: " << sqlite3_errmsg(db)
              << std::endl;
    exit(EXIT_FAILURE);
  }

  ::sqlite3_stmt *maxStmt;
  query = "SELECT MAX(ID) FROM nodes;";
  if (sqlite3_prepare_v3(db, query.c_str(), -1, SQLITE_PREPARE_PERSISTENT,
                         &maxStmt, nullptr) != SQLITE_OK) {
    std::cerr << "Cannot prepare max statement: " << sqlite3_errmsg(db)
              << std::endl;
    exit(EXIT_FAILURE);
  }

  // read max id
  int rc;
  std::uint64_t maxID;
  if ((rc = sqlite3_step(maxStmt)) == SQLITE_ROW) {
    maxID = static_cast<std::uint32_t>(sqlite3_column_int(maxStmt, 0));
  } else {
    std::cerr << "Cannot read maximum ID: " << sqlite3_errmsg(db) << std::endl;
    exit(EXIT_FAILURE);
  }

  // reserve space
  nodes.resize(maxID + 1, {});

  // read rows into vector
  while ((rc = sqlite3_step(readStmt)) == SQLITE_ROW) {
    const auto ID = static_cast<std::uint32_t>(sqlite3_column_int(readStmt, 0));
    const auto stateID =
        static_cast<std::uint32_t>(sqlite3_column_int(readStmt, 1));
    const auto left =
        static_cast<std::uint32_t>(sqlite3_column_int(readStmt, 2));
    const auto right =
        static_cast<std::uint32_t>(sqlite3_column_int(readStmt, 3));
    const auto asmLine =
        static_cast<std::uint32_t>(sqlite3_column_int(readStmt, 4));
    const auto tmpKind =
        static_cast<std::uint8_t>(sqlite3_column_int(readStmt, 5));

    // sanity checks: valid indices
    if (ID == 0) {
      std::cerr << "ExecutionTree DB contains illegal node ID (0)" << std::endl;
      exit(EXIT_FAILURE);
    }

    if (left > maxID || right > maxID) {
      std::cerr << "ExecutionTree DB contains references to non-existing nodes "
                   "(> max. ID) in node "
                << ID << std::endl;
      exit(EXIT_FAILURE);
    }

    if ((left == 0 && right != 0) || (left != 0 && right == 0)) {
      std::cerr << "Warning: ExecutionTree DB contains ambiguous node "
                   "(0 vs. non-0 children): "
                << ID << std::endl;
    }

    // determine node kind (branch or leaf node)
    decltype(Node::kind) kind;
    if (left == 0 && right == 0) {
      kind = static_cast<StateTerminationType>(tmpKind);
    } else {
      kind = static_cast<BranchType>(tmpKind);
    }

    // store children
    nodes[ID] = {.left = left,
                 .right = right,
                 .stateID = stateID,
                 .asmLine = asmLine,
                 .kind = kind};
  }

  if (rc != SQLITE_DONE) {
    std::cerr << "Error while reading database: " << sqlite3_errmsg(db)
              << std::endl;
    exit(EXIT_FAILURE);
  }

  // close db
  sqlite3_finalize(maxStmt);
  sqlite3_finalize(readStmt);
  sqlite3_close(db);

  // initialise global sets/maps and sanity check
  initialiseValidTypes();
  sanityCheck();
  initialiseTypeNames();
}

void Tree::initialiseTypeNames() {
// branch types
#undef BTYPE
#define BTYPE(Name, I) branchTypeNames[BranchType::Name] = #Name;
  BRANCH_TYPES

// termination types
#undef TTYPE
#define TTYPE(Name, I, S)                                                      \
  terminationTypeNames[StateTerminationType::Name] = #Name;
  TERMINATION_TYPES
}

void Tree::initialiseValidTypes() {
// branch types
#undef BTYPE
#define BTYPE(Name, I) validBranchTypes.insert(BranchType::Name);
  BRANCH_TYPES

// termination types
#undef TTYPE
#define TTYPE(Name, I, S)                                                      \
  validTerminationTypes.insert(StateTerminationType::Name);
  TERMINATION_TYPES
}

void Tree::sanityCheck() {
  if (nodes.size() <= 1) // [0] is unused
    return;

  std::vector<std::uint32_t> stack{1}; // root ID
  std::unordered_set<std::uint32_t> visited;
  while (!stack.empty()) {
    const auto id = stack.back();
    stack.pop_back();

    if (!visited.insert(id).second) {
      std::cerr
          << "ExecutionTree DB contains duplicate child reference or circular "
             "structure. Affected node: "
          << id << std::endl;
      exit(EXIT_FAILURE);
    }

    const auto &node = nodes[id];

    // default constructed "gap" in vector
    if (!node.left && !node.right &&
        std::holds_alternative<BranchType>(node.kind) &&
        static_cast<std::uint8_t>(std::get<BranchType>(node.kind)) == 0u) {
      std::cerr << "ExecutionTree DB references undefined node. Affected node: "
                << id << std::endl;
      exit(EXIT_FAILURE);
    }

    if (node.left || node.right) {
      if (node.right)
        stack.push_back(node.right);
      if (node.left)
        stack.push_back(node.left);
      // valid branch types
      assert(std::holds_alternative<BranchType>(node.kind));
      const auto branchType = std::get<BranchType>(node.kind);
      if (validBranchTypes.count(branchType) == 0) {
        std::cerr << "ExecutionTree DB contains unknown branch type ("
                  << (unsigned)static_cast<std::uint8_t>(branchType)
                  << ") in node " << id << std::endl;
        exit(EXIT_FAILURE);
      }
    } else {
      // valid termination types
      assert(std::holds_alternative<StateTerminationType>(node.kind));
      const auto terminationType = std::get<StateTerminationType>(node.kind);
      if (validTerminationTypes.count(terminationType) == 0 ||
          terminationType == StateTerminationType::RUNNING) {
        std::cerr << "ExecutionTree DB contains unknown termination type ("
                  << (unsigned)static_cast<std::uint8_t>(terminationType)
                  << ") in node " << id << std::endl;
        exit(EXIT_FAILURE);
      }
    }
  }
}
