add_klee_unit_test(AnnotationsTest
        AnnotationsTest.cpp)
target_link_libraries(AnnotationsTest PRIVATE kleaverExpr kleaverSolver)
