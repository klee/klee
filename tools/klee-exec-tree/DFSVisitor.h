//===-- DFSVisitor.h --------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#pragma once

#include "Tree.h"

#include <functional>

/// @brief Traverses an execution tree and calls registered callbacks for
/// intermediate and leaf nodes (not the classical Visitor pattern).
class DFSVisitor {
  // void _(node ID, node, depth)
  using callbackT = std::function<void(std::uint32_t, Node, std::uint32_t)>;

  const Tree &tree;
  callbackT cb_intermediate;
  callbackT cb_leaf;
  void run() const noexcept;

public:
  DFSVisitor(const Tree &tree, callbackT cb_intermediate,
             callbackT cb_leaf) noexcept;
  ~DFSVisitor() = default;
};
