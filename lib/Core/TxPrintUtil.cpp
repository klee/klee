//===-- TxPrintUtil ---------------------------------------------*- C++ -*-===//
//
//               The Tracer-X KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Functions to help display object contents.
///
//===----------------------------------------------------------------------===//

#include "TxPrintUtil.h"

#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 3)
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#else
#include <llvm/BasicBlock.h>
#include <llvm/Function.h>
#include <llvm/Instruction.h>
#endif

using namespace klee;

namespace klee {

bool outputFunctionName(llvm::Value *value, llvm::raw_ostream &stream) {
  llvm::Instruction *inst = llvm::dyn_cast<llvm::Instruction>(value);
  if (inst) {
    llvm::BasicBlock *bb = inst->getParent();
    if (bb) {
      llvm::Function *f = bb->getParent();
      stream << f->getName();
      return true;
    }
  }
  return false;
}

std::string makeTabs(const unsigned paddingAmount) {
  std::string tabsString;
  for (unsigned i = 0; i < paddingAmount; ++i) {
    tabsString += appendTab(tabsString);
  }
  return tabsString;
}

std::string appendTab(const std::string &prefix) { return prefix + "        "; }
}
