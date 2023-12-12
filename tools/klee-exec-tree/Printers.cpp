//===-- Printers.cpp --------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "Printers.h"

#include "DFSVisitor.h"
#include "klee/Core/BranchTypes.h"
#include "klee/Core/TerminationTypes.h"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <string_view>
#include <unordered_map>

// branches

void printBranches(const Tree &tree) {
  if (tree.nodes.size() <= 1) {
    std::cout << "Empty tree.\n";
    return;
  }

  std::unordered_map<BranchType, std::uint32_t> branchTypes;

  DFSVisitor visitor(
      tree,
      [&branchTypes](std::uint32_t id, Node node, std::uint32_t depth) {
        branchTypes[std::get<BranchType>(node.kind)]++;
      },
      nullptr);

  // sort output
  std::vector<std::pair<BranchType, std::uint32_t>> sortedBranchTypes(
      branchTypes.begin(), branchTypes.end());
  std::sort(sortedBranchTypes.begin(), sortedBranchTypes.end(),
            [](const auto &lhs, const auto &rhs) {
              return (lhs.second > rhs.second) ||
                     (lhs.second == rhs.second && lhs.first < rhs.first);
            });

  std::cout << "branch type,count\n";
  for (const auto &[branchType, count] : sortedBranchTypes)
    std::cout << branchTypeNames[branchType] << ',' << count << '\n';
}

// depths

struct DepthInfo {
  std::vector<std::uint32_t> depths;
  std::uint32_t maxDepth{0};
  std::uint32_t noLeaves{0};
  std::uint32_t noNodes{0};
};

DepthInfo getDepthInfo(const Tree &tree) {
  DepthInfo I;

  DFSVisitor visitor(
      tree,
      [&](std::uint32_t id, Node node, std::uint32_t depth) { ++I.noNodes; },
      [&I](std::uint32_t id, Node node, std::uint32_t depth) {
        ++I.noLeaves;
        ++I.noNodes;
        if (depth > I.maxDepth)
          I.maxDepth = depth;
        if (depth >= I.depths.size())
          I.depths.resize(depth + 1, 0);
        ++I.depths[depth];
      });

  return I;
}

void printDepths(const Tree &tree) {
  if (tree.nodes.size() <= 1) {
    std::cout << "Empty tree.\n";
    return;
  }

  const auto DI = getDepthInfo(tree);
  std::cout << "depth,count\n";
  for (size_t depth = 1; depth <= DI.maxDepth; ++depth) {
    auto count = DI.depths[depth];
    if (count)
      std::cout << depth << ',' << count << '\n';
  }
}

// dot

std::array<std::string, 6> NodeColours = {"green", "orange",   "red",
                                          "blue",  "darkblue", "darkgrey"};

std::string_view terminationTypeColour(StateTerminationType type) {
  // Exit
  if (type == StateTerminationType::Exit)
    return NodeColours[0];

  // Early
  if ((StateTerminationType::EXIT < type &&
       type <= StateTerminationType::EARLY) ||
      (StateTerminationType::EXECERR < type &&
       type <= StateTerminationType::END)) {
    return NodeColours[1];
  }

  // Program error
  if (StateTerminationType::SOLVERERR < type &&
      type <= StateTerminationType::PROGERR)
    return NodeColours[2];

  // User error
  if (StateTerminationType::PROGERR < type &&
      type <= StateTerminationType::USERERR)
    return NodeColours[3];

  // Execution/Solver error
  if ((StateTerminationType::USERERR < type &&
       type <= StateTerminationType::EXECERR) ||
      (StateTerminationType::EARLY < type &&
       type <= StateTerminationType::SOLVERERR))
    return NodeColours[4];

  return NodeColours[5];
}

void printIntermediateNode(std::uint32_t id, Node node, std::uint32_t depth) {
  std::cout << 'N' << id << '[' << "tooltip=\""
            << branchTypeNames[std::get<BranchType>(node.kind)] << "\\n"
            << "node: " << id << "\\nstate: " << node.stateID
            << "\\nasm: " << node.asmLine << "\"];\n";
}

void printLeafNode(std::uint32_t id, Node node, std::uint32_t depth) {
  const auto terminationType = std::get<StateTerminationType>(node.kind);
  const auto colour = terminationTypeColour(terminationType);
  std::cout << 'N' << id << '[' << "tooltip=\""
            << terminationTypeNames[terminationType] << "\\n"
            << "node: " << id << "\\nstate: " << node.stateID
            << "\\nasm: " << node.asmLine << "\",color=" << colour << "];\n";
}

void printEdges(std::uint32_t id, Node node, std::uint32_t depth) {
  std::cout << 'N' << id << "->{";
  if (node.left && node.right) {
    std::cout << 'N' << node.left << " N" << node.right;
  } else {
    std::cout << 'N' << (node.left ? node.left : node.right);
  }
  std::cout << "};\n";
}

void printDOT(const Tree &tree) {
  // header
  // - style defaults to intermediate nodes
  std::cout << "strict digraph ExecutionTree {\n"
               "node[shape=point,width=0.15,color=darkgrey];\n"
               "edge[color=darkgrey];\n\n";

  // nodes
  // - change colour for leaf nodes
  // - attach branch and location info as tooltips
  DFSVisitor nodeVisitor(tree, printIntermediateNode, printLeafNode);

  // edges
  DFSVisitor edgeVisitor(tree, printEdges, nullptr);

  // footer
  std::cout << '}' << std::endl;
}

// instructions

struct Info {
  std::uint32_t noBranches{0};
  std::uint32_t noTerminations{0};
  std::map<StateTerminationType, std::uint32_t> terminationTypes;
};

void printInstructions(const Tree &tree) {
  std::map<std::uint32_t, Info> asmInfo;

  DFSVisitor visitor(
      tree,
      [&asmInfo](std::uint32_t id, Node node, std::uint32_t depth) {
        asmInfo[node.asmLine].noBranches++;
      },
      [&asmInfo](std::uint32_t id, Node node, std::uint32_t depth) {
        auto &info = asmInfo[node.asmLine];
        info.noTerminations++;
        info.terminationTypes[std::get<StateTerminationType>(node.kind)]++;
      });

  std::cout << "asm line,branches,terminations,termination types\n";
  for (const auto &[asmLine, info] : asmInfo) {
    std::cout << asmLine << ',' << info.noBranches << ',' << info.noTerminations
              << ',';
    std::string sep{""};
    for (const auto &[terminationType, count] : info.terminationTypes) {
      std::cout << sep << terminationTypeNames[terminationType] << '(' << count
                << ')';
      sep = ";";
    }
    std::cout << '\n';
  }
}

// terminations

void printTerminations(const Tree &tree) {
  if (tree.nodes.size() <= 1) {
    std::cout << "Empty tree.\n";
    return;
  }

  std::map<StateTerminationType, std::uint32_t> terminations;

  DFSVisitor visitor(
      tree, nullptr,
      [&terminations](std::uint32_t id, Node node, std::uint32_t depth) {
        terminations[std::get<StateTerminationType>(node.kind)]++;
      });

  std::cout << "termination type,count\n";
  for (const auto &[terminationType, count] : terminations)
    std::cout << terminationTypeNames[terminationType] << ',' << count << '\n';
}

// tree info

void printTreeInfo(const Tree &tree) {
  if (tree.nodes.size() <= 1) {
    std::cout << "Empty tree.\n";
    return;
  }

  const auto DI = getDepthInfo(tree);

  // determine average depth
  std::uint64_t sum{0};
  for (size_t depth = 1; depth <= DI.maxDepth; ++depth) {
    auto count = DI.depths[depth];
    if (count)
      sum += count * depth;
  }
  double avgDepth = (double)sum / DI.noLeaves;

  std::cout << "nodes: " << DI.noNodes << '\n'
            << "leaf nodes: " << DI.noLeaves
            << (DI.noNodes && (DI.noLeaves != DI.noNodes / 2 + 1)
                    ? " (not a binary tree?!)"
                    : "")
            << '\n'
            << "max. depth: " << DI.maxDepth << '\n'
            << "avg. depth: " << std::setprecision(2) << avgDepth << '\n';
}
