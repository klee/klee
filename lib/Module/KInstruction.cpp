//===-- KInstruction.cpp --------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Internal/Module/KInstruction.h"
#include "llvm/Support/raw_ostream.h"
#include <string>

using namespace llvm;
using namespace klee;

/***/

KInstruction::~KInstruction() {
  delete[] operands;
}

void KInstruction::printFileLine(llvm::raw_ostream &debugFile) const {
  if (info->file != "")
    debugFile << info->file << ":" << info->line;
  else debugFile << "[no debug info]";
}

std::string KInstruction::printFileLine() const {
  std::string str;
  llvm::raw_string_ostream oss(str);
  printFileLine(oss);
  return oss.str();
}
