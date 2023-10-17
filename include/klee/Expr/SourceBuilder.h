#ifndef KLEE_SOURCEBUILDER_H
#define KLEE_SOURCEBUILDER_H

#include "klee/ADT/Ref.h"
#include "klee/ADT/SparseStorage.h"
#include "klee/Expr/SymbolicSource.h"
#include "klee/Module/KModule.h"

namespace klee {

class SourceBuilder {
public:
  SourceBuilder() = delete;

  static ref<SymbolicSource>
  constant(SparseStorage<ref<ConstantExpr>> constantValues);

  static ref<SymbolicSource> uninitialized(unsigned version,
                                           const KInstruction *allocSite);
  static ref<SymbolicSource>
  symbolicSizeConstantAddress(unsigned version, const KInstruction *allocSite,
                              ref<Expr> size);
  static ref<SymbolicSource> makeSymbolic(const std::string &name,
                                          unsigned version);
  static ref<SymbolicSource> lazyInitializationAddress(ref<Expr> pointer);
  static ref<SymbolicSource> lazyInitializationSize(ref<Expr> pointer);
  static ref<SymbolicSource> lazyInitializationContent(ref<Expr> pointer);
  static ref<SymbolicSource> argument(const llvm::Argument &_allocSite,
                                      int _index, KModule *km);
  static ref<SymbolicSource> instruction(const llvm::Instruction &_allocSite,
                                         int _index, KModule *km);
  static ref<SymbolicSource> value(const llvm::Value &_allocSite, int _index,
                                   KModule *km);
  static ref<SymbolicSource> irreproducible(const std::string &name);
};

}; // namespace klee

#endif /* KLEE_EXPRBUILDER_H */
