//===-- Casting.h -----------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_CASTING_H
#define KLEE_CASTING_H

#include "klee/Config/Version.h"

#include "llvm/Support/Casting.h"

namespace klee {

using llvm::cast;
using llvm::cast_or_null;
using llvm::dyn_cast;
using llvm::dyn_cast_or_null;
using llvm::isa;
#if LLVM_VERSION_CODE >= LLVM_VERSION(9, 0)
using llvm::isa_and_nonnull;
#else
template <typename... X, typename Y>
inline bool isa_and_nonnull(const Y &value) {
  return value && isa<X...>(value);
}
#endif

} // namespace klee

#endif /* KLEE_CASTING_H */