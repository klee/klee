//===-- Printers.h ----------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#pragma once

#include "Tree.h"

/// print branch types in csv format
void printBranches(const Tree &tree);

/// print depths in csv format
void printDepths(const Tree &tree);

/// print tree in dot format
void printDOT(const Tree &tree);

/// print instruction information in csv format
void printInstructions(const Tree &tree);

/// print termination types in csv format
void printTerminations(const Tree &tree);

/// print tree/node information
void printTreeInfo(const Tree &tree);
