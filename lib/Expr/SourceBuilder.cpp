#include "klee/Expr/SourceBuilder.h"
#include "klee/Expr/Expr.h"
#include "klee/Expr/SymbolicSource.h"

using namespace klee;

ref<SymbolicSource> SourceBuilder::constantSource =
    ref<SymbolicSource>(new ConstantSource());
ref<SymbolicSource> SourceBuilder::makeSymbolicSource =
    ref<SymbolicSource>(new MakeSymbolicSource());
ref<SymbolicSource> SourceBuilder::lazyInitializationMakeSymbolicSource =
    ref<SymbolicSource>(new LazyInitializationSymbolicSource());

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

ref<SymbolicSource> SourceBuilder::lazyInitializationMakeSymbolic() {
  return SourceBuilder::lazyInitializationMakeSymbolicSource;
}
