#ifndef KLEE_SOURCEBUILDER_H
#define KLEE_SOURCEBUILDER_H

#include "klee/Expr/SymbolicSource.h"

#include <unordered_map>

namespace klee {

class SourceBuilder {
private:
  static ref<SymbolicSource> constantSource;
  static ref<SymbolicSource> makeSymbolicSource;
  static ref<SymbolicSource> symbolicAddressSource;
  static ref<SymbolicSource> lazyInitializationSymbolicSource;

public:
  SourceBuilder() = delete;

  static ref<SymbolicSource> constant();
  static ref<SymbolicSource> makeSymbolic();
  static ref<SymbolicSource> symbolicAddress();
  static ref<SymbolicSource> lazyInitializationMakeSymbolic();
};

}; // namespace klee

#endif /* KLEE_EXPRBUILDER_H */
