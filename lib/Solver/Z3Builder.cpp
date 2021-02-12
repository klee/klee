//===-- Z3Builder.cpp ------------------------------------------*- C++ -*-====//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#include "klee/Config/config.h"

#ifdef ENABLE_Z3

#include "Z3Builder.h"
#include "klee/ADT/Bits.h"

#include "klee/Expr/Expr.h"
#include "klee/Solver/Solver.h"
#include "klee/Solver/SolverStats.h"
#include "klee/Support/ErrorHandling.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/Support/CommandLine.h"

using namespace klee;

namespace klee {
// Declared here rather than `Z3Builder.h` so they can be called in gdb.
template <> void Z3NodeHandle<Z3_sort>::dump() {
  llvm::errs() << "Z3SortHandle:\n"
               << ::Z3_sort_to_string(context, node) << "\n";
}

template <> void Z3NodeHandle<Z3_ast>::dump() {
  llvm::errs() << "Z3ASTHandle:\n"
               << ::Z3_ast_to_string(context, as_ast()) << "\n";
}

void custom_z3_error_handler(Z3_context ctx, Z3_error_code ec) {
  ::Z3_string errorMsg =
#ifdef HAVE_Z3_GET_ERROR_MSG_NEEDS_CONTEXT
      // Z3 > 4.4.1
      Z3_get_error_msg(ctx, ec);
#else
      // Z3 4.4.1
      Z3_get_error_msg(ec);
#endif
  // FIXME: This is kind of a hack. The value comes from the enum
  // Z3_CANCELED_MSG but this isn't currently exposed by Z3's C API
  if (strcmp(errorMsg, "canceled") == 0) {
    // Solver timeout is not a fatal error
    return;
  }
  llvm::errs() << "Error: Incorrect use of Z3. [" << ec << "] " << errorMsg
               << "\n";
  // NOTE: The current implementation of `Z3_close_log()` can be safely
  // called even if the log isn't open.
  Z3_close_log();
  abort();
}

Z3ArrayExprHash::~Z3ArrayExprHash() {}

void Z3ArrayExprHash::clear() {
  _update_node_hash.clear();
  _array_hash.clear();
}

void Z3ArrayExprHash::clearUpdates() { _update_node_hash.clear(); }

Z3Builder::Z3Builder(bool autoClearConstructCache,
                     const char *z3LogInteractionFileArg)
    : autoClearConstructCache(autoClearConstructCache),
      z3LogInteractionFile("") {
  if (z3LogInteractionFileArg)
    this->z3LogInteractionFile = std::string(z3LogInteractionFileArg);
  if (z3LogInteractionFile.length() > 0) {
    klee_message("Logging Z3 API interaction to \"%s\"",
                 z3LogInteractionFile.c_str());
    assert(!Z3HashConfig::Z3InteractionLogOpen &&
           "interaction log should not already be open");
    Z3_open_log(z3LogInteractionFile.c_str());
    Z3HashConfig::Z3InteractionLogOpen = true;
  }
  // FIXME: Should probably let the client pass in a Z3_config instead
  Z3_config cfg = Z3_mk_config();
  // It is very important that we ask Z3 to let us manage memory so that
  // we are able to cache expressions and sorts.
  ctx = Z3_mk_context_rc(cfg);
  // Make sure we handle any errors reported by Z3.
  Z3_set_error_handler(ctx, custom_z3_error_handler);
  // When emitting Z3 expressions make them SMT-LIBv2 compliant
  Z3_set_ast_print_mode(ctx, Z3_PRINT_SMTLIB2_COMPLIANT);
  Z3_del_config(cfg);
}

Z3Builder::~Z3Builder() {
  // Clear caches so exprs/sorts gets freed before the destroying context
  // they aren associated with.
  clearConstructCache();
  _arr_hash.clear();
  constant_array_assertions.clear();
  Z3_del_context(ctx);
  if (z3LogInteractionFile.length() > 0) {
    Z3_close_log();
    Z3HashConfig::Z3InteractionLogOpen = false;
  }
}

Z3SortHandle Z3Builder::getBvSort(unsigned width) {
  // FIXME: cache these
  return Z3SortHandle(Z3_mk_bv_sort(ctx, width), ctx);
}

Z3SortHandle Z3Builder::getArraySort(Z3SortHandle domainSort,
                                     Z3SortHandle rangeSort) {
  // FIXME: cache these
  return Z3SortHandle(Z3_mk_array_sort(ctx, domainSort, rangeSort), ctx);
}

Z3ASTHandle Z3Builder::buildArray(const char *name, unsigned indexWidth,
                                  unsigned valueWidth) {
  Z3SortHandle domainSort = getBvSort(indexWidth);
  Z3SortHandle rangeSort = getBvSort(valueWidth);
  Z3SortHandle t = getArraySort(domainSort, rangeSort);
  Z3_symbol s = Z3_mk_string_symbol(ctx, const_cast<char *>(name));
  return Z3ASTHandle(Z3_mk_const(ctx, s, t), ctx);
}

Z3ASTHandle Z3Builder::getTrue() { return Z3ASTHandle(Z3_mk_true(ctx), ctx); }

Z3ASTHandle Z3Builder::getFalse() { return Z3ASTHandle(Z3_mk_false(ctx), ctx); }

Z3ASTHandle Z3Builder::bvOne(unsigned width) { return bvZExtConst(width, 1); }

Z3ASTHandle Z3Builder::bvZero(unsigned width) { return bvZExtConst(width, 0); }

Z3ASTHandle Z3Builder::bvMinusOne(unsigned width) {
  return bvSExtConst(width, (int64_t)-1);
}

Z3ASTHandle Z3Builder::bvConst32(unsigned width, uint32_t value) {
  Z3SortHandle t = getBvSort(width);
  return Z3ASTHandle(Z3_mk_unsigned_int(ctx, value, t), ctx);
}

Z3ASTHandle Z3Builder::bvConst64(unsigned width, uint64_t value) {
  Z3SortHandle t = getBvSort(width);
  return Z3ASTHandle(Z3_mk_unsigned_int64(ctx, value, t), ctx);
}

Z3ASTHandle Z3Builder::bvZExtConst(unsigned width, uint64_t value) {
  if (width <= 64)
    return bvConst64(width, value);

  Z3ASTHandle expr = Z3ASTHandle(bvConst64(64, value), ctx);
  Z3ASTHandle zero = Z3ASTHandle(bvConst64(64, 0), ctx);
  for (width -= 64; width > 64; width -= 64)
    expr = Z3ASTHandle(Z3_mk_concat(ctx, zero, expr), ctx);
  return Z3ASTHandle(Z3_mk_concat(ctx, bvConst64(width, 0), expr), ctx);
}

Z3ASTHandle Z3Builder::bvSExtConst(unsigned width, uint64_t value) {
  if (width <= 64)
    return bvConst64(width, value);

  Z3SortHandle t = getBvSort(width - 64);
  if (value >> 63) {
    Z3ASTHandle r = Z3ASTHandle(Z3_mk_int64(ctx, -1, t), ctx);
    return Z3ASTHandle(Z3_mk_concat(ctx, r, bvConst64(64, value)), ctx);
  }

  Z3ASTHandle r = Z3ASTHandle(Z3_mk_int64(ctx, 0, t), ctx);
  return Z3ASTHandle(Z3_mk_concat(ctx, r, bvConst64(64, value)), ctx);
}

Z3ASTHandle Z3Builder::bvBoolExtract(Z3ASTHandle expr, int bit) {
  return Z3ASTHandle(Z3_mk_eq(ctx, bvExtract(expr, bit, bit), bvOne(1)), ctx);
}

Z3ASTHandle Z3Builder::notExpr(Z3ASTHandle expr) {
  return Z3ASTHandle(Z3_mk_not(ctx, expr), ctx);
}

Z3ASTHandle Z3Builder::andExpr(Z3ASTHandle lhs, Z3ASTHandle rhs) {
  ::Z3_ast args[2] = {lhs, rhs};
  return Z3ASTHandle(Z3_mk_and(ctx, 2, args), ctx);
}

Z3ASTHandle Z3Builder::orExpr(Z3ASTHandle lhs, Z3ASTHandle rhs) {
  ::Z3_ast args[2] = {lhs, rhs};
  return Z3ASTHandle(Z3_mk_or(ctx, 2, args), ctx);
}

Z3ASTHandle Z3Builder::iffExpr(Z3ASTHandle lhs, Z3ASTHandle rhs) {
  Z3SortHandle lhsSort = Z3SortHandle(Z3_get_sort(ctx, lhs), ctx);
  Z3SortHandle rhsSort = Z3SortHandle(Z3_get_sort(ctx, rhs), ctx);
  assert(Z3_get_sort_kind(ctx, lhsSort) == Z3_get_sort_kind(ctx, rhsSort) &&
         "lhs and rhs sorts must match");
  assert(Z3_get_sort_kind(ctx, lhsSort) == Z3_BOOL_SORT &&
         "args must have BOOL sort");
  return Z3ASTHandle(Z3_mk_iff(ctx, lhs, rhs), ctx);
}

Z3ASTHandle Z3Builder::writeExpr(Z3ASTHandle array, Z3ASTHandle index,
                                 Z3ASTHandle value) {
  return Z3ASTHandle(Z3_mk_store(ctx, array, index, value), ctx);
}

Z3ASTHandle Z3Builder::readExpr(Z3ASTHandle array, Z3ASTHandle index) {
  return Z3ASTHandle(Z3_mk_select(ctx, array, index), ctx);
}

unsigned Z3Builder::getBVLength(Z3ASTHandle expr) {
  return Z3_get_bv_sort_size(ctx, Z3SortHandle(Z3_get_sort(ctx, expr), ctx));
}

Z3ASTHandle Z3Builder::getInitialArray(const Array *root) {

  assert(root);
  Z3ASTHandle array_expr;
  bool hashed = _arr_hash.lookupArrayExpr(root, array_expr);

  if (!hashed) {
    // Unique arrays by name, so we make sure the name is unique by
    // using the size of the array hash as a counter.
    std::string unique_id = llvm::utostr(_arr_hash._array_hash.size());
    std::string unique_name = root->name + unique_id;

    array_expr =
        buildArray(unique_name.c_str(), root->getDomain(), root->getRange());

    if (root->isConstantArray() && constant_array_assertions.count(root) == 0) {
      std::vector<Z3ASTHandle> array_assertions;
      for (unsigned i = 0, e = root->size; i != e; ++i) {
        // construct(= (select i root) root->value[i]) to be asserted in
        // Z3Solver.cpp
        int width_out;
        Z3ASTHandle array_value =
            construct(root->constantValues[i], &width_out);
        assert(width_out == (int)root->getRange() &&
               "Value doesn't match root range");
        array_assertions.push_back(
            eqExpr(readExpr(array_expr, bvConst32(root->getDomain(), i)),
                   array_value));
      }
      constant_array_assertions[root] = std::move(array_assertions);
    }

    _arr_hash.hashArrayExpr(root, array_expr);
  }

  return (array_expr);
}

Z3ASTHandle Z3Builder::getInitialRead(const Array *root, unsigned index) {
  return readExpr(getInitialArray(root), bvConst32(32, index));
}

Z3ASTHandle Z3Builder::getArrayForUpdate(const Array *root,
                                         const UpdateNode *un) {
  if (!un) {
    return (getInitialArray(root));
  } else {
    // FIXME: This really needs to be non-recursive.
    Z3ASTHandle un_expr;
    bool hashed = _arr_hash.lookupUpdateNodeExpr(un, un_expr);

    if (!hashed) {
      un_expr = writeExpr(getArrayForUpdate(root, un->next.get()),
                          construct(un->index, 0), construct(un->value, 0));

      _arr_hash.hashUpdateNodeExpr(un, un_expr);
    }

    return (un_expr);
  }
}

Z3ASTHandle Z3Builder::construct(ref<Expr> e, int *width_out) {
  // TODO: We could potentially use Z3_simplify() here
  // to store simpler expressions.
  if (!Z3HashConfig::UseConstructHashZ3 || isa<ConstantExpr>(e)) {
    return constructActual(e, width_out);
  } else {
    ExprHashMap<std::pair<Z3ASTHandle, unsigned>>::iterator it =
        constructed.find(e);
    if (it != constructed.end()) {
      if (width_out)
        *width_out = it->second.second;
      return it->second.first;
    } else {
      int width;
      if (!width_out)
        width_out = &width;
      Z3ASTHandle res = constructActual(e, width_out);
      constructed.insert(std::make_pair(e, std::make_pair(res, *width_out)));
      return res;
    }
  }
}
} // namespace klee
#endif // ENABLE_Z3
