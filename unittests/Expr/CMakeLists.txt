add_klee_unit_test(ExprTest
  ExprTest.cpp
  ArrayExprTest.cpp)
target_link_libraries(ExprTest PRIVATE kleaverExpr kleeSupport kleaverSolver)
