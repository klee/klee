#include "Z3HashConfig.h"
#include <klee/Expr/Expr.h>

namespace Z3HashConfig {
llvm::cl::opt<bool> UseConstructHashZ3(
    "use-construct-hash-z3",
    llvm::cl::desc(
        "Use hash-consing during Z3 query construction (default=true)"),
    llvm::cl::init(true), llvm::cl::cat(klee::ExprCat));

std::atomic<bool> Z3InteractionLogOpen(false);
} // namespace Z3HashConfig
