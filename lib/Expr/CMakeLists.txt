#===------------------------------------------------------------------------===#
#
#                     The KLEE Symbolic Virtual Machine
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#
add_library(kleaverExpr
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
  IndependentConstraintSetUnion.cpp
  IndependentSet.cpp
  Path.cpp
  SourceBuilder.cpp
  SymbolicSource.cpp
  Lexer.cpp
  Parser.cpp
  Updates.cpp
)

llvm_config(kleaverExpr "${USE_LLVM_SHARED}" support)
target_include_directories(kleaverExpr PRIVATE ${KLEE_INCLUDE_DIRS} ${LLVM_INCLUDE_DIRS})
target_compile_options(kleaverExpr PRIVATE ${KLEE_COMPONENT_CXX_FLAGS})
target_compile_definitions(kleaverExpr PRIVATE ${KLEE_COMPONENT_CXX_DEFINES})
