//===-- InstructionInfoTable.h ----------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_INSTRUCTIONINFOTABLE_H
#define KLEE_INSTRUCTIONINFOTABLE_H

#include <llvm/ADT/Optional.h>
#include <llvm/Support/raw_ostream.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace llvm {
class Function;
class Instruction;
class Module;
} // namespace llvm

namespace klee {

/// @brief InstructionInfo stores debug information for a KInstruction.
struct InstructionInfo {
  /// @brief The instruction id.
  unsigned id;
  /// @brief Line number in source file.
  unsigned line;
  /// @brief Column number in source file.
  unsigned column;
  /// @brief Line number in generated assembly.ll.
  llvm::Optional<uint64_t> assemblyLine;
  /// @brief Source file name.
  const std::string &file;

public:
  InstructionInfo(unsigned id, const std::string &file, unsigned line,
                  unsigned column, llvm::Optional<uint64_t> assemblyLine)
      : id{id}, line{line}, column{column},
        assemblyLine{assemblyLine}, file{file} {}
};

/// @brief FunctionInfo stores debug information for a KFunction.
struct FunctionInfo {
  /// @brief The function id.
  unsigned id;
  /// @brief Line number in source file.
  unsigned line;
  /// @brief Line number in generated assembly.ll.
  llvm::Optional<uint64_t> assemblyLine;
  /// @brief Source file name.
  const std::string &file;

public:
  FunctionInfo(unsigned id, const std::string &file, unsigned line,
               llvm::Optional<uint64_t> assemblyLine)
      : id{id}, line{line}, assemblyLine{assemblyLine}, file{file} {}

  FunctionInfo(const FunctionInfo &) = delete;
  FunctionInfo &operator=(FunctionInfo const &) = delete;

  FunctionInfo(FunctionInfo &&) = default;
};

class InstructionInfoTable {
  std::unordered_map<const llvm::Instruction *,
                     std::unique_ptr<InstructionInfo>>
      infos;
  std::unordered_map<const llvm::Function *, std::unique_ptr<FunctionInfo>>
      functionInfos;
  std::vector<std::unique_ptr<std::string>> internedStrings;

public:
  explicit InstructionInfoTable(const llvm::Module &m,
                                std::unique_ptr<llvm::raw_fd_ostream> assemblyFS);

  unsigned getMaxID() const;
  const InstructionInfo &getInfo(const llvm::Instruction &) const;
  const FunctionInfo &getFunctionInfo(const llvm::Function &) const;
};

} // namespace klee

#endif /* KLEE_INSTRUCTIONINFOTABLE_H */
