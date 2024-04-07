add_klee_unit_test(deadlock
  deadlock.cpp)
target_link_libraries(deadlock PRIVATE kleaverExpr kleaverSolver)
target_compile_options(deadlock PRIVATE ${KLEE_COMPONENT_CXX_FLAGS})
target_compile_definitions(deadlock PRIVATE ${KLEE_COMPONENT_CXX_DEFINES})

target_include_directories(deadlock PRIVATE ${KLEE_INCLUDE_DIRS})
