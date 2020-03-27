add_klee_unit_test(SolverTest
  SolverTest.cpp)
target_link_libraries(SolverTest PRIVATE kleaverSolver)

if (${ENABLE_Z3})
  add_klee_unit_test(Z3SolverTest
    Z3SolverTest.cpp)
target_link_libraries(Z3SolverTest PRIVATE kleaverSolver)
endif()
