//===-- KInstruction.h ------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_KINSTRUCTION_H
#define KLEE_KINSTRUCTION_H

#include <vector>

namespace llvm {
  class Instruction;
}

namespace klee {
  class Executor;
  struct InstructionInfo;
  class KModule;


  /// KInstruction - Intermediate instruction representation used
  /// during execution.
  struct KInstruction {
    llvm::Instruction *inst;    
    const InstructionInfo *info;

    /// Value numbers for each operand. -1 is an invalid value,
    /// otherwise negative numbers are indices (negated and offset by
    /// 2) into the module constant table and positive numbers are
    /// register indices.
    int *operands;
    /// Destination register index.
    unsigned dest;

  public:
    virtual ~KInstruction(); 
  };

  struct KGEPInstruction : KInstruction {
    std::vector< std::pair<unsigned, unsigned> > indices;
    unsigned offset;
  };
}

#endif

