//===-- Context.cpp -------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Core/Context.h"

#include "klee/Expr/Expr.h"

#include <cassert>

using namespace klee;

bool klee::ContextInitialized = false;
static Context TheContext;

void Context::initialize(bool IsLittleEndian, Expr::Width PointerWidth) {
  assert(!ContextInitialized && "Duplicate context initialization!");
  TheContext = Context(IsLittleEndian, PointerWidth);
  ContextInitialized = true;
}

const Context &Context::get() {
  assert(ContextInitialized && "Context has not been initialized!");
  return TheContext;
}

// FIXME: This is a total hack, just to avoid a layering issue until this stuff
// moves out of Expr.

ref<Expr> Expr::createSExtToPointerWidth(ref<Expr> e) {
  return SExtExpr::create(e, Context::get().getPointerWidth());
}

ref<Expr> Expr::createZExtToPointerWidth(ref<Expr> e) {
  return ZExtExpr::create(e, Context::get().getPointerWidth());
}

ref<ConstantExpr> Expr::createPointer(uint64_t v) {
  return ConstantExpr::create(v, Context::get().getPointerWidth());
}
