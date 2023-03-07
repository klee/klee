#include "klee/Expr/SourceBuilder.h"
#include "klee/Expr/Expr.h"
#include "klee/Expr/SymbolicSource.h"

using namespace klee;

ref<SymbolicSource> SourceBuilder::constantSource =
    ref<SymbolicSource>(new ConstantSource());
ref<SymbolicSource> SourceBuilder::makeSymbolicSource =
    ref<SymbolicSource>(new MakeSymbolicSource());
ref<SymbolicSource> SourceBuilder::lazyInitializationSymbolicSource =
    ref<SymbolicSource>(new LazyInitializationSymbolicSource());
ref<SymbolicSource> SourceBuilder::irreproducibleSource =
    ref<SymbolicSource>(new IrreproducibleSource());
ref<SymbolicSource> SourceBuilder::symbolicValueSource =
    ref<SymbolicSource>(new SymbolicValueSource());

ref<SymbolicSource> SourceBuilder::constant() {
  return SourceBuilder::constantSource;
}

ref<SymbolicSource>
SourceBuilder::constantWithSymbolicSize(unsigned defaultValue) {
  return new ConstantWithSymbolicSizeSource(defaultValue);
}

ref<SymbolicSource> SourceBuilder::makeSymbolic() {
  return SourceBuilder::makeSymbolicSource;
}

ref<SymbolicSource> SourceBuilder::symbolicAddress() {
  return new SymbolicAddressSource();
}

ref<SymbolicSource> SourceBuilder::symbolicSize() {
  return new SymbolicSizeSource();
}

ref<SymbolicSource> SourceBuilder::lazyInitializationSymbolic() {
  return SourceBuilder::lazyInitializationSymbolicSource;
}

ref<SymbolicSource> SourceBuilder::irreproducible() {
  return SourceBuilder::irreproducibleSource;
}

ref<SymbolicSource> SourceBuilder::symbolicValue() {
  return SourceBuilder::symbolicValueSource;
}
