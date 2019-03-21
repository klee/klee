//===-- LowerSwitch.cpp - Eliminate Switch instructions -------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Derived from LowerSwitch.cpp in LLVM, heavily modified by piotrek
// to get rid of the binary search transform, as it was creating
// multiple paths through the program (i.e., extra paths that didn't
// exist in the original program).
//
//===----------------------------------------------------------------------===//

#include "Passes.h"
#include "klee/Config/Version.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include <algorithm>

using namespace llvm;

namespace klee {

char LowerSwitchPass::ID = 0;

// The comparison function for sorting the switch case values in the vector.
struct SwitchCaseCmp {
  bool operator () (const LowerSwitchPass::SwitchCase& C1,
                    const LowerSwitchPass::SwitchCase& C2) {
    
    const ConstantInt* CI1 = cast<const ConstantInt>(C1.value);
    const ConstantInt* CI2 = cast<const ConstantInt>(C2.value);
    return CI1->getValue().slt(CI2->getValue());
  }
};

bool LowerSwitchPass::runOnFunction(Function &F) {
  bool changed = false;

  for (Function::iterator I = F.begin(), E = F.end(); I != E; ) {
    BasicBlock *cur = &*I;
    I++; // Advance over block so we don't traverse new blocks

    if (SwitchInst *SI = dyn_cast<SwitchInst>(cur->getTerminator())) {
      changed = true;
      processSwitchInst(SI);
    }
  }

  return changed;
}

// switchConvert - Convert the switch statement into a linear scan
// through all the case values
void LowerSwitchPass::switchConvert(CaseItr begin, CaseItr end,
                                    Value* value, BasicBlock* origBlock,
                                    BasicBlock* defaultBlock)
{
  BasicBlock *curHead = defaultBlock;
  Function *F = origBlock->getParent();
  llvm::IRBuilder<> Builder(defaultBlock);

  // iterate through all the cases, creating a new BasicBlock for each
  for (CaseItr it = begin; it < end; ++it) {
    BasicBlock *newBlock = BasicBlock::Create(F->getContext(), "NodeBlock");
    Function::iterator FI = origBlock->getIterator();
    F->getBasicBlockList().insert(++FI, newBlock);
    Builder.SetInsertPoint(newBlock);
    auto cmpValue = Builder.CreateICmpEQ(value, it->value, "case.cmp");
    Builder.CreateCondBr(cmpValue, it->block, curHead);

    // If there were any PHI nodes in this successor, rewrite one entry
    // from origBlock to come from newBlock.
    for (BasicBlock::iterator bi = it->block->begin(); isa<PHINode>(bi); ++bi) {
      PHINode* PN = cast<PHINode>(bi);

      int blockIndex = PN->getBasicBlockIndex(origBlock);
      assert(blockIndex != -1 && "Switch didn't go to this successor??");
      PN->setIncomingBlock((unsigned)blockIndex, newBlock);
    }
    
    curHead = newBlock;
  }

  // Branch to our shiny new if-then stuff...
  Builder.SetInsertPoint(origBlock);
  Builder.CreateBr(curHead);
}

// processSwitchInst - Replace the specified switch instruction with a sequence
// of chained if-then instructions.
//
void LowerSwitchPass::processSwitchInst(SwitchInst *SI) {
  BasicBlock *origBlock = SI->getParent();
  BasicBlock *defaultBlock = SI->getDefaultDest();
  Function *F = origBlock->getParent();
  Value *switchValue = SI->getCondition();

  // Create a new, empty default block so that the new hierarchy of
  // if-then statements go to this and the PHI nodes are happy.
  BasicBlock* newDefault = BasicBlock::Create(F->getContext(), "newDefault");
  llvm::IRBuilder<> Builder(newDefault);

  F->getBasicBlockList().insert(defaultBlock->getIterator(), newDefault);
  Builder.CreateBr(defaultBlock);

  // If there is an entry in any PHI nodes for the default edge, make sure
  // to update them as well.
  for (BasicBlock::iterator I = defaultBlock->begin(); isa<PHINode>(I); ++I) {
    PHINode *PN = cast<PHINode>(I);
    int BlockIdx = PN->getBasicBlockIndex(origBlock);
    assert(BlockIdx != -1 && "Switch didn't go to this successor??");
    PN->setIncomingBlock((unsigned)BlockIdx, newDefault);
  }
  
  CaseVector cases;

  for (auto i : SI->cases())
    cases.push_back(SwitchCase(i.getCaseValue(),
                               i.getCaseSuccessor()));
  
  // reverse cases, as switchConvert constructs a chain of
  //   basic blocks by appending to the front. if we reverse,
  //   the if comparisons will happen in the same order
  //   as the cases appear in the switch
  std::reverse(cases.begin(), cases.end());
  
  switchConvert(cases.begin(), cases.end(), switchValue, origBlock, newDefault);

  // We are now done with the switch instruction, so delete it
  origBlock->getInstList().erase(SI);
}

}
