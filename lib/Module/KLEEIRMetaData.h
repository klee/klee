//===-- KLEEIRMetaData.h ----------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_KLEEIRMETADATA_H
#define KLEE_KLEEIRMETADATA_H

#include "llvm/IR/MDBuilder.h"

namespace klee {

/// Handles KLEE-specific LLVM IR meta-data.
class KleeIRMetaData : public llvm::MDBuilder {

  llvm::LLVMContext &Context;

public:
  KleeIRMetaData(llvm::LLVMContext &context)
      : llvm::MDBuilder(context), Context(context) {}

  /// \brief Return a string node reflecting the value
  llvm::MDNode *createStringNode(llvm::StringRef value) {
    return llvm::MDNode::get(Context, createString(value));
  }

  void addAnnotation(llvm::Instruction &inst, llvm::StringRef key,
                           llvm::StringRef value) {
    inst.setMetadata(key, createStringNode(value));
  }

  /// \brief Check if the instruction has the key/value meta data
  static bool hasAnnotation(const llvm::Instruction &inst, llvm::StringRef key,
                             llvm::StringRef value) {
    auto v = inst.getMetadata(key);
    if (!v)
      return false;
    auto sv = llvm::dyn_cast<llvm::MDString>(v->getOperand(0));
    if (!sv)
      return false;

    return sv->getString().equals(value);
  }
};
}

#endif /* KLEE_KLEEIRMETADATA_H */
