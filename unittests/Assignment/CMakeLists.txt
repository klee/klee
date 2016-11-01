add_klee_unit_test(AssignmentTest
  AssignmentTest.cpp)
klee_get_llvm_libs(LLVM_LIBS Support)
target_link_libraries(AssignmentTest PRIVATE kleaverExpr ${LLVM_LIBS})
