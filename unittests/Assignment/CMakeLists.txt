add_klee_unit_test(AssignmentTest
  AssignmentTest.cpp)
target_link_libraries(AssignmentTest PRIVATE kleaverExpr kleaverSolver)
target_compile_options(AssignmentTest PRIVATE ${KLEE_COMPONENT_CXX_FLAGS})
target_compile_definitions(AssignmentTest PRIVATE ${KLEE_COMPONENT_CXX_DEFINES})

target_include_directories(AssignmentTest PRIVATE ${KLEE_INCLUDE_DIRS})
