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

#include "klee/Config/Version.h"
#include "klee/Module/InstructionInfoTable.h"

#include "llvm/Support/DataTypes.h"
#include "llvm/Support/raw_ostream.h"

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
  struct KInstruction { //klee中表示指令的数据结构，包括指令和指令信息等
    llvm::Instruction *inst;    
    const InstructionInfo *info; //info包括指令id，指令在源码中的行数，指令在源码中的列数，指令在完整assembly.ll中的行数，指令所在源码文件名等

    /// Value numbers for each operand. -1 is an invalid value,
    /// otherwise negative numbers are indices (negated and offset by
    /// 2) into the module constant table and positive numbers are
    /// register indices.
    /// 一个int数组，保存指令的操作数数值，其中存储的信息通过计算获取
    /// 取出来是-1表示操作数无效，其他负数是constant table中的索引（将负数取反并-2），正数是寄存器索引
    int *operands; 
    /// Destination register index.
    unsigned dest; //指令的目标寄存器地址？

  public:
    virtual ~KInstruction();
    std::string getSourceLocation() const; //获得指令在源码中的位置信息

  };

  struct KGEPInstruction : KInstruction {
    /// indices - The list of variable sized adjustments to add to the pointer
    /// operand to execute the instruction. The first element is the operand
    /// index into the GetElementPtr instruction, and the second element is the
    /// element size to multiple that index by.
    std::vector< std::pair<unsigned, uint64_t> > indices;

    /// offset - A constant offset to add to the pointer operand to execute the
    /// instruction.
    uint64_t offset;
  };
}

#endif /* KLEE_KINSTRUCTION_H */
