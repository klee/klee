//===-- KInstruction.cpp --------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Module/KInstruction.h"
#include "klee/Module/KModule.h"
#include "llvm/ADT/StringExtras.h"
#include <string>

using namespace llvm;
using namespace klee;

/***/

KInstruction::~KInstruction() { delete[] operands; }

std::string KInstruction::getSourceLocation() const {
  if (!info->file.empty())
    return info->file + ":" + std::to_string(info->line) + " " +
           std::to_string(info->column);
  else
    return "[no debug info]";
}

std::string KInstruction::toString() const {
  return llvm::utostr(index) + " at " + parent->toString() + " (" +
         inst->getOpcodeName() + ")";
}
