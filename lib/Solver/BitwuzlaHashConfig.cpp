//===-- BitwuzlaHashConfig.cpp ---------------------------------------*- C++
//-*-====//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "BitwuzlaHashConfig.h"
#include <klee/Expr/Expr.h>

namespace BitwuzlaHashConfig {
llvm::cl::opt<bool> UseConstructHashBitwuzla(
    "use-construct-hash-bitwuzla",
    llvm::cl::desc(
        "Use hash-consing during Bitwuzla query construction (default=true)"),
    llvm::cl::init(true), llvm::cl::cat(klee::ExprCat));
} // namespace BitwuzlaHashConfig
