add_klee_unit_test(DiscretePDFTest
  DiscretePDFTest.cpp)
# FIXME add the following line to link against libgtest.a
target_link_libraries(DiscretePDFTest PRIVATE kleaverSolver)