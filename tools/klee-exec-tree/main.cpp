//===-- main.cpp ------------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <cstdlib>
#include <filesystem>
#include <iostream>

#include "Printers.h"

namespace fs = std::filesystem;

void print_usage() {
  std::cout << "Usage: klee-exec-tree <option> /path[/exec_tree.db]\n\n"
               "Options:\n"
               "\tbranches     -  print branch statistics in csv format\n"
               "\tdepths       -  print depths statistics in csv format\n"
               "\tinstructions -  print asm line summary in csv format\n"
               "\tterminations -  print termination statistics in csv format\n"
               "\ttree-dot     -  print tree in dot format\n"
               "\ttree-info    -  print tree statistics"
               "\n";
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    print_usage();
    exit(EXIT_FAILURE);
  }

  // parse options
  void (*action)(const Tree &);
  std::string option(argv[1]);
  if (option == "branches") {
    action = printBranches;
  } else if (option == "instructions") {
    action = printInstructions;
  } else if (option == "depths") {
    action = printDepths;
  } else if (option == "terminations") {
    action = printTerminations;
  } else if (option == "tree-dot") {
    action = printDOT;
  } else if (option == "tree-info") {
    action = printTreeInfo;
  } else {
    print_usage();
    exit(EXIT_FAILURE);
  }

  // create tree
  fs::path path{argv[2]};
  if (fs::is_directory(path))
    path /= "exec_tree.db";
  if (!fs::exists(path)) {
    std::cerr << "Cannot open " << path << '\n';
    exit(EXIT_FAILURE);
  }
  Tree tree(path);

  // print results
  action(tree);
}
