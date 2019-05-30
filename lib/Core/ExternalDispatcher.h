//===-- ExternalDispatcher.h ------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_EXTERNALDISPATCHER_H
#define KLEE_EXTERNALDISPATCHER_H

#include "klee/Config/Version.h"

#include <map>
#include <memory>
#include <stdint.h>
#include <string>

namespace llvm {
class Instruction;
class LLVMContext;
class Function;
}

namespace klee {
class ExternalDispatcherImpl;
class ExternalDispatcher {
private:
  ExternalDispatcherImpl *impl;

public:
  ExternalDispatcher(llvm::LLVMContext &ctx);
  ~ExternalDispatcher();

  /* Call the given function using the parameter passing convention of
   * ci with arguments in args[1], args[2], ... and writing the result
   * into args[0].
   */
  bool executeCall(llvm::Function *function, llvm::Instruction *i,
                   uint64_t *args);
  void *resolveSymbol(const std::string &name);

  int getLastErrno();
  void setLastErrno(int newErrno);
};
}

#endif /* KLEE_EXTERNALDISPATCHER_H */
