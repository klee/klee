add_klee_unit_test(ExprTest
  ExprTest.cpp)
# FIXME: KLEE's libraries should just declare what libraries
# they depend on so we don't need to manually link.
klee_get_llvm_libs(LLVM_LIBS Support)
target_link_libraries(ExprTest PRIVATE kleaverExpr ${LLVM_LIBS})
