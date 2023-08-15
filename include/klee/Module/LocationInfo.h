////===-- LocationInfo.h ----------------------------------*- C++ -*-===//
////
////                     The KLEE Symbolic Virtual Machine
////
//// This file is distributed under the University of Illinois Open Source
//// License. See LICENSE.TXT for details.
////
////===----------------------------------------------------------------------===//

#ifndef KLEE_LOCATIONINFO_H
#define KLEE_LOCATIONINFO_H

#include <memory>
#include <string>

namespace llvm {
class Function;
class Instruction;
class Module;
} // namespace llvm

namespace klee {

struct LocationInfo {
  std::string file;
  size_t line;
  size_t column;
};

LocationInfo getLocationInfo(const llvm::Function *func);
LocationInfo getLocationInfo(const llvm::Instruction *inst);

} // namespace klee

#endif /* KLEE_LOCATIONINFO_H */
