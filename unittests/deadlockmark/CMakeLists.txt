add_klee_unit_test(marktest1
  deadlock.cpp)
target_link_libraries(ExprTest PRIVATE kleaverExpr kleeSupport kleaverSolver)
target_compile_definitions(marktest1 PRIVATE ${KLEE_COMPONENT_CXX_DEFINES})
target_compile_options(marktest1 PRIVATE ${KLEE_COMPONENT_CXX_FLAGS})
target_include_directories(marktest1 PRIVATE ${KLEE_INCLUDE_DIRS})