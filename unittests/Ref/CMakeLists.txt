add_klee_unit_test(RefTest
  RefTest.cpp)
# FIXME: KLEE's libraries should just declare what libraries
# they depend on so we don't need to manually link.
klee_get_llvm_libs(LLVM_LIBS Support)
target_link_libraries(RefTest PRIVATE kleaverExpr ${LLVM_LIBS})
