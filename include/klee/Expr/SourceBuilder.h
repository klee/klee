#ifndef KLEE_SOURCEBUILDER_H
#define KLEE_SOURCEBUILDER_H

#include "klee/ADT/Ref.h"
#include "klee/Expr/SymbolicSource.h"

namespace klee {

class SourceBuilder {
private:
  static ref<SymbolicSource> constantSource;
  static ref<SymbolicSource> makeSymbolicSource;
  static ref<SymbolicSource> symbolicAddressSource;
  static ref<SymbolicSource> lazyInitializationSymbolicSource;
  static ref<SymbolicSource> irreproducibleSource;
  static ref<SymbolicSource> symbolicValueSource;

public:
  SourceBuilder() = delete;

  static ref<SymbolicSource> constant();
  static ref<SymbolicSource> constantWithSymbolicSize(unsigned defaultValue);
  static ref<SymbolicSource> makeSymbolic();
  static ref<SymbolicSource> symbolicAddress();
  static ref<SymbolicSource> symbolicSize();
  static ref<SymbolicSource> lazyInitializationSymbolic();
  static ref<SymbolicSource> irreproducible();
  static ref<SymbolicSource> symbolicValue();
};

}; // namespace klee

#endif /* KLEE_EXPRBUILDER_H */
