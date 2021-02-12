#ifndef KLEE_Z3HASHCONFIG_H
#define KLEE_Z3HASHCONFIG_H

#include <atomic>
#include <llvm/Support/CommandLine.h>

namespace Z3HashConfig {
extern llvm::cl::opt<bool> UseConstructHashZ3;
extern std::atomic<bool> Z3InteractionLogOpen;
} // namespace Z3HashConfig
#endif // KLEE_Z3HASHCONFIG_H
