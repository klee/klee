//===-- ReturnSplitter.cpp
//-------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "Passes.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

using namespace llvm;

namespace klee {

char ReturnSplitter::ID = 0;

bool ReturnSplitter::runOnFunction(Function &F) {
  unsigned n = F.getBasicBlockList().size();
  BasicBlock **blocks = new BasicBlock *[n];
  unsigned i = 0;
  for (llvm::Function::iterator bbit = F.begin(), bbie = F.end(); bbit != bbie;
       bbit++, i++) {
    blocks[i] = &*bbit;
  }

  for (unsigned j = 0; j < n; j++) {
    BasicBlock *fbb = blocks[j];
    llvm::BasicBlock::iterator it = fbb->begin();
    llvm::BasicBlock::iterator ie = fbb->end();
    Instruction *firstInst = &*it;
    while (it != ie) {
      if (isa<ReturnInst>(it)) {
        Instruction *returnInst = &*it++;
        if (returnInst != firstInst) {
          fbb = fbb->splitBasicBlock(returnInst);
        }
      } else {
        it++;
      }
    }
  }

  delete[] blocks;

  return false;
}
} // namespace klee