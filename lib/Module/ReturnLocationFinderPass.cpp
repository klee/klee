//===-- ReturnLocationFinderPass.cpp ----------------------------*- C++ -*-===//
//
//                     The KLEEF Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#include "Passes.h"

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Metadata.h"
#include "llvm/Support/Casting.h"

using namespace klee;

/// @brief Determines whether given block is a "return" block.
/// @param block block to check.
/// @return true iff block is a "return" block.
///
/// @details Check is based on form of return block: either
/// just a single `ret` instruction without return value or
/// exactly two instructions `load` and `ret` where `ret`
/// return value loaded by `load`.
static bool isReturnBlock(const llvm::BasicBlock *block) {
  bool shouldReturnConstant = false;

  switch (block->size()) {
  case 2: {
    if (!llvm::isa<const llvm::LoadInst>(&block->front())) {
      return false;
    }
    shouldReturnConstant = true;
    [[fallthrough]];
  }
  case 1: {
    const llvm::ReturnInst *returnInstruction =
        llvm::dyn_cast<const llvm::ReturnInst>(&block->back());
    if (!returnInstruction) {
      return false;
    }
    // `isa_and_nonnull` required as return value may not exist
    return shouldReturnConstant == llvm::isa_and_nonnull<const llvm::Constant>(
                                       returnInstruction->getReturnValue());
  }
  default: {
    return false;
  }
  }
}

////////////////////////////////////////////////

char ReturnLocationFinderPass::ID = 0;

bool ReturnLocationFinderPass::runOnFunction(llvm::Function &function) {
  llvm::BasicBlock &terminatorBlock = function.back();

  if (!isReturnBlock(&terminatorBlock)) {
    return false;
  }

  for (auto predecessorBlock : predecessors(&terminatorBlock)) {
    llvm::Instruction *predecessorTerminator =
        predecessorBlock->getTerminator();
    // Predecessor instruction should be a `br` instruction.
    // Sometimes optimizer may generate `switch` instruction
    // with branch at return block. But it is not mapped
    // to a `return` statement in source code.
    if (llvm::isa<llvm::BranchInst>(predecessorTerminator)) {
      // Attach metadata to the instruction.
      llvm::MDTuple *predecessorMetadataTuple =
          llvm::MDNode::get(predecessorTerminator->getContext(), {});
      predecessorTerminator->setMetadata("md.ret", predecessorMetadataTuple);
    }
  }

  return true;
}
