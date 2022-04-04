add_klee_unit_test(ExprTest
  ExprTest.cpp
  ArrayExprTest.cpp)
target_link_libraries(ExprTest PRIVATE kleaverExpr kleeSupport kleaverSolver)
target_compile_options(ExprTest PRIVATE ${KLEE_COMPONENT_CXX_FLAGS})
target_compile_definitions(ExprTest PRIVATE ${KLEE_COMPONENT_CXX_DEFINES})

target_include_directories(ExprTest PRIVATE ${KLEE_INCLUDE_DIRS})

