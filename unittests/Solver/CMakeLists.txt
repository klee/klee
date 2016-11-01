add_klee_unit_test(SolverTest
  SolverTest.cpp)
# FIXME: KLEE's libraries should just declare what libraries
# they depend on so we don't need to manually link.
klee_get_llvm_libs(LLVM_LIBS Support)
target_link_libraries(SolverTest
  PRIVATE
  kleaverSolver
  kleaverExpr
  kleeSupport
  kleeBasic
  ${LLVM_LIBS})
