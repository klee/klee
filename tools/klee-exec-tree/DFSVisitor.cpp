//===-- DFSVisitor.cpp ------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "DFSVisitor.h"

#include <utility>

DFSVisitor::DFSVisitor(const Tree &tree, callbackT cb_intermediate,
                       callbackT cb_leaf) noexcept
    : tree{tree},
      cb_intermediate{std::move(cb_intermediate)}, cb_leaf{std::move(cb_leaf)} {
  run();
}

void DFSVisitor::run() const noexcept {
  // empty tree
  if (tree.nodes.size() <= 1)
    return;

  std::vector<std::tuple<std::uint32_t, std::uint32_t>> stack{
      {1, 1}}; // (id, depth)
  while (!stack.empty()) {
    std::uint32_t id, depth;
    std::tie(id, depth) = stack.back();
    stack.pop_back();
    const auto &node = tree.nodes[id];

    if (node.left || node.right) {
      if (cb_intermediate)
        cb_intermediate(id, node, depth);
      if (node.right)
        stack.emplace_back(node.right, depth + 1);
      if (node.left)
        stack.emplace_back(node.left, depth + 1);
    } else {
      if (cb_leaf)
        cb_leaf(id, node, depth);
    }
  }
}
