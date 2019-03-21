add_klee_unit_test(ExprTest
  ExprTest.cpp)
target_link_libraries(ExprTest PRIVATE kleaverExpr)

add_klee_unit_test(CExprTest
   CExprTest.cpp)
target_link_libraries(CExprTest PRIVATE expr)
