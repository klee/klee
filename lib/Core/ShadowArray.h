//===--- ShadowArray.h ------------------------------------------*- C++ -*-===//
//
//               The Tracer-X KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declarations for the shadow array, which replaces KLEE arrays with their shadow counterparts as existentially-quantified variables in the interpolant.
///
//===----------------------------------------------------------------------===//

#ifndef KLEE_SHADOWARRAY_H
#define KLEE_SHADOWARRAY_H

#include "AddressSpace.h"

namespace klee {

  /// \brief Implements the replacement mechanism for replacing variables, used in
  /// replacing free with bound variables.
  class ShadowArray {
    static std::map<const Array *, const Array *> shadowArray;

    static UpdateNode *getShadowUpdate(const UpdateNode *chain,
				       std::set<const Array *> &replacements);

  public:
    static ref<Expr> createBinaryOfSameKind(ref<Expr> originalExpr,
					    ref<Expr> newLhs, ref<Expr> newRhs);

    static void addShadowArrayMap(const Array *source, const Array *target);

    static ref<Expr> getShadowExpression(ref<Expr> expr,
					 std::set<const Array *> &replacements);

    static std::string getShadowName(std::string name) {
      return "__shadow__" + name;
    }
  };

}

#endif
