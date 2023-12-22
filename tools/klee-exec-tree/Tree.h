//===-- Tree.h --------------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#pragma once

#include "klee/Core/BranchTypes.h"
#include "klee/Core/TerminationTypes.h"

#include <filesystem>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

inline std::unordered_set<BranchType> validBranchTypes;
inline std::unordered_set<StateTerminationType> validTerminationTypes;
inline std::unordered_map<BranchType, std::string> branchTypeNames;
inline std::unordered_map<StateTerminationType, std::string>
    terminationTypeNames;

///@brief A Tree node representing an ExecutionTreeNode
struct Node final {
  std::uint32_t left{0};
  std::uint32_t right{0};
  std::uint32_t stateID{0};
  std::uint32_t asmLine{0};
  std::variant<BranchType, StateTerminationType> kind{BranchType::NONE};
};

///@brief An in-memory representation of a complete execution tree
class Tree final {
  /// Creates branchTypeNames and terminationTypeNames maps
  static void initialiseTypeNames();
  /// Creates validBranchTypes and validTerminationTypes sets
  static void initialiseValidTypes();
  /// Checks tree properties (e.g. valid branch/termination types)
  void sanityCheck();

public:
  /// sorted vector of Nodes default initialised with BranchType::NONE
  std::vector<Node> nodes; // ExecutionTree node IDs start with 1!

  /// Reads complete exec-tree.db into memory
  explicit Tree(const std::filesystem::path &path);
  ~Tree() = default;
};
