//===-- CallSplitter.cpp --------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "Passes.h"

#include "klee/Support/CompilerWarning.h"
DISABLE_WARNING_PUSH
DISABLE_WARNING_DEPRECATED_DECLARATIONS
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Module.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
DISABLE_WARNING_POP

using namespace llvm;

namespace klee {

char CallSplitter::ID = 0;

bool CallSplitter::runOnFunction(Function &F) {
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
      if (isa<CallInst>(it)) {
        Instruction *callInst = &*it++;
        Instruction *afterCallInst = &*it;
        if (callInst != firstInst) {
          fbb = fbb->splitBasicBlock(callInst);
        }
        if ((isa<BranchInst>(afterCallInst) &&
             cast<BranchInst>(afterCallInst)->isUnconditional()) ||
            isa<UnreachableInst>(afterCallInst)) {
          break;
        }
        fbb = fbb->splitBasicBlock(afterCallInst);
        it = fbb->begin();
        ie = fbb->end();
        firstInst = &*it;
      } else if (isa<InvokeInst>(it)) {
        Instruction *invokeInst = &*it++;
        if (invokeInst != firstInst) {
          fbb = fbb->splitBasicBlock(invokeInst);
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