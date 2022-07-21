#===------------------------------------------------------------------------===#
#
#                     The KLEE Symbolic Virtual Machine
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#
klee_add_component(kleaverExpr
  APFloatEval.cpp
  ArrayCache.cpp
  ArrayExprOptimizer.cpp
  ArrayExprRewriter.cpp
  ArrayExprVisitor.cpp
  Assignment.cpp
  AssignmentGenerator.cpp
  Constraints.cpp
  ExprBuilder.cpp
  Expr.cpp
  ExprEvaluator.cpp
  ExprPPrinter.cpp
  ExprSMTLIBPrinter.cpp
  ExprUtil.cpp
  ExprVisitor.cpp
  IndependentSet.cpp
  SourceBuilder.cpp
  SymbolicSource.cpp
  Lexer.cpp
  Parser.cpp
  Updates.cpp
)

set(LLVM_COMPONENTS
  support
)
klee_get_llvm_libs(LLVM_LIBS ${LLVM_COMPONENTS})
target_link_libraries(kleaverExpr PRIVATE ${LLVM_LIBS})

install(TARGETS kleaverExpr
        EXPORT run_klee
        DESTINATION "${RUN_KLEE_LIB_DEST}"
        )
