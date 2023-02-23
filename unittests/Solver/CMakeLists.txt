add_klee_unit_test(SolverTest
  SolverTest.cpp)
target_link_libraries(SolverTest PRIVATE kleaverSolver)
target_compile_options(SolverTest PRIVATE ${KLEE_COMPONENT_CXX_FLAGS})
target_compile_definitions(SolverTest PRIVATE ${KLEE_COMPONENT_CXX_DEFINES})
target_include_directories(SolverTest PRIVATE ${KLEE_INCLUDE_DIRS})


if (${ENABLE_Z3})
  add_klee_unit_test(Z3SolverTest
    Z3SolverTest.cpp)
  target_link_libraries(Z3SolverTest PRIVATE kleaverSolver)
  target_compile_options(Z3SolverTest PRIVATE ${KLEE_COMPONENT_CXX_FLAGS})
  target_compile_definitions(Z3SolverTest PRIVATE ${KLEE_COMPONENT_CXX_DEFINES})
  target_include_directories(Z3SolverTest PRIVATE ${KLEE_INCLUDE_DIRS})
endif()
