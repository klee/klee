/*
 * This source file has been modified by Huawei. Copyright (c) 2021
 */

//===-- KInstruction.cpp --------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Module/KInstruction.h"
#include <string>

using namespace llvm;
using namespace klee;

/***/

KInstruction::KInstruction(const KInstruction& ki):
  inst(ki.inst),
  info(ki.info),
  operands(ki.operands),
  dest(ki.dest) {}

KInstruction::~KInstruction() {
  delete[] operands;
}

KGEPInstruction::KGEPInstruction(const KGEPInstruction& ki):
  KInstruction(ki),
  indices(ki.indices),
  offset(ki.offset) {}

std::string KInstruction::getSourceLocation() const {
  if (!info->file.empty())
    return info->file + ":" + std::to_string(info->line) + " " +
           std::to_string(info->column);
  else return "[no debug info]";
}
