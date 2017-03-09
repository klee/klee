//===-- KInstruction.cpp --------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Internal/Module/KInstruction.h"

using namespace llvm;
using namespace klee;

/***/

KInstruction::~KInstruction() {
  delete[] operands;
}

void KInstruction::printFileLine(llvm::raw_ostream &debugFile) {
  if (info->file != "")
    debugFile << info->file << ":" << info->line;
  else
    debugFile << "[no debug info]";
}
