//===-- InstructionInfoTable.h ----------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_LIB_INSTRUCTIONINFOTABLE_H
#define KLEE_LIB_INSTRUCTIONINFOTABLE_H

#include <map>
#include <string>
#include <set>

namespace llvm {
  class Function;
  class Instruction;
  class Module; 
}

namespace klee {

  /* Stores debug information for a KInstruction */
  struct InstructionInfo {
    unsigned id;
    const std::string &file;
    unsigned line;
    unsigned assemblyLine;

  public:
    InstructionInfo(unsigned _id,
                    const std::string &_file,
                    unsigned _line,
                    unsigned _assemblyLine)
      : id(_id), 
        file(_file),
        line(_line),
        assemblyLine(_assemblyLine) {
    }
  };

  class InstructionInfoTable {
    struct ltstr { 
      bool operator()(const std::string *a, const std::string *b) const {
        return *a<*b;
      }
    };

    std::string dummyString;
    InstructionInfo dummyInfo;
    std::map<const llvm::Instruction*, InstructionInfo> infos;
    std::set<const std::string *, ltstr> internedStrings;

  private:
    const std::string *internString(std::string s);
    bool getInstructionDebugInfo(const llvm::Instruction *I,
                                 const std::string *&File, unsigned &Line);

  public:
    InstructionInfoTable(llvm::Module *m);
    ~InstructionInfoTable();

    unsigned getMaxID() const;
    const InstructionInfo &getInfo(const llvm::Instruction*) const;
    const InstructionInfo &getFunctionInfo(const llvm::Function*) const;
  };

}

#endif
